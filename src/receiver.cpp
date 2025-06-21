#include "audio_playback.h"
#include "network_udp.h"
#include "jitter_buffer.h"
#include "logger.h"
#include "config.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <signal.h>

// Global variables for graceful shutdown
std::atomic<bool> g_running(true);
std::atomic<uint64_t> g_frameCounter(0);
std::atomic<uint32_t> g_sequenceCounter(0);

// Signal handler for graceful shutdown
void SignalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        LOG_INFO("Received shutdown signal, stopping receiver...");
        g_running = false;
    }
}

// Statistics display thread
void StatisticsThread(NetworkUDP *network, JitterBuffer *jitterBuffer, AudioPlayback *audioPlayback)
{
    auto lastTime = std::chrono::high_resolution_clock::now();
    uint64_t lastFrameCounter = 0;

    while (g_running)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(g_config.stats_interval_ms));

        auto currentTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime);
        uint64_t currentFrameCounter = g_frameCounter.load();

        // Calculate frame rate
        uint64_t framesInPeriod = currentFrameCounter - lastFrameCounter;
        float frameRate = (framesInPeriod * 1000.0f) / duration.count();

        // Get network, jitter buffer, and audio playback statistics
        auto networkStats = network->GetStats();
        auto jitterStats = jitterBuffer->GetStats();
        auto playbackStats = audioPlayback->GetStats();

        // Display statistics
        LOG_INFO("=== Receiver Statistics ===");
        LOG_INFO_FMT("Frame Rate: {:.2f} fps", frameRate);
        LOG_INFO_FMT("Total Frames Processed: {}", currentFrameCounter);
        LOG_INFO_FMT("Network - Received: {}, Lost: {}, Avg Latency: {:.2f} ms",
                     networkStats.packets_received, networkStats.packets_lost, networkStats.avg_latency_ms);
        LOG_INFO_FMT("Network - Bytes Received: {}", networkStats.bytes_received);
        LOG_INFO("=== Jitter Buffer Stats ===");
        LOG_INFO_FMT("Current Latency: {} ms, Target: {} ms",
                     jitterStats.current_latency_ms, jitterStats.target_latency_ms);
        LOG_INFO_FMT("Buffer Occupancy: {}%, Frames Buffered: {}",
                     jitterStats.buffer_occupancy_percent, jitterStats.frames_buffered);
        LOG_INFO_FMT("Frames Dropped: {}, Reordered: {}, Total Received: {}",
                     jitterStats.frames_dropped, jitterStats.frames_reordered, jitterStats.total_frames_received);
        LOG_INFO_FMT("Average Jitter: {:.2f} ms", jitterStats.avg_jitter_ms);
        LOG_INFO("=== Audio Playback Stats ===");
        LOG_INFO_FMT("Status: {}, Volume: {:.2f}",
                     playbackStats.is_playing ? "Playing" : "Stopped", playbackStats.current_volume);
        LOG_INFO_FMT("Frames Played: {}, Dropped: {}, Underruns: {}",
                     playbackStats.frames_played, playbackStats.frames_dropped, playbackStats.buffer_underruns);
        LOG_INFO_FMT("Format: {}Hz, {}ch, {}bit",
                     playbackStats.sample_rate, playbackStats.channels, playbackStats.bits_per_sample);
        LOG_INFO("==========================");

        lastTime = currentTime;
        lastFrameCounter = currentFrameCounter;
    }
}

