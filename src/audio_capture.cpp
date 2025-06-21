#include "audio_capture.h"
#include <iostream>
#include <algorithm>
#include <chrono>

AudioCapture::AudioCapture()
    : isCapturing_(false), comInitialized_(false)
{
    // Initialize COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr))
    {
        comInitialized_ = true;
    }
}

AudioCapture::~AudioCapture()
{
    StopCapture();
    CleanupWASAPI();

    if (comInitialized_)
    {
        CoUninitialize();
    }
}

bool AudioCapture::InitAudioCapture(const std::string &deviceName)
{
    if (!InitializeWASAPI())
    {
        std::cerr << "Failed to initialize WASAPI\n";
        return false;
    }

    if (!CreateAudioClient())
    {
        std::cerr << "Failed to create audio client\n";
        return false;
    }

    std::cout << "Audio capture initialized successfully\n";
    return true;
}

bool AudioCapture::StartCapture()
{
    if (isCapturing_)
    {
        std::cout << "Audio capture already running\n";
        return true;
    }

    if (!StartAudioCapture())
    {
        std::cerr << "Failed to start audio capture\n";
        return false;
    }

    isCapturing_ = true;
    captureThread_ = std::thread(&AudioCapture::CaptureThread, this);

    std::cout << "Audio capture started\n";
    return true;
}

void AudioCapture::StopCapture()
{
    if (!isCapturing_)
        return;

    isCapturing_ = false;

    if (captureThread_.joinable())
    {
        captureThread_.join();
    }

    std::cout << "Audio capture stopped\n";
}

bool AudioCapture::GetNextFrame(std::vector<uint8_t> &frame_out)
{
    std::unique_lock<std::mutex> lock(frameMutex_);

    if (frameBuffer_.empty())
    {
        return false;
    }

    frame_out = frameBuffer_;
    frameBuffer_.clear();

    stats_.framesCaptured++;
    stats_.bytesCaptured += frame_out.size();

    return true;
}

void AudioCapture::SetCallback(std::function<void(const std::vector<uint8_t> &)> callback)
{
    frameCallback_ = callback;
}

void AudioCapture::SetAudioFormat(const AudioFormat &format)
{
    format_ = format;
}

AudioCapture::CaptureStats AudioCapture::GetStats() const
{
    return stats_;
}

std::vector<std::string> AudioCapture::GetAvailableDevices()
{
    std::vector<std::string> devices;

    // For now, return a default device
    devices.push_back("Default Audio Device");

    return devices;
}

// Private methods implementation
bool AudioCapture::InitializeWASAPI()
{
    // This is a simplified implementation
    // In a real implementation, you would initialize COM objects here
    return true;
}

bool AudioCapture::CreateAudioClient()
{
    // This is a simplified implementation
    // In a real implementation, you would create WASAPI audio client here
    return true;
}

bool AudioCapture::StartAudioCapture()
{
    // This is a simplified implementation
    // In a real implementation, you would start WASAPI capture here
    return true;
}

void AudioCapture::CaptureThread()
{
    // Simulate audio capture with dummy data
    const size_t frameSize = format_.frameSize * format_.channels * (format_.bitsPerSample / 8);

    while (isCapturing_)
    {
        // Generate dummy audio frame
        std::vector<uint8_t> frame(frameSize);

        // Fill with some dummy data (sine wave)
        static float phase = 0.0f;
        const float frequency = 440.0f; // A4 note
        const float amplitude = 0.1f;

        for (size_t i = 0; i < frameSize; i += 2)
        {
            int16_t sample = static_cast<int16_t>(amplitude * 32767.0f * sin(phase));
            frame[i] = sample & 0xFF;
            frame[i + 1] = (sample >> 8) & 0xFF;
            phase += 2.0f * M_PI * frequency / format_.sampleRate;
        }

        // Store frame
        {
            std::lock_guard<std::mutex> lock(frameMutex_);
            frameBuffer_ = frame;
        }

        // Call callback if set
        if (frameCallback_)
        {
            frameCallback_(frame);
        }

        // Sleep for frame duration
        std::this_thread::sleep_for(std::chrono::milliseconds(
            static_cast<int>(1000.0f * format_.frameSize / format_.sampleRate)));
    }
}

void AudioCapture::ProcessAudioFrame()
{
    // This would process actual WASAPI audio frames
}

void AudioCapture::CleanupWASAPI()
{
    // Clean up WASAPI objects
    if (wasapi_.audioClient)
    {
        wasapi_.audioClient->Release();
        wasapi_.audioClient = nullptr;
    }
    if (wasapi_.captureClient)
    {
        wasapi_.captureClient->Release();
        wasapi_.captureClient = nullptr;
    }
    if (wasapi_.device)
    {
        wasapi_.device->Release();
        wasapi_.device = nullptr;
    }
    if (wasapi_.deviceEnumerator)
    {
        wasapi_.deviceEnumerator->Release();
        wasapi_.deviceEnumerator = nullptr;
    }
}

std::string AudioCapture::GetDefaultDeviceName() const
{
    return "Default Audio Device";
}

bool AudioCapture::IsDeviceValid(const std::string &deviceName) const
{
    return true; // Simplified implementation
}

uint64_t AudioCapture::GetCurrentTimestamp() const
{
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
    return duration.count();
}
