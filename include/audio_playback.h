#pragma once

#include <string>
#include <vector>
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

class AudioPlayback
{
public:
    AudioPlayback();
    ~AudioPlayback();

    // Core interface
    bool InitAudioPlayback(const std::string &deviceName = "");
    bool StartPlayback();
    bool PlayFrame(const std::vector<uint8_t> &frame);
    void StopPlayback();
    void SetVolume(float volume); // 0.0 - 1.0

    // Device management
    std::vector<std::string> GetAvailableDevices() const;
    bool IsPlaying() const { return m_isPlaying; }

    // Statistics
    struct PlaybackStats
    {
        uint32_t frames_played;
        uint32_t frames_dropped;
        float current_volume;
        bool is_playing;
        uint32_t sample_rate;
        uint32_t channels;
        uint32_t bits_per_sample;
        uint32_t buffer_underruns;
    };
    PlaybackStats GetStats() const;
    void ResetStats();

    // Configuration
    void SetSampleRate(uint32_t sample_rate);
    void SetChannels(uint32_t channels);
    void SetBitsPerSample(uint32_t bits_per_sample);
    void SetFrameSize(uint32_t frame_size);

private:
    // WASAPI members
    IMMDeviceEnumerator *m_deviceEnumerator;
    IMMDevice *m_device;
    IAudioClient *m_audioClient;
    IAudioRenderClient *m_renderClient;
    WAVEFORMATEX *m_waveFormat;

    // State
    bool m_isPlaying;
    bool m_isInitialized;
    float m_volume;

    // Audio format
    uint32_t m_sampleRate;
    uint32_t m_channels;
    uint32_t m_bitsPerSample;
    uint32_t m_frameSize;
    uint32_t m_bufferFrameCount;

    // Statistics
    mutable PlaybackStats m_stats;

    // Helper methods
    bool InitializeWASAPI();
    void CleanupWASAPI();
    bool FillBuffer();
    void UpdateStats() const;

    // Thread management
    HANDLE m_playbackThread;
    bool m_threadShouldExit;
    static DWORD WINAPI PlaybackThread(LPVOID param);
    DWORD PlaybackThreadMain();
};
