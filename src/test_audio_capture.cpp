#include "audio_capture.h"
#include "logger.h"
#include <iostream>
#include <chrono>
#include <thread>

int main()
{
    // Initialize logger
    Logger::Instance().SetLogLevel(LogLevel::INFO);
    Logger::Instance().SetLogFile("test_audio_capture.log");
    Logger::Instance().EnableConsole(true);

    // LOG_INFO("=== Audio Capture Test ===");
    std::cout << "=== Audio Capture Test ===" << std::endl;

    // Create audio capture instance
    AudioCapture capture;

    // List available devices
    // LOG_INFO("Available audio devices:");
    std::cout << "Available audio devices:" << std::endl;
    auto devices = capture.GetAvailableDevices();
    for (size_t i = 0; i < devices.size(); i++)
    {
        // LOG_INFO_FMT("  %zu: %s", i + 1, devices[i].c_str());
        std::cout << "  " << i + 1 << ": " << devices[i] << std::endl;
    }

    // Initialize audio capture
    if (!capture.InitAudioCapture())
    {
        LOG_ERROR("Failed to initialize audio capture");
        return -1;
    }

    // Set up callback to count frames
    int frameCount = 0;
    auto startTime = std::chrono::steady_clock::now();

    capture.SetCallback([&frameCount, &startTime](const std::vector<uint8_t> &frame)
                        {
        frameCount++;

        // Log every 100 frames
        if (frameCount % 100 == 0) {
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
            if (duration > 0) {
                float fps = static_cast<float>(frameCount) / duration;
                // LOG_INFO_FMT("Captured %d frames (%.1f fps), frame size: %zu bytes",
                //             frameCount, fps, frame.size());
                std::cout << "Captured " << frameCount << " frames (" << fps << " fps), frame size: " << frame.size() << " bytes" << std::endl;
            }
        } });

    // Start capture
    if (!capture.StartCapture())
    {
        LOG_ERROR("Failed to start audio capture");
        return -1;
    }

    // LOG_INFO("Audio capture started. Press Enter to stop...");
    std::cout << "Audio capture started. Capturing for 10 seconds..." << std::endl;

    // Run for 10 seconds
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // Stop capture
    capture.StopCapture();

    // Get final statistics
    auto stats = capture.GetStats();
    auto totalTime = std::chrono::duration_cast<std::chrono::seconds>(
                         std::chrono::steady_clock::now() - startTime)
                         .count();

    // LOG_INFO("=== Test Results ===");
    std::cout << "=== Test Results ===" << std::endl;
    // LOG_INFO_FMT("Total frames captured: %d", stats.frames_captured);
    std::cout << "Total frames captured: " << stats.frames_captured << std::endl;
    // LOG_INFO_FMT("Frames dropped: %d", stats.frames_dropped);
    std::cout << "Frames dropped: " << stats.frames_dropped << std::endl;
    // LOG_INFO_FMT("Sample rate: %d Hz", stats.sample_rate);
    std::cout << "Sample rate: " << stats.sample_rate << " Hz" << std::endl;
    // LOG_INFO_FMT("Channels: %d", stats.channels);
    std::cout << "Channels: " << stats.channels << std::endl;
    // LOG_INFO_FMT("Bits per sample: %d", stats.bits_per_sample);
    std::cout << "Bits per sample: " << stats.bits_per_sample << std::endl;
    // LOG_INFO_FMT("Frame size: %d samples", stats.frame_size);
    std::cout << "Frame size: " << stats.frame_size << " samples" << std::endl;
    // LOG_INFO_FMT("Test duration: %lld seconds", totalTime);
    std::cout << "Test duration: " << totalTime << " seconds" << std::endl;

    if (stats.frames_captured > 0 && totalTime > 0)
    {
        float avgFps = static_cast<float>(stats.frames_captured) / totalTime;
        // LOG_INFO_FMT("Average capture rate: %.1f fps", avgFps);
        std::cout << "Average capture rate: " << avgFps << " fps" << std::endl;

        if (avgFps > 40.0f)
        { // Should be around 43.1 fps for 1024 samples at 44.1kHz
            // LOG_INFO("✓ Audio capture performance is good");
            std::cout << "✓ Audio capture performance is good" << std::endl;
        }
        else
        {
            // LOG_WARNING("⚠ Audio capture performance is below expected");
            std::cout << "⚠ Audio capture performance is below expected" << std::endl;
        }
    }
    else
    {
        // LOG_ERROR("✗ No frames were captured");
        std::cout << "✗ No frames were captured" << std::endl;
    }

    // LOG_INFO("Test completed");
    std::cout << "Test completed" << std::endl;
    return 0;
}
