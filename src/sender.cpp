#include "audio_capture.h"
#include "network_udp.h"
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
        LOG_INFO("Received shutdown signal, stopping sender...");
        g_running = false;
    }
}

// Statistics display thread
void StatisticsThread(AudioCapture *audioCapture, NetworkUDP *network)
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

        // Get statistics
        auto audioStats = audioCapture->GetStats();
        auto networkStats = network->GetStats();

        // Display statistics
        LOG_INFO("=== Sender Statistics ===");
        LOG_INFO_FMT("Frame Rate: {:.2f} fps", frameRate);
        LOG_INFO_FMT("Total Frames Sent: {}", currentFrameCounter);
        LOG_INFO_FMT("Audio - Captured: {}, Dropped: {}",
                     audioStats.frames_captured, audioStats.frames_dropped);
        LOG_INFO_FMT("Network - Sent: {}, Lost: {}, Avg Latency: {:.2f} ms",
                     networkStats.packets_sent, networkStats.packets_lost, networkStats.avg_latency_ms);
        LOG_INFO_FMT("Network - Bytes Sent: {}, Received: {}",
                     networkStats.bytes_sent, networkStats.bytes_received);
        LOG_INFO("========================");

        lastTime = currentTime;
        lastFrameCounter = currentFrameCounter;
    }
}

// Audio frame callback for real-time transmission
void AudioFrameCallback(const std::vector<uint8_t> &frame, NetworkUDP *network)
{
    if (!g_running || !network)
        return;

    // Get current timestamp in microseconds
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();

    // Generate sequence number for this frame
    uint32_t sequenceNumber = g_sequenceCounter++;

    // Get current audio format
    int sampleRate, channels, bitsPerSample;
    // Note: We need to get this from the audio capture instance
    // For now, use config values
    sampleRate = g_config.audio.sample_rate;
    channels = g_config.audio.channels;
    bitsPerSample = g_config.audio.bits_per_sample;

    // Create packet with format info, sequence number, timestamp, and audio data
    std::vector<uint8_t> packet;
    size_t headerSize = sizeof(uint32_t) + sizeof(uint64_t) + sizeof(int) * 3; // seq + timestamp + format
    packet.resize(headerSize + frame.size());

    // Add sequence number (4 bytes)
    memcpy(packet.data(), &sequenceNumber, sizeof(uint32_t));

    // Add timestamp (8 bytes)
    memcpy(packet.data() + sizeof(uint32_t), &timestamp, sizeof(uint64_t));

    // Add audio format info (12 bytes: sampleRate, channels, bitsPerSample)
    memcpy(packet.data() + sizeof(uint32_t) + sizeof(uint64_t), &sampleRate, sizeof(int));
    memcpy(packet.data() + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(int), &channels, sizeof(int));
    memcpy(packet.data() + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(int) * 2, &bitsPerSample, sizeof(int));

    // Add audio frame data
    memcpy(packet.data() + headerSize, frame.data(), frame.size());

    // Send packet over network
    if (network->SendFrame(packet.data(), packet.size(), timestamp))
    {
        g_frameCounter++;
    }
    else
    {
        LOG_WARNING("Failed to send audio frame");
    }
}

