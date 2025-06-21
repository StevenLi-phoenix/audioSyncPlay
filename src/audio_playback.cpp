#include "audio_playback.h"
#include "logger.h"
#include <functiondiscoverykeys_devpkey.h>
#include <algorithm>
#include <thread>

// Placeholder audio playback implementation
// This will be fully implemented in Week 4
AudioPlayback::AudioPlayback()
    : m_deviceEnumerator(nullptr), m_device(nullptr), m_audioClient(nullptr), m_renderClient(nullptr), m_waveFormat(nullptr), m_isPlaying(false), m_isInitialized(false), m_volume(1.0f), m_sampleRate(44100), m_channels(2), m_bitsPerSample(16), m_frameSize(1024), m_bufferFrameCount(0), m_playbackThread(nullptr), m_threadShouldExit(false)
{
    ResetStats();
    LOG_INFO_FMT("Audio playback initialized with format: {}Hz, {}ch, {}bit",
                 m_sampleRate, m_channels, m_bitsPerSample);
}

AudioPlayback::~AudioPlayback()
{
    StopPlayback();
    CleanupWASAPI();
    LOG_INFO("Audio playback destroyed");
}

bool AudioPlayback::InitAudioPlayback(const std::string &deviceName)
{
    LOG_INFO("Initializing audio playback...");

    // Initialize COM for WASAPI
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        LOG_ERROR_FMT("Failed to initialize COM: 0x{:08X}", hr);
        return false;
    }

    // Initialize WASAPI
    if (!InitializeWASAPI())
    {
        LOG_ERROR("Failed to initialize WASAPI");
        return false;
    }

    // Set device if specified
    if (!deviceName.empty())
    {
        auto devices = GetAvailableDevices();
        bool deviceFound = false;
        for (const auto &device : devices)
        {
            if (device.find(deviceName) != std::string::npos)
            {
                LOG_INFO_FMT("Using specified audio device: {}", device);
                deviceFound = true;
                break;
            }
        }
        if (!deviceFound)
        {
            LOG_WARNING_FMT("Specified device '{}' not found, using default", deviceName);
        }
    }

    // Configure audio format to match capture format
    if (m_waveFormat)
    {
        // Use the same format as capture for consistency
        m_waveFormat->wFormatTag = WAVE_FORMAT_PCM;
        m_waveFormat->nChannels = m_channels;
        m_waveFormat->nSamplesPerSec = m_sampleRate;
        m_waveFormat->wBitsPerSample = m_bitsPerSample;
        m_waveFormat->nBlockAlign = (m_waveFormat->nChannels * m_waveFormat->wBitsPerSample) / 8;
        m_waveFormat->nAvgBytesPerSec = m_waveFormat->nSamplesPerSec * m_waveFormat->nBlockAlign;
        m_waveFormat->cbSize = 0;

        LOG_INFO_FMT("Audio playback format: {}Hz, {}ch, {}bit",
                     m_sampleRate, m_channels, m_bitsPerSample);
    }

    m_isInitialized = true;
    LOG_INFO("Audio playback initialized successfully");
    return true;
}

bool AudioPlayback::StartPlayback()
{
    if (!m_isInitialized || !m_audioClient)
    {
        LOG_ERROR("Audio playback not initialized");
        return false;
    }

    if (m_isPlaying)
    {
        LOG_WARNING("Playback already started");
        return true;
    }

    LOG_INFO("Starting audio playback...");

    // Start the audio client
    HRESULT hr = m_audioClient->Start();
    if (FAILED(hr))
    {
        LOG_ERROR_FMT("Failed to start audio client: 0x{:08X}", hr);
        return false;
    }

    m_isPlaying = true;
    m_threadShouldExit = false;

    // Start playback thread
    m_playbackThread = CreateThread(nullptr, 0, PlaybackThread, this, 0, nullptr);
    if (!m_playbackThread)
    {
        LOG_ERROR("Failed to create playback thread");
        m_isPlaying = false;
        return false;
    }

    LOG_INFO("Audio playback started successfully");
    return true;
}

