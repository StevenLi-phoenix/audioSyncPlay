#define INITGUID
#include "audio_capture.h"
#include "logger.h"
#include <iostream>
#include <algorithm>
#include <functiondiscoverykeys_devpkey.h>

AudioCapture::AudioCapture()
    : m_deviceEnumerator(nullptr), m_device(nullptr), m_audioClient(nullptr), m_captureClient(nullptr), m_waveFormat(nullptr), m_isCapturing(false), m_isInitialized(false), m_captureThread(nullptr), m_threadShouldExit(false)
{
    memset(&m_stats, 0, sizeof(m_stats));
    m_stats.sample_rate = 44100;
    m_stats.channels = 2;
    m_stats.bits_per_sample = 16;
    m_stats.frame_size = 1024;
}

AudioCapture::~AudioCapture()
{
    StopCapture();
    CleanupWASAPI();
}

bool AudioCapture::InitAudioCapture(const std::string &deviceName)
{
    LOG_INFO("Initializing audio capture...");

    if (m_isInitialized)
    {
        LOG_WARNING("Audio capture already initialized");
        return true;
    }

    if (!InitializeWASAPI())
    {
        LOG_ERROR("Failed to initialize WASAPI");
        return false;
    }

    m_isInitialized = true;
    LOG_INFO("Audio capture initialized successfully");
    return true;
}

