#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <functional>

// Windows headers
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>

/**
 * @brief Audio playback class using WASAPI
 *
 * This class provides functionality to play audio through Windows WASAPI
 * with support for real-time streaming and volume control.
 */
class AudioPlayback
{
public:
    // Audio format configuration
    struct AudioFormat
    {
        int sampleRate = 44100;
        int channels = 2;
        int bitsPerSample = 16;
        int frameSize = 1024; // samples per frame
    };

    // Playback statistics
    struct PlaybackStats
    {
        uint32_t framesPlayed = 0;
        uint32_t framesDropped = 0;
        uint32_t framesUnderrun = 0;
        uint64_t bytesPlayed = 0;
        float currentVolume = 1.0f;
        bool isPlaying = false;
        float bufferLevelPercent = 0.0f;
        float playbackLatencyMs = 0.0f;
    };

    // Configuration
    struct PlaybackConfig
    {
        AudioFormat format;
        int bufferSizeMs = 100;
        int maxLatencyMs = 200;
        bool enableVolumeControl = true;
        bool enableStatistics = true;
        std::string deviceName = "";
    };

    AudioPlayback();
    ~AudioPlayback();

    /**
     * @brief Initialize audio playback with specified device
     * @param deviceName Audio device name (empty for default)
     * @param config Playback configuration
     * @return true if initialization successful
     */
    bool InitAudioPlayback(const std::string &deviceName = "",
                           const PlaybackConfig &config = PlaybackConfig());

    /**
     * @brief Start audio playback
     * @return true if playback started successfully
     */
    bool StartPlayback();

    /**
     * @brief Stop audio playback
     */
    void StopPlayback();

    /**
     * @brief Play audio frame
     * @param frame Audio frame data
     * @return true if frame queued successfully
     */
    bool PlayFrame(const std::vector<uint8_t> &frame);

    /**
     * @brief Set playback volume
     * @param volume Volume level (0.0 - 1.0)
     */
    void SetVolume(float volume);

    /**
     * @brief Get current playback statistics
     * @return Playback statistics
     */
    PlaybackStats GetStats() const;

    /**
     * @brief Set playback configuration
     * @param config Playback configuration
     */
    void SetConfig(const PlaybackConfig &config);

    /**
     * @brief Check if playback is active
     * @return true if playing
     */
    bool IsPlaying() const;

    /**
     * @brief Get list of available audio devices
     * @return Vector of device names
     */
    static std::vector<std::string> GetAvailableDevices();

    /**
     * @brief Flush playback buffer
     */
    void FlushBuffer();

    /**
     * @brief Pause playback
     */
    void Pause();

    /**
     * @brief Resume playback
     */
    void Resume();

private:
    // WASAPI COM objects
    struct WASAPIObjects
    {
        IMMDeviceEnumerator *deviceEnumerator = nullptr;
        IMMDevice *device = nullptr;
        IAudioClient *audioClient = nullptr;
        IAudioRenderClient *renderClient = nullptr;
        IAudioSessionControl *sessionControl = nullptr;
        ISimpleAudioVolume *audioVolume = nullptr;
    };

    // Private methods
    bool InitializeWASAPI();
    bool CreateAudioClient();
    bool StartAudioPlayback();
    void PlaybackThread();
    void ProcessAudioBuffer();
    void CleanupWASAPI();

    // Buffer management
    bool QueueFrame(const std::vector<uint8_t> &frame);
    bool WriteToBuffer(const uint8_t *data, size_t size);
    void UpdateBufferLevel();

    // Helper methods
    std::string GetDefaultDeviceName() const;
    bool IsDeviceValid(const std::string &deviceName) const;
    uint64_t GetCurrentTimestamp() const;
    void ApplyVolume(std::vector<uint8_t> &frame);

    // Member variables
    WASAPIObjects wasapi_;
    PlaybackConfig config_;
    PlaybackStats stats_;

    // Threading
    std::atomic<bool> isPlaying_;
    std::thread playbackThread_;
    std::mutex bufferMutex_;
    std::condition_variable bufferCondition_;

    // Audio buffer
    std::queue<std::vector<uint8_t>> frameQueue_;
    std::vector<uint8_t> tempBuffer_;

    // COM initialization
    bool comInitialized_;

    // Timing
    uint64_t lastPlaybackTime_;
    uint64_t frameDuration_;

    // Constants
    static constexpr size_t MAX_QUEUE_SIZE = 50;
    static constexpr size_t MAX_FRAME_SIZE = 8192;
    static constexpr int BUFFER_UNDERRUN_THRESHOLD = 3;
};