bool AudioPlayback::PlayFrame(const std::vector<uint8_t> &frame)
{
    if (!m_isPlaying || !m_renderClient)
    {
        return false;
    }

    // Get the number of frames available in the buffer
    UINT32 numFramesAvailable;
    HRESULT hr = m_audioClient->GetCurrentPadding(&numFramesAvailable);
    if (FAILED(hr))
    {
        LOG_ERROR_FMT("Failed to get buffer padding: 0x{:08X}", hr);
        return false;
    }

    UINT32 numFramesToWrite = m_bufferFrameCount - numFramesAvailable;
    if (numFramesToWrite == 0)
    {
        // Buffer is full, drop frame but log it
        m_stats.frames_dropped++;
        LOG_DEBUG("Audio buffer full, dropping frame");
        return false;
    }

    // Get buffer
    BYTE *buffer;
    hr = m_renderClient->GetBuffer(numFramesToWrite, &buffer);
    if (FAILED(hr))
    {
        LOG_ERROR_FMT("Failed to get render buffer: 0x{:08X}", hr);
        return false;
    }

    // Calculate frame size in bytes
    size_t frameSizeBytes = m_channels * (m_bitsPerSample / 8);
    size_t framesToCopy = std::min(static_cast<size_t>(numFramesToWrite),
                                   frame.size() / frameSizeBytes);

    // Ensure we have enough data
    if (framesToCopy == 0)
    {
        m_renderClient->ReleaseBuffer(0, 0);
        m_stats.frames_dropped++;
        LOG_DEBUG("Frame too small, dropping");
        return false;
    }

    // Copy audio data with volume adjustment
    if (m_volume != 1.0f)
    {
        // Apply volume scaling
        if (m_bitsPerSample == 16)
        {
            int16_t *src = reinterpret_cast<int16_t *>(const_cast<uint8_t *>(frame.data()));
            int16_t *dst = reinterpret_cast<int16_t *>(buffer);
            for (size_t i = 0; i < framesToCopy * m_channels; ++i)
            {
                dst[i] = static_cast<int16_t>(src[i] * m_volume);
            }
        }
        else if (m_bitsPerSample == 32)
        {
            float *src = reinterpret_cast<float *>(const_cast<uint8_t *>(frame.data()));
            float *dst = reinterpret_cast<float *>(buffer);
            for (size_t i = 0; i < framesToCopy * m_channels; ++i)
            {
                dst[i] = src[i] * m_volume;
            }
        }
    }
    else
    {
        // No volume adjustment needed
        memcpy(buffer, frame.data(), framesToCopy * frameSizeBytes);
    }

    // Release buffer
    hr = m_renderClient->ReleaseBuffer(framesToCopy, 0);
    if (FAILED(hr))
    {
        LOG_ERROR_FMT("Failed to release render buffer: 0x{:08X}", hr);
        return false;
    }

    m_stats.frames_played++;
    UpdateStats();

    return true;
}

void AudioPlayback::StopPlayback()
{
    if (!m_isPlaying)
    {
        return;
    }

    LOG_INFO("Stopping audio playback...");

    // Signal thread to exit
    m_threadShouldExit = true;

    // Wait for thread to finish
    if (m_playbackThread)
    {
        WaitForSingleObject(m_playbackThread, 5000); // 5 second timeout
        CloseHandle(m_playbackThread);
        m_playbackThread = nullptr;
    }

    // Stop audio client
    if (m_audioClient)
    {
        m_audioClient->Stop();
    }

    m_isPlaying = false;
    LOG_INFO("Audio playback stopped");
}

void AudioPlayback::SetVolume(float volume)
{
    m_volume = std::max(0.0f, std::min(1.0f, volume));
    m_stats.current_volume = m_volume;
    LOG_INFO_FMT("Volume set to: {:.2f}", m_volume);
}

