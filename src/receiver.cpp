#include "network_udp.h"
#include "jitter_buffer.h"
#include "audio_playback.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <memory>

/**
 * @brief Receiver class for audio streaming
 *
 * Receives audio data over UDP and plays it through audio output with jitter buffering.
 */
class Receiver
{
public:
    struct ReceiverConfig
    {
        uint16_t listenPort = 8080;
        std::string audioDevice = "";
        int targetLatencyMs = 100;
        int maxLatencyMs = 200;
        float volume = 1.0f;
        int sampleRate = 44100;
        int channels = 2;
        int bitsPerSample = 16;
        int frameSize = 1024;
        bool enableStatistics = true;
        int statisticsIntervalMs = 1000;
        bool enableAdaptiveLatency = true;
    };

    Receiver(const ReceiverConfig &config) : config_(config), isRunning_(false) {}
    ~Receiver() { Stop(); }

    /**
     * @brief Initialize receiver components
     * @return true if initialization successful
     */
    bool Initialize()
    {
        std::cout << "Initializing receiver...\n";

        // Initialize network
        NetworkUDP::NetworkConfig netConfig;
        netConfig.localPort = config_.listenPort;
        netConfig.bufferSize = 65536;

        if (!network_.InitUDPReceiver(config_.listenPort, netConfig))
        {
            std::cerr << "Failed to initialize network\n";
            return false;
        }

        // Initialize jitter buffer
        JitterBuffer::BufferConfig bufferConfig;
        bufferConfig.targetLatencyMs = config_.targetLatencyMs;
        bufferConfig.maxLatencyMs = config_.maxLatencyMs;
        bufferConfig.enableAdaptiveLatency = config_.enableAdaptiveLatency;

        jitterBuffer_.SetConfig(bufferConfig);

        // Initialize audio playback
        AudioPlayback::PlaybackConfig playbackConfig;
        playbackConfig.format.sampleRate = config_.sampleRate;
        playbackConfig.format.channels = config_.channels;
        playbackConfig.format.bitsPerSample = config_.bitsPerSample;
        playbackConfig.format.frameSize = config_.frameSize;
        playbackConfig.bufferSizeMs = config_.targetLatencyMs;
        playbackConfig.deviceName = config_.audioDevice;

        if (!audioPlayback_.InitAudioPlayback(config_.audioDevice, playbackConfig))
        {
            std::cerr << "Failed to initialize audio playback\n";
            return false;
        }

        // Set volume
        audioPlayback_.SetVolume(config_.volume);

        std::cout << "Receiver initialized successfully\n";
        return true;
    }

    /**
     * @brief Start audio reception and playback
     * @return true if started successfully
     */
    bool Start()
    {
        if (isRunning_)
        {
            std::cout << "Receiver already running\n";
            return true;
        }

        std::cout << "Starting audio playback...\n";
        if (!audioPlayback_.StartPlayback())
        {
            std::cerr << "Failed to start audio playback\n";
            return false;
        }

        isRunning_ = true;
        startTime_ = std::chrono::steady_clock::now();

        // Start network receive thread
        receiveThread_ = std::thread(&Receiver::ReceiveThread, this);

        // Start playback thread
        playbackThread_ = std::thread(&Receiver::PlaybackThread, this);

        // Start statistics thread
        if (config_.enableStatistics)
        {
            statsThread_ = std::thread(&Receiver::StatisticsThread, this);
        }

        std::cout << "Receiver started successfully\n";
        return true;
    }

    /**
     * @brief Stop audio reception and playback
     */
    void Stop()
    {
        if (!isRunning_)
            return;

        std::cout << "Stopping receiver...\n";
        isRunning_ = false;

        audioPlayback_.StopPlayback();

        if (receiveThread_.joinable())
        {
            receiveThread_.join();
        }

        if (playbackThread_.joinable())
        {
            playbackThread_.join();
        }

        if (statsThread_.joinable())
        {
            statsThread_.join();
        }

        std::cout << "Receiver stopped\n";
    }

