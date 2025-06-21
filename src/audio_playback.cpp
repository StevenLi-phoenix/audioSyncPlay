#include "audio_playback.h"
#include <iostream>
#include <algorithm>
#include <cmath>

AudioPlayback::AudioPlayback()
    : isPlaying_(false), comInitialized_(false)
{
    // Initialize COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr))
    {
        comInitialized_ = true;
    }
}

AudioPlayback::~AudioPlayback()
{
    StopPlayback();
    CleanupWASAPI();

    if (comInitialized_)
    {
        CoUninitialize();
    }
}

bool AudioPlayback::InitAudioPlayback(const std::string &deviceName, const PlaybackConfig &config)
{
    config_ = config;

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

    std::cout << "Audio playback initialized successfully\n";
    return true;
}

bool AudioPlayback::StartPlayback()
{
    if (isPlaying_)
    {
        std::cout << "Audio playback already running\n";
        return true;
    }

    if (!StartAudioPlayback())
    {
        std::cerr << "Failed to start audio playback\n";
        return false;
    }

    isPlaying_ = true;
    playbackThread_ = std::thread(&AudioPlayback::PlaybackThread, this);

    std::cout << "Audio playback started\n";
    return true;
}

void AudioPlayback::StopPlayback()
{
    if (!isPlaying_)
        return;

    isPlaying_ = false;

    if (playbackThread_.joinable())
    {
        playbackThread_.join();
    }

    std::cout << "Audio playback stopped\n";
}

bool AudioPlayback::PlayFrame(const std::vector<uint8_t> &frame)
{
    if (!isPlaying_)
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(bufferMutex_);

    if (frameQueue_.size() >= MAX_QUEUE_SIZE)
    {
        stats_.framesDropped++;
        return false;
    }

    // Apply volume if enabled
    std::vector<uint8_t> frameCopy = frame;
    if (config_.enableVolumeControl)
    {
        ApplyVolume(frameCopy);
    }

    frameQueue_.push(frameCopy);
    bufferCondition_.notify_one();

    return true;
}

void AudioPlayback::SetVolume(float volume)
{
    volume = std::max(0.0f, std::min(1.0f, volume)); // Clamp to 0-1
    stats_.currentVolume = volume;

    if (wasapi_.audioVolume)
    {
        wasapi_.audioVolume->SetMasterVolume(volume, nullptr);
    }
}

AudioPlayback::PlaybackStats AudioPlayback::GetStats() const
{
    std::lock_guard<std::mutex> lock(bufferMutex_);
    return stats_;
}

void AudioPlayback::SetConfig(const PlaybackConfig &config)
{
    config_ = config;
}

bool AudioPlayback::IsPlaying() const
{
    return isPlaying_;
}

std::vector<std::string> AudioPlayback::GetAvailableDevices()
{
    std::vector<std::string> devices;

    // For now, return a default device
    devices.push_back("Default Audio Device");

    return devices;
}

void AudioPlayback::FlushBuffer()
{
    std::lock_guard<std::mutex> lock(bufferMutex_);

    while (!frameQueue_.empty())
    {
        frameQueue_.pop();
    }
}

void AudioPlayback::Pause()
{
    // This would pause WASAPI playback
    std::cout << "Audio playback paused\n";
}

void AudioPlayback::Resume()
{
    // This would resume WASAPI playback
    std::cout << "Audio playback resumed\n";
}

// Private methods implementation
bool AudioPlayback::InitializeWASAPI()
{
    // This is a simplified implementation
    // In a real implementation, you would initialize COM objects here
    return true;
}

bool AudioPlayback::CreateAudioClient()
{
    // This is a simplified implementation
    // In a real implementation, you would create WASAPI audio client here
    return true;
}

bool AudioPlayback::StartAudioPlayback()
{
    // This is a simplified implementation
    // In a real implementation, you would start WASAPI playback here
    return true;
}

void AudioPlayback::PlaybackThread()
{
    // Simulate audio playback
    const size_t frameSize = config_.format.frameSize * config_.format.channels * (config_.format.bitsPerSample / 8);

    while (isPlaying_)
    {
        std::vector<uint8_t> frame;

        {
            std::unique_lock<std::mutex> lock(bufferMutex_);
            bufferCondition_.wait(lock, [this]
                                  { return !frameQueue_.empty() || !isPlaying_; });

            if (!isPlaying_)
                break;

            if (!frameQueue_.empty())
            {
                frame = frameQueue_.front();
                frameQueue_.pop();
            }
        }

        if (!frame.empty())
        {
            // Simulate writing to audio buffer
            if (WriteToBuffer(frame.data(), frame.size()))
            {
                stats_.framesPlayed++;
                stats_.bytesPlayed += frame.size();
                lastPlaybackTime_ = GetCurrentTimestamp();
            }
            else
            {
                stats_.framesUnderrun++;
            }
        }
        else
        {
            // Generate silence if no frame available
            std::vector<uint8_t> silence(frameSize, 0);
            WriteToBuffer(silence.data(), silence.size());
            stats_.framesUnderrun++;
        }

        // Sleep for frame duration
        std::this_thread::sleep_for(std::chrono::milliseconds(
            static_cast<int>(1000.0f * config_.format.frameSize / config_.format.sampleRate)));
    }
}

void AudioPlayback::ProcessAudioBuffer()
{
    // This would process actual WASAPI audio buffer
}

void AudioPlayback::CleanupWASAPI()
{
    // Clean up WASAPI objects
    if (wasapi_.audioVolume)
    {
        wasapi_.audioVolume->Release();
        wasapi_.audioVolume = nullptr;
    }
    if (wasapi_.renderClient)
    {
        wasapi_.renderClient->Release();
        wasapi_.renderClient = nullptr;
    }
    if (wasapi_.audioClient)
    {
        wasapi_.audioClient->Release();
        wasapi_.audioClient = nullptr;
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

bool AudioPlayback::QueueFrame(const std::vector<uint8_t> &frame)
{
    std::lock_guard<std::mutex> lock(bufferMutex_);

    if (frameQueue_.size() >= MAX_QUEUE_SIZE)
    {
        return false;
    }

    frameQueue_.push(frame);
    return true;
}

bool AudioPlayback::WriteToBuffer(const uint8_t *data, size_t size)
{
    // This is a simplified implementation
    // In a real implementation, you would write to WASAPI buffer here

    // Simulate some processing time
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    return true;
}

void AudioPlayback::UpdateBufferLevel()
{
    std::lock_guard<std::mutex> lock(bufferMutex_);

    if (MAX_QUEUE_SIZE > 0)
    {
        stats_.bufferLevelPercent = (frameQueue_.size() * 100.0f) / MAX_QUEUE_SIZE;
    }
}

std::string AudioPlayback::GetDefaultDeviceName() const
{
    return "Default Audio Device";
}

bool AudioPlayback::IsDeviceValid(const std::string &deviceName) const
{
    return true; // Simplified implementation
}

uint64_t AudioPlayback::GetCurrentTimestamp() const
{
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
    return duration.count();
}

void AudioPlayback::ApplyVolume(std::vector<uint8_t> &frame)
{
    if (stats_.currentVolume == 1.0f)
    {
        return; // No volume adjustment needed
    }

    // Apply volume to 16-bit PCM samples
    if (config_.format.bitsPerSample == 16)
    {
        int16_t *samples = reinterpret_cast<int16_t *>(frame.data());
        size_t numSamples = frame.size() / 2;

        for (size_t i = 0; i < numSamples; i++)
        {
            samples[i] = static_cast<int16_t>(samples[i] * stats_.currentVolume);
        }
    }
}