// Main receiver loop
void ReceiverLoop(NetworkUDP &network, JitterBuffer &jitterBuffer, AudioPlayback &audioPlayback)
{
    std::vector<uint8_t> packet;
    uint64_t networkTimestamp;

    LOG_INFO("Starting receiver loop...");

    while (g_running)
    {
        // Try to receive next audio packet
        if (network.ReceiveFrame(packet, networkTimestamp))
        {
            // Check if packet is large enough to contain header
            size_t minHeaderSize = sizeof(uint32_t) + sizeof(uint64_t) + sizeof(int) * 3; // seq + timestamp + format
            if (packet.size() < minHeaderSize)
            {
                LOG_WARNING("Received packet too small, skipping");
                continue;
            }

            // Extract sequence number (first 4 bytes)
            uint32_t sequenceNumber;
            memcpy(&sequenceNumber, packet.data(), sizeof(uint32_t));

            // Extract timestamp (next 8 bytes)
            uint64_t timestamp;
            memcpy(&timestamp, packet.data() + sizeof(uint32_t), sizeof(uint64_t));

            // Extract audio format info (next 12 bytes)
            int sampleRate, channels, bitsPerSample;
            memcpy(&sampleRate, packet.data() + sizeof(uint32_t) + sizeof(uint64_t), sizeof(int));
            memcpy(&channels, packet.data() + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(int), sizeof(int));
            memcpy(&bitsPerSample, packet.data() + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(int) * 2, sizeof(int));

            // Extract audio frame data (remaining bytes)
            std::vector<uint8_t> frame;
            size_t frameSize = packet.size() - minHeaderSize;
            frame.assign(packet.begin() + minHeaderSize, packet.end());

            // Check if format conversion is needed
            if (sampleRate != g_config.audio.sample_rate ||
                channels != g_config.audio.channels ||
                bitsPerSample != g_config.audio.bits_per_sample)
            {
                LOG_INFO_FMT("Format conversion needed: {}Hz/{}ch/{}bit -> {}Hz/{}ch/{}bit",
                             sampleRate, channels, bitsPerSample,
                             g_config.audio.sample_rate, g_config.audio.channels, g_config.audio.bits_per_sample);

                // For now, update our config to match the sender's format
                // This ensures compatibility
                g_config.audio.sample_rate = sampleRate;
                g_config.audio.channels = channels;
                g_config.audio.bits_per_sample = bitsPerSample;

                // Update audio playback format
                audioPlayback.SetSampleRate(sampleRate);
                audioPlayback.SetChannels(channels);
                audioPlayback.SetBitsPerSample(bitsPerSample);

                LOG_INFO_FMT("Updated receiver format to match sender: {}Hz/{}ch/{}bit",
                             sampleRate, channels, bitsPerSample);
            }

            // Push frame into jitter buffer
            jitterBuffer.PushFrame(frame, timestamp, sequenceNumber);

            // Try to get frames from jitter buffer and play them
            std::vector<uint8_t> outputFrame;
            while (jitterBuffer.GetFrame(outputFrame))
            {
                g_frameCounter++;

                // Play the audio frame
                if (!audioPlayback.PlayFrame(outputFrame))
                {
                    LOG_DEBUG_FMT("Failed to play frame {}", g_frameCounter.load());
                }

                // Calculate latency (fix the negative latency issue)
                auto now = std::chrono::high_resolution_clock::now();
                auto duration = now.time_since_epoch();
                uint64_t currentTime = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();

                // Ensure positive latency calculation
                uint64_t latency = 0;
                if (currentTime > timestamp)
                {
                    latency = currentTime - timestamp;
                }
                else
                {
                    // Handle clock drift or timestamp issues
                    latency = 0;
                }

                // Log first few frames for debugging
                if (g_frameCounter.load() <= 5)
                {
                    LOG_INFO_FMT("Processed and played frame {} (seq: {}): {} bytes, latency: {} us",
                                 g_frameCounter.load(), sequenceNumber, outputFrame.size(), latency);
                }
            }
        }
        else
        {
            // No frame available, small sleep to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    LOG_INFO("Receiver loop stopped");
}

// Initialize network receiver
bool InitializeNetworkReceiver(NetworkUDP &network)
{
    LOG_INFO("Initializing network receiver...");
    LOG_INFO_FMT("Listening on port: {}", g_config.network.receiver_port);

    // Set network configuration
    network.SetBufferSize(g_config.network.buffer_size);
    network.SetTimeout(g_config.network.timeout_ms);

    if (!network.InitUDPReceiver(g_config.network.receiver_port))
    {
        LOG_ERROR("Failed to initialize UDP receiver");
        return false;
    }

    LOG_INFO("Network receiver initialized successfully");
    return true;
}

// Initialize jitter buffer
bool InitializeJitterBuffer(JitterBuffer &jitterBuffer)
{
    LOG_INFO("Initializing jitter buffer...");

    // Configure jitter buffer
    jitterBuffer.SetTargetLatencyMs(g_config.audio.target_latency_ms);
    jitterBuffer.SetMaxLatencyMs(g_config.audio.max_latency_ms);
    jitterBuffer.SetFrameSize(g_config.audio.frame_size);
    jitterBuffer.SetSampleRate(g_config.audio.sample_rate);
    jitterBuffer.SetChannels(g_config.audio.channels);

    LOG_INFO("Jitter buffer initialized successfully");
    return true;
}

// Initialize audio playback
bool InitializeAudioPlayback(AudioPlayback &audioPlayback)
{
    LOG_INFO("Initializing audio playback...");

    // Configure audio playback
    audioPlayback.SetSampleRate(g_config.audio.sample_rate);
    audioPlayback.SetChannels(g_config.audio.channels);
    audioPlayback.SetBitsPerSample(g_config.audio.bits_per_sample);
    audioPlayback.SetFrameSize(g_config.audio.frame_size);

    // Initialize WASAPI
    if (!audioPlayback.InitAudioPlayback(g_config.audio.device_name))
    {
        LOG_ERROR("Failed to initialize audio playback");
        return false;
    }

    // Start playback
    if (!audioPlayback.StartPlayback())
    {
        LOG_ERROR("Failed to start audio playback");
        return false;
    }

    LOG_INFO("Audio playback initialized and started successfully");
    return true;
}

// Main function
int main(int argc, char *argv[])
{
    // Set default configuration
    SetDefaultConfig();

    // Parse command line arguments
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h")
        {
            std::cout << "AudioSync Receiver - Real-time audio streaming with playback\n";
            std::cout << "Usage: receiver [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --port <port>      Listen port (default: 8889)\n";
            std::cout << "  --buffer <size>    Network buffer size (default: 65536)\n";
            std::cout << "  --timeout <ms>     Network timeout (default: 1000)\n";
            std::cout << "  --target-latency <ms> Jitter buffer target latency (default: 100)\n";
            std::cout << "  --max-latency <ms> Jitter buffer max latency (default: 200)\n";
            std::cout << "  --device <name>    Audio device name (default: default)\n";
            std::cout << "  --volume <0.0-1.0> Audio volume (default: 1.0)\n";
            std::cout << "  --stats-interval <ms> Statistics display interval (default: 1000)\n";
            std::cout << "  --help, -h         Show this help message\n";
            return 0;
        }
        else if (arg == "--port" && i + 1 < argc)
        {
            g_config.network.receiver_port = static_cast<uint16_t>(std::stoi(argv[++i]));
        }
        else if (arg == "--buffer" && i + 1 < argc)
        {
            g_config.network.buffer_size = std::stoi(argv[++i]);
        }
        else if (arg == "--timeout" && i + 1 < argc)
        {
            g_config.network.timeout_ms = std::stoi(argv[++i]);
        }
        else if (arg == "--target-latency" && i + 1 < argc)
        {
            g_config.audio.target_latency_ms = std::stoi(argv[++i]);
        }
        else if (arg == "--max-latency" && i + 1 < argc)
        {
            g_config.audio.max_latency_ms = std::stoi(argv[++i]);
        }
        else if (arg == "--device" && i + 1 < argc)
        {
            g_config.audio.device_name = argv[++i];
        }
        else if (arg == "--volume" && i + 1 < argc)
        {
            float volume = std::stof(argv[++i]);
            g_config.audio.volume = std::max(0.0f, std::min(1.0f, volume));
        }
        else if (arg == "--stats-interval" && i + 1 < argc)
        {
            g_config.stats_interval_ms = std::stoi(argv[++i]);
        }
    }

    // Initialize logger
    LOG_INFO("=== AudioSync Receiver with Playback Starting ===");
    LOG_INFO("Version: 1.0");
    LOG_INFO("Configuration:");
    LOG_INFO_FMT("  Network: port={}, buffer={}, timeout={}ms",
                 g_config.network.receiver_port, g_config.network.buffer_size, g_config.network.timeout_ms);
    LOG_INFO_FMT("  Jitter Buffer: target={}ms, max={}ms",
                 g_config.audio.target_latency_ms, g_config.audio.max_latency_ms);
    LOG_INFO_FMT("  Audio: {}Hz, {}ch, {}bit, device='{}', volume={:.2f}",
                 g_config.audio.sample_rate, g_config.audio.channels, g_config.audio.bits_per_sample,
                 g_config.audio.device_name, g_config.audio.volume);

    // Set up signal handlers for graceful shutdown
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    // Initialize components
    NetworkUDP network;
    JitterBuffer jitterBuffer;
    AudioPlayback audioPlayback;

    if (!InitializeNetworkReceiver(network))
    {
        LOG_ERROR("Failed to initialize network receiver");
        return 1;
    }

    if (!InitializeJitterBuffer(jitterBuffer))
    {
        LOG_ERROR("Failed to initialize jitter buffer");
        return 1;
    }

    if (!InitializeAudioPlayback(audioPlayback))
    {
        LOG_ERROR("Failed to initialize audio playback");
        return 1;
    }

    // Set volume
    audioPlayback.SetVolume(g_config.audio.volume);

    // Start statistics thread
    std::thread statsThread(StatisticsThread, &network, &jitterBuffer, &audioPlayback);

    // Main receiver loop
    ReceiverLoop(network, jitterBuffer, audioPlayback);

    // Cleanup
    LOG_INFO("Cleaning up...");

    // Stop audio playback
    audioPlayback.StopPlayback();

    // Wait for statistics thread to finish
    if (statsThread.joinable())
    {
        statsThread.join();
    }

    // Close network connection
    network.CloseReceiver();
    LOG_INFO("Network connection closed");

    // Final statistics
    auto finalStats = network.GetStats();
    auto finalJitterStats = jitterBuffer.GetStats();
    auto finalPlaybackStats = audioPlayback.GetStats();
    LOG_INFO("=== Final Statistics ===");
    LOG_INFO_FMT("Total frames processed: {}", g_frameCounter.load());
    LOG_INFO_FMT("Network packets received: {}", finalStats.packets_received);
    LOG_INFO_FMT("Network packets lost: {}", finalStats.packets_lost);
    LOG_INFO_FMT("Network bytes received: {}", finalStats.bytes_received);
    LOG_INFO_FMT("Average latency: {:.2f} ms", finalStats.avg_latency_ms);
    LOG_INFO("=== Jitter Buffer Final Stats ===");
    LOG_INFO_FMT("Frames buffered: {}", finalJitterStats.frames_buffered);
    LOG_INFO_FMT("Frames dropped: {}", finalJitterStats.frames_dropped);
    LOG_INFO_FMT("Frames reordered: {}", finalJitterStats.frames_reordered);
    LOG_INFO_FMT("Average jitter: {:.2f} ms", finalJitterStats.avg_jitter_ms);
    LOG_INFO("=== Audio Playback Final Stats ===");
    LOG_INFO_FMT("Frames played: {}", finalPlaybackStats.frames_played);
    LOG_INFO_FMT("Frames dropped: {}", finalPlaybackStats.frames_dropped);
    LOG_INFO_FMT("Buffer underruns: {}", finalPlaybackStats.buffer_underruns);
    LOG_INFO("=======================");

    LOG_INFO("=== AudioSync Receiver with Playback Shutdown Complete ===");
    return 0;
}