    /**
     * @brief Check if receiver is running
     * @return true if running
     */
    bool IsRunning() const { return isRunning_; }

private:
    /**
     * @brief Network receive thread
     */
    void ReceiveThread()
    {
        std::vector<uint8_t> frame;
        uint64_t timestamp;

        while (isRunning_)
        {
            if (network_.ReceiveFrame(frame, timestamp))
            {
                // Push frame to jitter buffer
                jitterBuffer_.PushFrame(frame, timestamp);
                framesReceived_++;
                bytesReceived_ += frame.size();
            }
            else
            {
                // No frame received, small delay
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    /**
     * @brief Audio playback thread
     */
    void PlaybackThread()
    {
        std::vector<uint8_t> frame;

        while (isRunning_)
        {
            if (jitterBuffer_.GetFrame(frame))
            {
                if (audioPlayback_.PlayFrame(frame))
                {
                    framesPlayed_++;
                    bytesPlayed_ += frame.size();
                }
                else
                {
                    framesDropped_++;
                }
            }
            else
            {
                // No frame available, small delay
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
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
                auto networkStats = network_.GetStats();
                auto bufferStats = jitterBuffer_.GetStats();
                auto playbackStats = audioPlayback_.GetStats();

                float receiveRate = static_cast<float>(framesReceived_) / duration;
                float playRate = static_cast<float>(framesPlayed_) / duration;
                float bitRate = static_cast<float>(bytesReceived_ * 8) / duration / 1000.0f; // kbps

                std::cout << "\n=== Receiver Statistics ===\n";
                std::cout << "Runtime: " << duration << "s\n";
                std::cout << "Frames received: " << framesReceived_ << " (" << receiveRate << " fps)\n";
                std::cout << "Frames played: " << framesPlayed_ << " (" << playRate << " fps)\n";
                std::cout << "Frames dropped: " << framesDropped_ << "\n";
                std::cout << "Bytes received: " << bytesReceived_ << " (" << bitRate << " kbps)\n";
                std::cout << "Network packets: " << networkStats.packetsReceived << "\n";
                std::cout << "Network lost: " << networkStats.packetsLost << "\n";
                std::cout << "Buffer latency: " << bufferStats.currentLatencyMs << "ms\n";
                std::cout << "Buffer occupancy: " << bufferStats.bufferOccupancyPercent << "%\n";
                std::cout << "Playback volume: " << playbackStats.currentVolume << "\n";
                std::cout << "==========================\n";
            }
        }
    }

    // Member variables
    ReceiverConfig config_;
    NetworkUDP network_;
    JitterBuffer jitterBuffer_;
    AudioPlayback audioPlayback_;

    std::atomic<bool> isRunning_;
    std::thread receiveThread_;
    std::thread playbackThread_;
    std::thread statsThread_;
    std::chrono::steady_clock::time_point startTime_;

    // Statistics
    std::atomic<uint64_t> framesReceived_{0};
    std::atomic<uint64_t> framesPlayed_{0};
    std::atomic<uint64_t> framesDropped_{0};
    std::atomic<uint64_t> bytesReceived_{0};
    std::atomic<uint64_t> bytesPlayed_{0};
};

// Global receiver instance (for main.cpp)
extern std::unique_ptr<Receiver> g_receiver;

/**
 * @brief Create and run receiver with given arguments
 */
bool RunReceiver(uint16_t port, const std::string &device, int latency, float volume)
{
    Receiver::ReceiverConfig config;
    config.listenPort = port;
    config.audioDevice = device;
    config.targetLatencyMs = latency;
    config.volume = volume;

    auto receiver = std::make_unique<Receiver>(config);

    if (!receiver->Initialize())
    {
        std::cerr << "Failed to initialize receiver\n";
        return false;
    }

    if (!receiver->Start())
    {
        std::cerr << "Failed to start receiver\n";
        return false;
    }

    // Keep receiver running
    while (receiver->IsRunning())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    receiver->Stop();
    return true;
}