std::vector<std::string> AudioPlayback::GetAvailableDevices() const
{
    std::vector<std::string> devices;

    if (!m_deviceEnumerator)
    {
        return devices;
    }

    IMMDeviceCollection *deviceCollection = nullptr;
    HRESULT hr = m_deviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &deviceCollection);
    if (FAILED(hr))
    {
        LOG_ERROR_FMT("Failed to enumerate audio devices: 0x{:08X}", hr);
        return devices;
    }

    UINT deviceCount;
    hr = deviceCollection->GetCount(&deviceCount);
    if (FAILED(hr))
    {
        deviceCollection->Release();
        return devices;
    }

    for (UINT i = 0; i < deviceCount; ++i)
    {
        IMMDevice *device = nullptr;
        hr = deviceCollection->Item(i, &device);
        if (SUCCEEDED(hr))
        {
            // Get device ID as a simple identifier
            LPWSTR deviceId = nullptr;
            hr = device->GetId(&deviceId);
            if (SUCCEEDED(hr) && deviceId)
            {
                // Convert wide string to narrow string
                int size = WideCharToMultiByte(CP_UTF8, 0, deviceId, -1, nullptr, 0, nullptr, nullptr);
                if (size > 0)
                {
                    std::string deviceName(size - 1, 0);
                    WideCharToMultiByte(CP_UTF8, 0, deviceId, -1, &deviceName[0], size, nullptr, nullptr);
                    devices.push_back(deviceName);
                }
                CoTaskMemFree(deviceId);
            }
            device->Release();
        }
    }

    deviceCollection->Release();
    return devices;
}

AudioPlayback::PlaybackStats AudioPlayback::GetStats() const
{
    UpdateStats();
    return m_stats;
}

void AudioPlayback::SetSampleRate(uint32_t sample_rate)
{
    m_sampleRate = sample_rate;
    LOG_INFO_FMT("Sample rate set to: {} Hz", sample_rate);
}

void AudioPlayback::SetChannels(uint32_t channels)
{
    m_channels = channels;
    LOG_INFO_FMT("Channels set to: {}", channels);
}

void AudioPlayback::SetBitsPerSample(uint32_t bits_per_sample)
{
    m_bitsPerSample = bits_per_sample;
    LOG_INFO_FMT("Bits per sample set to: {}", bits_per_sample);
}

void AudioPlayback::SetFrameSize(uint32_t frame_size)
{
    m_frameSize = frame_size;
    LOG_INFO_FMT("Frame size set to: {} samples", frame_size);
}