// Main sender loop
void SenderLoop(AudioCapture &audioCapture, NetworkUDP &network)
{
    std::vector<uint8_t> frame;

    LOG_INFO("Starting sender loop...");

    while (g_running)
    {
        // Try to get next audio frame
        if (audioCapture.GetNextFrame(frame))
        {
            // Get current timestamp
            auto now = std::chrono::high_resolution_clock::now();
            auto duration = now.time_since_epoch();
            uint64_t timestamp = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();

            // Generate sequence number for this frame
            uint32_t sequenceNumber = g_sequenceCounter++;

            // Create packet with sequence number and timestamp
            std::vector<uint8_t> packet;
            packet.resize(sizeof(uint32_t) + sizeof(uint64_t) + frame.size());

            // Add sequence number (4 bytes)
            memcpy(packet.data(), &sequenceNumber, sizeof(uint32_t));

            // Add timestamp (8 bytes)
            memcpy(packet.data() + sizeof(uint32_t), &timestamp, sizeof(uint64_t));

            // Add audio frame data
            memcpy(packet.data() + sizeof(uint32_t) + sizeof(uint64_t), frame.data(), frame.size());

            // Send packet over network
            if (network.SendFrame(packet.data(), packet.size(), timestamp))
            {
                g_frameCounter++;
            }
            else
            {
                LOG_WARNING("Failed to send audio frame");
            }
        }
        else
        {
            // No frame available, small sleep to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    LOG_INFO("Sender loop stopped");
}

// Initialize audio capture with device selection
bool InitializeAudioCapture(AudioCapture &audioCapture)
{
    LOG_INFO("Initializing audio capture...");

    // Get available audio devices
    auto devices = audioCapture.GetAvailableDevices();
    LOG_INFO("Available audio devices:");
    for (size_t i = 0; i < devices.size(); ++i)
    {
        LOG_INFO_FMT("  {}: {}", i, devices[i]);
    }

    // Try to initialize with default device or first available
    std::string deviceName = g_config.audio.default_audio_device;
    if (deviceName.empty() && !devices.empty())
    {
        deviceName = devices[0];
        LOG_INFO_FMT("Using default audio device: {}", deviceName);
    }

    if (!audioCapture.InitAudioCapture(deviceName))
    {
        LOG_ERROR("Failed to initialize audio capture");
        return false;
    }

    LOG_INFO("Audio capture initialized successfully");
    return true;
}

// Initialize network sender
bool InitializeNetworkSender(NetworkUDP &network)
{
    LOG_INFO("Initializing network sender...");
    LOG_INFO_FMT("Target: {}:{}", g_config.network.sender_ip, g_config.network.sender_port);

    // Set network configuration
    network.SetBufferSize(g_config.network.buffer_size);
    network.SetTimeout(g_config.network.timeout_ms);

    if (!network.InitUDPSender(g_config.network.sender_ip, g_config.network.sender_port))
    {
        LOG_ERROR("Failed to initialize UDP sender");
        return false;
    }

    LOG_INFO("Network sender initialized successfully");
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
            std::cout << "AudioSync Sender - Real-time audio streaming\n";
            std::cout << "Usage: sender [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --ip <ip>          Target IP address (default: 127.0.0.1)\n";
            std::cout << "  --port <port>      Target port (default: 8888)\n";
            std::cout << "  --device <name>    Audio device name\n";
            std::cout << "  --buffer <size>    Network buffer size (default: 65536)\n";
            std::cout << "  --timeout <ms>     Network timeout (default: 1000)\n";
            std::cout << "  --stats-interval <ms> Statistics display interval (default: 1000)\n";
            std::cout << "  --help, -h         Show this help message\n";
            return 0;
        }
        else if (arg == "--ip" && i + 1 < argc)
        {
            g_config.network.sender_ip = argv[++i];
        }
        else if (arg == "--port" && i + 1 < argc)
        {
            g_config.network.sender_port = static_cast<uint16_t>(std::stoi(argv[++i]));
        }
        else if (arg == "--device" && i + 1 < argc)
        {
            g_config.audio.default_audio_device = argv[++i];
        }
        else if (arg == "--buffer" && i + 1 < argc)
        {
            g_config.network.buffer_size = std::stoi(argv[++i]);
        }
        else if (arg == "--timeout" && i + 1 < argc)
        {
            g_config.network.timeout_ms = std::stoi(argv[++i]);
        }
        else if (arg == "--stats-interval" && i + 1 < argc)
        {
            g_config.stats_interval_ms = std::stoi(argv[++i]);
        }
    }

    // Initialize logger
    LOG_INFO("=== AudioSync Sender Starting ===");
    LOG_INFO("Version: 1.0");
    LOG_INFO("Configuration:");
    LOG_INFO_FMT("  Audio: {}Hz, {} channels, {} bits, {} samples/frame",
                 g_config.audio.sample_rate, g_config.audio.channels,
                 g_config.audio.bits_per_sample, g_config.audio.frame_size);
    LOG_INFO_FMT("  Network: {}:{}, buffer={}, timeout={}ms",
                 g_config.network.sender_ip, g_config.network.sender_port,
                 g_config.network.buffer_size, g_config.network.timeout_ms);

    // Set up signal handlers for graceful shutdown
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    // Initialize components
    AudioCapture audioCapture;
    NetworkUDP network;

    if (!InitializeAudioCapture(audioCapture))
    {
        LOG_ERROR("Failed to initialize audio capture");
        return 1;
    }

    if (!InitializeNetworkSender(network))
    {
        LOG_ERROR("Failed to initialize network sender");
        return 1;
    }

    // Set up audio frame callback for real-time transmission
    audioCapture.SetCallback([&network](const std::vector<uint8_t> &frame)
                             { AudioFrameCallback(frame, &network); });

    // Start audio capture
    if (!audioCapture.StartCapture())
    {
        LOG_ERROR("Failed to start audio capture");
        return 1;
    }

    LOG_INFO("Audio capture started successfully");

    // Start statistics thread
    std::thread statsThread(StatisticsThread, &audioCapture, &network);

    // Main sender loop
    SenderLoop(audioCapture, network);

    // Cleanup
    LOG_INFO("Cleaning up...");

    // Stop audio capture
    audioCapture.StopCapture();
    LOG_INFO("Audio capture stopped");

    // Wait for statistics thread to finish
    if (statsThread.joinable())
    {
        statsThread.join();
    }

    // Close network connection
    network.CloseSender();
    LOG_INFO("Network connection closed");

    // Final statistics
    auto finalStats = network.GetStats();
    LOG_INFO("=== Final Statistics ===");
    LOG_INFO_FMT("Total frames sent: {}", g_frameCounter.load());
    LOG_INFO_FMT("Network packets sent: {}", finalStats.packets_sent);
    LOG_INFO_FMT("Network packets lost: {}", finalStats.packets_lost);
    LOG_INFO_FMT("Network bytes sent: {}", finalStats.bytes_sent);
    LOG_INFO_FMT("Average latency: {:.2f} ms", finalStats.avg_latency_ms);
    LOG_INFO("=======================");

    LOG_INFO("=== AudioSync Sender Shutdown Complete ===");
    return 0;
}