bool AudioCapture::InitializeWASAPI()
{
    HRESULT hr;
    std::cout << "[DEBUG] CoInitializeEx..." << std::endl;
    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    std::cout << "[DEBUG] CoInitializeEx HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
    if (FAILED(hr))
    {
        LOG_ERROR_FMT("CoInitializeEx failed: 0x%08X", hr);
        std::cout << "[ERROR] CoInitializeEx failed: 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }

    // Create device enumerator
    std::cout << "[DEBUG] CoCreateInstance..." << std::endl;
    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        (void **)&m_deviceEnumerator);
    std::cout << "[DEBUG] CoCreateInstance HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
    if (FAILED(hr))
    {
        LOG_ERROR_FMT("Failed to create device enumerator: 0x%08X", hr);
        std::cout << "[ERROR] Failed to create device enumerator: 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }

    // Get default render device (for loopback capture)
    std::cout << "[DEBUG] GetDefaultAudioEndpoint..." << std::endl;
    hr = m_deviceEnumerator->GetDefaultAudioEndpoint(
        eRender,
        eConsole,
        &m_device);
    std::cout << "[DEBUG] GetDefaultAudioEndpoint HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
    if (FAILED(hr))
    {
        LOG_ERROR_FMT("Failed to get default audio endpoint: 0x%08X", hr);
        std::cout << "[ERROR] Failed to get default audio endpoint: 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }

    // Create audio client
    std::cout << "[DEBUG] Activate IAudioClient..." << std::endl;
    hr = m_device->Activate(
        __uuidof(IAudioClient),
        CLSCTX_ALL,
        nullptr,
        (void **)&m_audioClient);
    std::cout << "[DEBUG] Activate IAudioClient HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
    if (FAILED(hr))
    {
        LOG_ERROR_FMT("Failed to activate audio client: 0x%08X", hr);
        std::cout << "[ERROR] Failed to activate audio client: 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }

    // Get mix format
    std::cout << "[DEBUG] GetMixFormat..." << std::endl;
    hr = m_audioClient->GetMixFormat(&m_waveFormat);
    std::cout << "[DEBUG] GetMixFormat HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
    if (FAILED(hr))
    {
        LOG_ERROR_FMT("Failed to get mix format: 0x%08X", hr);
        std::cout << "[ERROR] Failed to get mix format: 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }

    // Log the native format
    std::cout << "[DEBUG] Native format - Sample Rate: " << m_waveFormat->nSamplesPerSec
              << ", Channels: " << m_waveFormat->nChannels
              << ", Bits: " << m_waveFormat->wBitsPerSample << std::endl;

    // Try to use the native format first, only modify if necessary
    bool formatModified = false;

    // Check if we need to modify the format
    if (m_waveFormat->wFormatTag != WAVE_FORMAT_PCM ||
        m_waveFormat->nChannels != m_stats.channels ||
        m_waveFormat->nSamplesPerSec != m_stats.sample_rate ||
        m_waveFormat->wBitsPerSample != m_stats.bits_per_sample)
    {

        formatModified = true;
        std::cout << "[DEBUG] Modifying format to match requirements..." << std::endl;

        // Set our desired format
        m_waveFormat->wFormatTag = WAVE_FORMAT_PCM;
        m_waveFormat->nChannels = m_stats.channels;
        m_waveFormat->nSamplesPerSec = m_stats.sample_rate;
        m_waveFormat->wBitsPerSample = m_stats.bits_per_sample;
        m_waveFormat->nBlockAlign = (m_waveFormat->nChannels * m_waveFormat->wBitsPerSample) / 8;
        m_waveFormat->nAvgBytesPerSec = m_waveFormat->nSamplesPerSec * m_waveFormat->nBlockAlign;
        m_waveFormat->cbSize = 0;
    }
    else
    {
        // Update our stats to match the native format
        m_stats.sample_rate = m_waveFormat->nSamplesPerSec;
        m_stats.channels = m_waveFormat->nChannels;
        m_stats.bits_per_sample = m_waveFormat->wBitsPerSample;
        m_stats.frame_size = 1024; // Keep our frame size
    }

    std::cout << "[DEBUG] Initialize audio client..." << std::endl;
    hr = m_audioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK,
        10000000, // 1 second buffer
        0,
        m_waveFormat,
        nullptr);
    std::cout << "[DEBUG] Initialize audio client HRESULT: 0x" << std::hex << hr << std::dec << std::endl;

    // If format modification failed, try with native format
    if (FAILED(hr) && formatModified)
    {
        std::cout << "[DEBUG] Format modification failed, trying native format..." << std::endl;

        // Get the mix format again
        CoTaskMemFree(m_waveFormat);
        hr = m_audioClient->GetMixFormat(&m_waveFormat);
        if (SUCCEEDED(hr))
        {
            // Update our stats to match the native format
            m_stats.sample_rate = m_waveFormat->nSamplesPerSec;
            m_stats.channels = m_waveFormat->nChannels;
            m_stats.bits_per_sample = m_waveFormat->wBitsPerSample;

            std::cout << "[DEBUG] Using native format - Sample Rate: " << m_stats.sample_rate
                      << ", Channels: " << m_stats.channels
                      << ", Bits: " << m_stats.bits_per_sample << std::endl;

            // Try initialization with native format
            hr = m_audioClient->Initialize(
                AUDCLNT_SHAREMODE_SHARED,
                AUDCLNT_STREAMFLAGS_LOOPBACK,
                10000000, // 1 second buffer
                0,
                m_waveFormat,
                nullptr);
            std::cout << "[DEBUG] Initialize with native format HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
        }
    }

    if (FAILED(hr))
    {
        LOG_ERROR_FMT("Failed to initialize audio client: 0x%08X", hr);
        std::cout << "[ERROR] Failed to initialize audio client: 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }

    // Get capture client
    std::cout << "[DEBUG] GetService IAudioCaptureClient..." << std::endl;
    hr = m_audioClient->GetService(
        __uuidof(IAudioCaptureClient),
        (void **)&m_captureClient);
    std::cout << "[DEBUG] GetService IAudioCaptureClient HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
    if (FAILED(hr))
    {
        LOG_ERROR_FMT("Failed to get capture client: 0x%08X", hr);
        std::cout << "[ERROR] Failed to get capture client: 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }

    LOG_INFO_FMT("WASAPI initialized - Sample Rate: %d, Channels: %d, Bits: %d",
                 m_stats.sample_rate, m_stats.channels, m_stats.bits_per_sample);
    std::cout << "[DEBUG] WASAPI initialized - Sample Rate: " << m_stats.sample_rate << ", Channels: " << m_stats.channels << ", Bits: " << m_stats.bits_per_sample << std::endl;
    return true;
}

bool AudioCapture::StartCapture()
{
    if (!m_isInitialized)
    {
        LOG_ERROR("Audio capture not initialized");
        return false;
    }

    if (m_isCapturing)
    {
        LOG_WARNING("Audio capture already started");
        return true;
    }

    LOG_INFO("Starting audio capture...");

    // Start the audio client
    HRESULT hr = m_audioClient->Start();
    if (FAILED(hr))
    {
        LOG_ERROR_FMT("Failed to start audio client: 0x%08X", hr);
        return false;
    }

    // Start capture thread
    m_threadShouldExit = false;
    m_captureThread = CreateThread(
        nullptr,
        0,
        CaptureThread,
        this,
        0,
        nullptr);

    if (!m_captureThread)
    {
        LOG_ERROR("Failed to create capture thread");
        m_audioClient->Stop();
        return false;
    }

    m_isCapturing = true;
    LOG_INFO("Audio capture started successfully");
    return true;
}

void AudioCapture::StopCapture()
{
    if (!m_isCapturing)
    {
        return;
    }

    LOG_INFO("Stopping audio capture...");

    // Signal thread to exit
    m_threadShouldExit = true;

    // Wait for thread to finish
    if (m_captureThread)
    {
        WaitForSingleObject(m_captureThread, 5000);
        CloseHandle(m_captureThread);
        m_captureThread = nullptr;
    }

    // Stop audio client
    if (m_audioClient)
    {
        m_audioClient->Stop();
    }

    m_isCapturing = false;
    LOG_INFO("Audio capture stopped");
}

bool AudioCapture::GetNextFrame(std::vector<uint8_t> &frame_out)
{
    // This method is for synchronous frame retrieval
    // In practice, we use the callback mechanism for real-time processing
    frame_out.clear();
    return false;
}

void AudioCapture::SetCallback(std::function<void(const std::vector<uint8_t> &)> callback)
{
    m_callback = callback;
}

DWORD WINAPI AudioCapture::CaptureThread(LPVOID param)
{
    AudioCapture *capture = static_cast<AudioCapture *>(param);
    return capture->CaptureThreadMain();
}

DWORD AudioCapture::CaptureThreadMain()
{
    LOG_INFO("Audio capture thread started");

    std::vector<uint8_t> frameBuffer;
    frameBuffer.reserve(m_stats.frame_size * m_stats.channels * (m_stats.bits_per_sample / 8));

    while (!m_threadShouldExit)
    {
        UINT32 packetLength = 0;
        HRESULT hr = m_captureClient->GetNextPacketSize(&packetLength);

        if (FAILED(hr))
        {
            LOG_ERROR_FMT("GetNextPacketSize failed: 0x%08X", hr);
            break;
        }

        while (packetLength > 0 && !m_threadShouldExit)
        {
            BYTE *data;
            UINT32 framesAvailable;
            DWORD flags;
            UINT64 devicePosition;
            UINT64 qpcPosition;

            hr = m_captureClient->GetBuffer(
                &data,
                &framesAvailable,
                &flags,
                &devicePosition,
                &qpcPosition);

            if (FAILED(hr))
            {
                LOG_ERROR_FMT("GetBuffer failed: 0x%08X", hr);
                break;
            }

            if (framesAvailable > 0)
            {
                size_t bytesPerFrame = m_stats.channels * (m_stats.bits_per_sample / 8);
                size_t totalBytes = framesAvailable * bytesPerFrame;

                // Copy frame data
                frameBuffer.assign(data, data + totalBytes);

                // Update statistics
                m_stats.frames_captured++;

                // Call callback if set
                if (m_callback)
                {
                    m_callback(frameBuffer);
                }
            }

            hr = m_captureClient->ReleaseBuffer(framesAvailable);
            if (FAILED(hr))
            {
                LOG_ERROR_FMT("ReleaseBuffer failed: 0x%08X", hr);
                break;
            }

            hr = m_captureClient->GetNextPacketSize(&packetLength);
            if (FAILED(hr))
            {
                LOG_ERROR_FMT("GetNextPacketSize failed: 0x%08X", hr);
                break;
            }
        }

        // Small sleep to prevent busy waiting
        Sleep(1);
    }

    LOG_INFO("Audio capture thread exiting");
    return 0;
}

std::vector<std::string> AudioCapture::GetAvailableDevices() const
{
    std::vector<std::string> devices;

    if (!m_deviceEnumerator)
    {
        std::cout << "[DEBUG] Device enumerator not initialized." << std::endl;
        return devices;
    }

    IMMDeviceCollection *deviceCollection = nullptr;
    HRESULT hr = m_deviceEnumerator->EnumAudioEndpoints(
        eRender,
        DEVICE_STATE_ACTIVE,
        &deviceCollection);
    std::cout << "[DEBUG] EnumAudioEndpoints HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
    if (SUCCEEDED(hr))
    {
        UINT deviceCount = 0;
        hr = deviceCollection->GetCount(&deviceCount);
        std::cout << "[DEBUG] Device count: " << deviceCount << ", HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
        if (SUCCEEDED(hr))
        {
            for (UINT i = 0; i < deviceCount; i++)
            {
                IMMDevice *device = nullptr;
                hr = deviceCollection->Item(i, &device);
                std::cout << "[DEBUG] Device " << i << " Item HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
                if (SUCCEEDED(hr))
                {
                    IPropertyStore *props = nullptr;
                    hr = device->OpenPropertyStore(STGM_READ, &props);
                    std::cout << "[DEBUG] OpenPropertyStore HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
                    if (SUCCEEDED(hr))
                    {
                        PROPVARIANT var;
                        PropVariantInit(&var);
                        hr = props->GetValue(PKEY_Device_FriendlyName, &var);
                        std::cout << "[DEBUG] GetValue HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
                        if (SUCCEEDED(hr) && var.vt == VT_LPWSTR)
                        {
                            // Convert wide string to narrow string
                            int size_needed = WideCharToMultiByte(CP_UTF8, 0, var.pwszVal, -1, nullptr, 0, nullptr, nullptr);
                            std::string deviceName(size_needed, 0);
                            WideCharToMultiByte(CP_UTF8, 0, var.pwszVal, -1, &deviceName[0], size_needed, nullptr, nullptr);
                            devices.push_back(deviceName);
                            std::cout << "[DEBUG] Device name: " << deviceName << std::endl;
                        }
                        PropVariantClear(&var);
                        props->Release();
                    }
                    device->Release();
                }
            }
        }
        deviceCollection->Release();
    }

    return devices;
}

AudioCapture::CaptureStats AudioCapture::GetStats() const
{
    return m_stats;
}

void AudioCapture::CleanupWASAPI()
{
    if (m_captureClient)
    {
        m_captureClient->Release();
        m_captureClient = nullptr;
    }

    if (m_audioClient)
    {
        m_audioClient->Release();
        m_audioClient = nullptr;
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

    if (m_waveFormat)
    {
        CoTaskMemFree(m_waveFormat);
        m_waveFormat = nullptr;
    }

    CoUninitialize();
    m_isInitialized = false;
}
