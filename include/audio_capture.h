#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

class AudioCapture
{
public:
    AudioCapture();
    ~AudioCapture();

    // Core interface
    bool InitAudioCapture(const std::string &deviceName = "");
    bool StartCapture();
    void StopCapture();
    bool GetNextFrame(std::vector<uint8_t> &frame_out);
    void SetCallback(std::function<void(const std::vector<uint8_t> &)> callback);

    // Device management
    std::vector<std::string> GetAvailableDevices() const;
    bool IsCapturing() const { return m_isCapturing; }

    // Statistics
    struct CaptureStats
    {
        uint32_t frames_captured;
        uint32_t frames_dropped;
        uint32_t sample_rate;
        uint32_t channels;
        uint32_t bits_per_sample;
        uint32_t frame_size;
    };
    CaptureStats GetStats() const;

private:
    // WASAPI members
    IMMDeviceEnumerator *m_deviceEnumerator;
    IMMDevice *m_device;
    IAudioClient *m_audioClient;
    IAudioCaptureClient *m_captureClient;
    WAVEFORMATEX *m_waveFormat;

    // State
    bool m_isCapturing;
    bool m_isInitialized;
    std::function<void(const std::vector<uint8_t> &)> m_callback;

    // Statistics
    mutable CaptureStats m_stats;

    // Helper methods
    bool InitializeWASAPI();
    void CleanupWASAPI();
    static DWORD WINAPI CaptureThread(LPVOID param);
    DWORD CaptureThreadMain();

    // Thread management
    HANDLE m_captureThread;
    bool m_threadShouldExit;
};
