#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

// Windows headers
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>

/**
 * @brief Audio capture class using WASAPI Loopback
 *
 * This class provides functionality to capture system audio using Windows WASAPI
 * Loopback technology. It can capture audio from any application playing through
 * the default audio device.
 */
class AudioCapture
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

    // Capture statistics
    struct CaptureStats
    {
        uint64_t framesCaptured = 0;
        uint64_t framesDropped = 0;
        uint64_t bytesCaptured = 0;
        float captureLatencyMs = 0.0f;
        bool isCapturing = false;
    };

    AudioCapture();
    ~AudioCapture();

    /**
     * @brief Initialize audio capture with specified device
     * @param deviceName Audio device name (empty for default)
     * @return true if initialization successful
     */
    bool InitAudioCapture(const std::string &deviceName = "");

    /**
     * @brief Start audio capture
     * @return true if capture started successfully
     */
    bool StartCapture();

    /**
     * @brief Stop audio capture
     */
    void StopCapture();

    /**
     * @brief Get next audio frame
     * @param frame_out Output buffer for audio frame
     * @return true if frame retrieved successfully
     */
    bool GetNextFrame(std::vector<uint8_t> &frame_out);

    /**
     * @brief Set callback function for audio frames
     * @param callback Function to call when new frame is available
     */
    void SetCallback(std::function<void(const std::vector<uint8_t> &)> callback);

    /**
     * @brief Set audio format
     * @param format Audio format configuration
     */
    void SetAudioFormat(const AudioFormat &format);

    /**
     * @brief Get current capture statistics
     * @return Capture statistics
     */
    CaptureStats GetStats() const;

    /**
     * @brief Get list of available audio devices
     * @return Vector of device names
     */
    static std::vector<std::string> GetAvailableDevices();

private:
    // WASAPI COM objects
    struct WASAPIObjects
    {
        IMMDeviceEnumerator *deviceEnumerator = nullptr;
        IMMDevice *device = nullptr;
        IAudioClient *audioClient = nullptr;
        IAudioCaptureClient *captureClient = nullptr;
        IAudioSessionManager *sessionManager = nullptr;
    };

    // Private methods
    bool InitializeWASAPI();
    bool CreateAudioClient();
    bool StartAudioCapture();
    void CaptureThread();
    void ProcessAudioFrame();
    void CleanupWASAPI();

    // Helper methods
    std::string GetDefaultDeviceName() const;
    bool IsDeviceValid(const std::string &deviceName) const;
    uint64_t GetCurrentTimestamp() const;

    // Member variables
    WASAPIObjects wasapi_;
    AudioFormat format_;
    CaptureStats stats_;

    // Threading
    std::atomic<bool> isCapturing_;
    std::thread captureThread_;
    std::mutex frameMutex_;
    std::condition_variable frameCondition_;

    // Frame buffer
    std::vector<uint8_t> frameBuffer_;
    std::function<void(const std::vector<uint8_t> &)> frameCallback_;

    // COM initialization
    bool comInitialized_;

    // Constants
    static constexpr size_t MAX_FRAME_SIZE = 8192;
    static constexpr size_t BUFFER_FRAMES = 3;
};