bool AudioPlayback::InitializeWASAPI()
{
    LOG_INFO("Initializing WASAPI...");

    // Create device enumerator
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  __uuidof(IMMDeviceEnumerator), (void **)&m_deviceEnumerator);
    if (FAILED(hr))
    {
        LOG_ERROR_FMT("Failed to create device enumerator: 0x%08lX", static_cast<unsigned long>(hr));
        return false;
    }

    // Get default audio endpoint
    hr = m_deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_device);
    if (FAILED(hr))
    {
        LOG_ERROR_FMT("Failed to get default audio endpoint: 0x%08lX", static_cast<unsigned long>(hr));
        return false;
    }

    // Activate audio client
    hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void **)&m_audioClient);
    if (FAILED(hr))
    {
        LOG_ERROR_FMT("Failed to activate audio client: 0x%08lX", static_cast<unsigned long>(hr));
        return false;
    }

    // Get mix format
    hr = m_audioClient->GetMixFormat(&m_waveFormat);
    if (FAILED(hr))
    {
        LOG_ERROR_FMT("Failed to get mix format: 0x%08lX", static_cast<unsigned long>(hr));
        return false;
    }

    // Log native format
    LOG_INFO_FMT("Native format - Sample Rate: {}, Channels: {}, Bits: {}", m_waveFormat->nSamplesPerSec, m_waveFormat->nChannels, m_waveFormat->wBitsPerSample);

    // Try to modify format to match our requirements
    WAVEFORMATEX requestedFormat = *m_waveFormat;
    requestedFormat.wFormatTag = WAVE_FORMAT_PCM;
    requestedFormat.nChannels = static_cast<WORD>(m_channels);
    requestedFormat.nSamplesPerSec = m_sampleRate;
    requestedFormat.wBitsPerSample = static_cast<WORD>(m_bitsPerSample);
    requestedFormat.nBlockAlign = (requestedFormat.nChannels * requestedFormat.wBitsPerSample) / 8;
    requestedFormat.nAvgBytesPerSec = requestedFormat.nSamplesPerSec * requestedFormat.nBlockAlign;
    requestedFormat.cbSize = 0;

    // Try to initialize with requested format
    hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 500000, 0, &requestedFormat, nullptr);
    if (FAILED(hr))
    {
        LOG_WARNING_FMT("Format modification failed (0x%08lX), trying native format...", static_cast<unsigned long>(hr));
        // Try native format
        hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 500000, 0, m_waveFormat, nullptr);
        if (FAILED(hr))
        {
            LOG_ERROR_FMT("Failed to initialize audio client with native format: 0x%08lX", static_cast<unsigned long>(hr));
            return false;
        }
        LOG_INFO("Using native format for playback");
    }
    else
    {
        LOG_INFO("Using requested format for playback");
    }

    // Get buffer size
    hr = m_audioClient->GetBufferSize(&m_bufferFrameCount);
    if (FAILED(hr))
    {
        LOG_ERROR_FMT("Failed to get buffer size: 0x%08lX", static_cast<unsigned long>(hr));
        return false;
    }

    // Get render client
    hr = m_audioClient->GetService(__uuidof(IAudioRenderClient), (void **)&m_renderClient);
    if (FAILED(hr))
    {
        LOG_ERROR_FMT("Failed to get render client: 0x%08lX", static_cast<unsigned long>(hr));
        return false;
    }

    LOG_INFO_FMT("WASAPI initialized - Sample Rate: {}, Channels: {}, Bits: {}, Buffer: {} frames",
                 m_waveFormat->nSamplesPerSec, m_waveFormat->nChannels, m_waveFormat->wBitsPerSample, m_bufferFrameCount);
    return true;
}

void AudioPlayback::CleanupWASAPI()
{
    if (m_renderClient)
    {
        m_renderClient->Release();
        m_renderClient = nullptr;
    }

    if (m_audioClient)
    {
        m_audioClient->Release();
        m_audioClient = nullptr;
    }

    if (m_waveFormat)
    {
        CoTaskMemFree(m_waveFormat);
        m_waveFormat = nullptr;
    }

    if (m_device)
    {
        m_device->Release();
        m_device = nullptr;
    }

    if (m_deviceEnumerator)
    {
        m_deviceEnumerator->Release();
        m_deviceEnumerator = nullptr;
    }

    CoUninitialize();
}

void AudioPlayback::UpdateStats() const
{
    m_stats.is_playing = m_isPlaying;
    m_stats.current_volume = m_volume;
    m_stats.sample_rate = m_sampleRate;
    m_stats.channels = m_channels;
    m_stats.bits_per_sample = m_bitsPerSample;
}

void AudioPlayback::ResetStats()
{
    m_stats = PlaybackStats();
    m_stats.current_volume = m_volume;
    m_stats.sample_rate = m_sampleRate;
    m_stats.channels = m_channels;
    m_stats.bits_per_sample = m_bitsPerSample;
}

DWORD WINAPI AudioPlayback::PlaybackThread(LPVOID param)
{
    AudioPlayback *playback = static_cast<AudioPlayback *>(param);
    return playback->PlaybackThreadMain();
}

DWORD AudioPlayback::PlaybackThreadMain()
{
    LOG_INFO("Audio playback thread started");

    while (!m_threadShouldExit)
    {
        // Check for buffer underruns
        UINT32 numFramesAvailable;
        HRESULT hr = m_audioClient->GetCurrentPadding(&numFramesAvailable);
        if (SUCCEEDED(hr) && numFramesAvailable == 0)
        {
            m_stats.buffer_underruns++;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    LOG_INFO("Audio playback thread stopped");
    return 0;
}
