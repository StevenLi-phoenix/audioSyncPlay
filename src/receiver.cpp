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
void StatisticsThread(NetworkUDP *network)
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

        // Get network statistics
        auto networkStats = network->GetStats();

        // Display statistics
        LOG_INFO("=== Receiver Statistics ===");
        LOG_INFO_FMT("Frame Rate: {:.2f} fps", frameRate);
        LOG_INFO_FMT("Total Frames Received: {}", currentFrameCounter);
        LOG_INFO_FMT("Network - Received: {}, Lost: {}, Avg Latency: {:.2f} ms",
                     networkStats.packets_received, networkStats.packets_lost, networkStats.avg_latency_ms);
        LOG_INFO_FMT("Network - Bytes Received: {}", networkStats.bytes_received);
        LOG_INFO("==========================");

        lastTime = currentTime;
        lastFrameCounter = currentFrameCounter;
    }
}

// Main receiver loop
void ReceiverLoop(NetworkUDP &network)
{
    std::vector<uint8_t> frame;
    uint64_t timestamp;

    LOG_INFO("Starting receiver loop...");

    while (g_running)
    {
        // Try to receive next audio frame
        if (network.ReceiveFrame(frame, timestamp))
        {
            g_frameCounter++;

            // Calculate latency
            auto now = std::chrono::high_resolution_clock::now();
            auto duration = now.time_since_epoch();
            uint64_t currentTime = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            uint64_t latency = currentTime - timestamp;

            // Log first few frames for debugging
            if (g_frameCounter.load() <= 5)
            {
                LOG_INFO_FMT("Received frame {}: {} bytes, latency: {} us",
                             g_frameCounter.load(), frame.size(), latency);
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
            std::cout << "AudioSync Receiver - Real-time audio streaming\n";
            std::cout << "Usage: receiver [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --port <port>      Listen port (default: 8889)\n";
            std::cout << "  --buffer <size>    Network buffer size (default: 65536)\n";
            std::cout << "  --timeout <ms>     Network timeout (default: 1000)\n";
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
        else if (arg == "--stats-interval" && i + 1 < argc)
        {
            g_config.stats_interval_ms = std::stoi(argv[++i]);
        }
    }

    // Initialize logger
    LOG_INFO("=== AudioSync Receiver Starting ===");
    LOG_INFO("Version: 1.0");
    LOG_INFO("Configuration:");
    LOG_INFO_FMT("  Network: port={}, buffer={}, timeout={}ms",
                 g_config.network.receiver_port, g_config.network.buffer_size, g_config.network.timeout_ms);

    // Set up signal handlers for graceful shutdown
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    // Initialize network receiver
    NetworkUDP network;

    if (!InitializeNetworkReceiver(network))
    {
        LOG_ERROR("Failed to initialize network receiver");
        return 1;
    }

    // Start statistics thread
    std::thread statsThread(StatisticsThread, &network);

    // Main receiver loop
    ReceiverLoop(network);

    // Cleanup
    LOG_INFO("Cleaning up...");

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
    LOG_INFO("=== Final Statistics ===");
    LOG_INFO_FMT("Total frames received: {}", g_frameCounter.load());
    LOG_INFO_FMT("Network packets received: {}", finalStats.packets_received);
    LOG_INFO_FMT("Network packets lost: {}", finalStats.packets_lost);
    LOG_INFO_FMT("Network bytes received: {}", finalStats.bytes_received);
    LOG_INFO_FMT("Average latency: {:.2f} ms", finalStats.avg_latency_ms);
    LOG_INFO("=======================");

    LOG_INFO("=== AudioSync Receiver Shutdown Complete ===");
    return 0;
}
