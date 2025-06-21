#include "audio_capture.h"
#include "network_udp.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <memory>

/**
 * @brief Sender class for audio streaming
 *
 * Captures system audio and streams it to receiver devices over UDP.
 */
class Sender
{
public:
    struct SenderConfig
    {
        std::string destinationIP = "127.0.0.1";
        uint16_t destinationPort = 8080;
        std::string audioDevice = "";
        int sampleRate = 44100;
        int channels = 2;
        int bitsPerSample = 16;
        int frameSize = 1024;
        bool enableStatistics = true;
        int statisticsIntervalMs = 1000;
    };

    Sender(const SenderConfig &config) : config_(config), isRunning_(false) {}
    ~Sender() { Stop(); }

    /**
     * @brief Initialize sender components
     * @return true if initialization successful
     */
    bool Initialize()
    {
        std::cout << "Initializing sender...\n";

        // Initialize audio capture
        AudioCapture::AudioFormat format;
        format.sampleRate = config_.sampleRate;
        format.channels = config_.channels;
        format.bitsPerSample = config_.bitsPerSample;
        format.frameSize = config_.frameSize;

        audioCapture_.SetAudioFormat(format);

        if (!audioCapture_.InitAudioCapture(config_.audioDevice))
        {
            std::cerr << "Failed to initialize audio capture\n";
            return false;
        }

        // Initialize network
        NetworkUDP::NetworkConfig netConfig;
        netConfig.remoteIP = config_.destinationIP;
        netConfig.remotePort = config_.destinationPort;
        netConfig.bufferSize = 65536;

        if (!network_.InitUDPSender(config_.destinationIP, config_.destinationPort, netConfig))
        {
            std::cerr << "Failed to initialize network\n";
            return false;
        }

        // Set up audio capture callback
        audioCapture_.SetCallback([this](const std::vector<uint8_t> &frame)
                                  { this->OnAudioFrame(frame); });

        std::cout << "Sender initialized successfully\n";
        return true;
    }

    /**
     * @brief Start audio streaming
     * @return true if started successfully
     */
    bool Start()
    {
        if (isRunning_)
        {
            std::cout << "Sender already running\n";
            return true;
        }

        std::cout << "Starting audio capture...\n";
        if (!audioCapture_.StartCapture())
        {
            std::cerr << "Failed to start audio capture\n";
            return false;
        }

        isRunning_ = true;
        startTime_ = std::chrono::steady_clock::now();

        // Start statistics thread
        if (config_.enableStatistics)
        {
            statsThread_ = std::thread(&Sender::StatisticsThread, this);
        }

        std::cout << "Sender started successfully\n";
        return true;
    }

    /**
     * @brief Stop audio streaming
     */
    void Stop()
    {
        if (!isRunning_)
            return;

        std::cout << "Stopping sender...\n";
        isRunning_ = false;

        audioCapture_.StopCapture();

        if (statsThread_.joinable())
        {
            statsThread_.join();
        }

        std::cout << "Sender stopped\n";
    }

    /**
     * @brief Check if sender is running
     * @return true if running
     */
    bool IsRunning() const { return isRunning_; }

private:
    /**
     * @brief Handle incoming audio frame
     * @param frame Audio frame data
     */
    void OnAudioFrame(const std::vector<uint8_t> &frame)
    {
        if (!isRunning_)
            return;

        uint64_t timestamp = GetCurrentTimestamp();

        // Send frame over network
        if (network_.SendFrame(frame.data(), frame.size(), timestamp))
        {
            framesSent_++;
            bytesSent_ += frame.size();
        }
        else
        {
            framesDropped_++;
        }
    }

    /**
     * @brief Statistics reporting thread
     */
    void StatisticsThread()
    {
        while (isRunning_)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(config_.statisticsIntervalMs));

            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - startTime_).count();

            if (duration > 0)
            {
                auto captureStats = audioCapture_.GetStats();
                auto networkStats = network_.GetStats();

                float sendRate = static_cast<float>(framesSent_) / duration;
                float bitRate = static_cast<float>(bytesSent_ * 8) / duration / 1000.0f; // kbps

                std::cout << "\n=== Sender Statistics ===\n";
                std::cout << "Runtime: " << duration << "s\n";
                std::cout << "Frames sent: " << framesSent_ << " (" << sendRate << " fps)\n";
                std::cout << "Frames dropped: " << framesDropped_ << "\n";
                std::cout << "Bytes sent: " << bytesSent_ << " (" << bitRate << " kbps)\n";
                std::cout << "Network packets sent: " << networkStats.packetsSent << "\n";
                std::cout << "Capture frames: " << captureStats.framesCaptured << "\n";
                std::cout << "Capture latency: " << captureStats.captureLatencyMs << "ms\n";
                std::cout << "========================\n";
            }
        }
    }

    /**
     * @brief Get current timestamp in microseconds
     * @return Current timestamp
     */
    uint64_t GetCurrentTimestamp() const
    {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
        return duration.count();
    }

    // Member variables
    SenderConfig config_;
    AudioCapture audioCapture_;
    NetworkUDP network_;

    std::atomic<bool> isRunning_;
    std::thread statsThread_;
    std::chrono::steady_clock::time_point startTime_;

    // Statistics
    std::atomic<uint64_t> framesSent_{0};
    std::atomic<uint64_t> framesDropped_{0};
    std::atomic<uint64_t> bytesSent_{0};
};

// Global sender instance (for main.cpp)
extern std::unique_ptr<Sender> g_sender;

/**
 * @brief Create and run sender with given arguments
 */
bool RunSender(const std::string &ip, uint16_t port, const std::string &device, int sampleRate)
{
    Sender::SenderConfig config;
    config.destinationIP = ip;
    config.destinationPort = port;
    config.audioDevice = device;
    config.sampleRate = sampleRate;

    auto sender = std::make_unique<Sender>(config);

    if (!sender->Initialize())
    {
        std::cerr << "Failed to initialize sender\n";
        return false;
    }

    if (!sender->Start())
    {
        std::cerr << "Failed to start sender\n";
        return false;
    }

    // Keep sender running
    while (sender->IsRunning())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    sender->Stop();
    return true;
}
