#include "jitter_buffer.h"
#include <iostream>
#include <algorithm>
#include <cmath>

JitterBuffer::JitterBuffer()
    : isReady_(false), firstFrameReceived_(false), expectedSequence_(0), lastSequence_(0)
{
}

JitterBuffer::~JitterBuffer()
{
}

void JitterBuffer::PushFrame(const std::vector<uint8_t> &frame, uint64_t timestamp, uint32_t sequence)
{
    std::lock_guard<std::mutex> lock(bufferMutex_);

    AudioFrame audioFrame(frame, timestamp, sequence);

    // Update statistics
    stats_.totalFramesReceived++;

    // Check if frame should be dropped
    if (ShouldDropFrame(audioFrame))
    {
        stats_.framesDropped++;
        return;
    }

    // Handle frame ordering
    if (config_.enableFrameReordering)
    {
        InsertFrameInOrder(audioFrame);
    }
    else
    {
        // Simple append
        frameBuffer_[timestamp] = audioFrame;
    }

    // Update buffer statistics
    UpdateStatistics();

    // Check if buffer is ready for playback
    if (!isReady_ && frameBuffer_.size() >= 3)
    { // At least 3 frames
        isReady_ = true;
    }

    // Clean up old frames
    CleanupOldFrames();

    // Adaptive latency adjustment
    if (config_.enableAdaptiveLatency)
    {
        AdaptiveLatencyAdjustment();
    }
}

bool JitterBuffer::GetFrame(std::vector<uint8_t> &frame_out)
{
    std::lock_guard<std::mutex> lock(bufferMutex_);

    if (!isReady_ || frameBuffer_.empty())
    {
        return false;
    }

    // Get the oldest frame
    auto it = frameBuffer_.begin();
    if (it != frameBuffer_.end())
    {
        frame_out = it->second.data;
        lastPlaybackTime_ = it->second.timestamp;
        frameBuffer_.erase(it);

        stats_.framesBuffered++;
        UpdateStatistics();

        return true;
    }

    return false;
}

void JitterBuffer::SetTargetLatencyMs(int latencyMs)
{
    std::lock_guard<std::mutex> lock(bufferMutex_);
    config_.targetLatencyMs = latencyMs;
    stats_.targetLatencyMs = latencyMs;
}

void JitterBuffer::SetMaxLatencyMs(int latencyMs)
{
    std::lock_guard<std::mutex> lock(bufferMutex_);
    config_.maxLatencyMs = latencyMs;
    stats_.maxLatencyMs = latencyMs;
}

void JitterBuffer::SetConfig(const BufferConfig &config)
{
    std::lock_guard<std::mutex> lock(bufferMutex_);
    config_ = config;
    stats_.targetLatencyMs = config.targetLatencyMs;
    stats_.maxLatencyMs = config.maxLatencyMs;
}

JitterBuffer::BufferStats JitterBuffer::GetStats() const
{
    std::lock_guard<std::mutex> lock(bufferMutex_);
    return stats_;
}

void JitterBuffer::Reset()
{
    std::lock_guard<std::mutex> lock(bufferMutex_);

    frameBuffer_.clear();
    playbackQueue_ = std::queue<AudioFrame>();
    jitterHistory_.clear();

    stats_ = BufferStats();
    stats_.targetLatencyMs = config_.targetLatencyMs;
    stats_.maxLatencyMs = config_.maxLatencyMs;

    isReady_ = false;
    firstFrameReceived_ = false;
    expectedSequence_ = 0;
    lastSequence_ = 0;
}

bool JitterBuffer::IsReady() const
{
    std::lock_guard<std::mutex> lock(bufferMutex_);
    return isReady_;
}

int JitterBuffer::GetOccupancyPercent() const
{
    std::lock_guard<std::mutex> lock(bufferMutex_);

    if (frameBuffer_.empty())
    {
        return 0;
    }

    // Calculate occupancy based on target latency
    uint64_t currentTime = GetCurrentTimestamp();
    uint64_t oldestFrameTime = frameBuffer_.begin()->second.timestamp;
    int actualLatencyMs = CalculateLatencyMs(oldestFrameTime);

    return static_cast<int>((actualLatencyMs * 100.0f) / config_.targetLatencyMs);
}

void JitterBuffer::Flush()
{
    std::lock_guard<std::mutex> lock(bufferMutex_);
    frameBuffer_.clear();
    playbackQueue_ = std::queue<AudioFrame>();
    isReady_ = false;
}

// Private methods implementation
void JitterBuffer::UpdateLatency()
{
    if (frameBuffer_.empty())
    {
        stats_.currentLatencyMs = 0;
        return;
    }

    uint64_t currentTime = GetCurrentTimestamp();
    uint64_t oldestFrameTime = frameBuffer_.begin()->second.timestamp;
    stats_.currentLatencyMs = CalculateLatencyMs(oldestFrameTime);
}

void JitterBuffer::AdaptiveLatencyAdjustment()
{
    if (jitterHistory_.size() < 10)
    {
        return; // Need more data
    }

    float avgJitter = CalculateJitter();
    float currentLatency = static_cast<float>(stats_.currentLatencyMs);

    // Adjust target latency based on jitter
    if (avgJitter > config_.adaptiveThresholdMs)
    {
        // Increase latency to handle high jitter
        int newLatency = static_cast<int>(currentLatency + LATENCY_ADJUSTMENT_FACTOR * avgJitter);
        if (newLatency <= config_.maxLatencyMs)
        {
            config_.targetLatencyMs = newLatency;
            stats_.targetLatencyMs = newLatency;
        }
    }
    else if (avgJitter < config_.adaptiveThresholdMs / 2)
    {
        // Decrease latency if jitter is low
        int newLatency = static_cast<int>(currentLatency - LATENCY_ADJUSTMENT_FACTOR * avgJitter);
        if (newLatency >= config_.minLatencyMs)
        {
            config_.targetLatencyMs = newLatency;
            stats_.targetLatencyMs = newLatency;
        }
    }
}

void JitterBuffer::CleanupOldFrames()
{
    uint64_t currentTime = GetCurrentTimestamp();
    uint64_t maxAge = MAX_FRAME_AGE_MS * 1000; // Convert to microseconds

    auto it = frameBuffer_.begin();
    while (it != frameBuffer_.end())
    {
        if (currentTime - it->second.timestamp > maxAge)
        {
            stats_.framesDropped++;
            it = frameBuffer_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

bool JitterBuffer::ShouldDropFrame(const AudioFrame &frame) const
{
    // Drop frames that are too old
    uint64_t currentTime = GetCurrentTimestamp();
    uint64_t maxAge = MAX_FRAME_AGE_MS * 1000;

    if (currentTime - frame.timestamp > maxAge)
    {
        return true;
    }

    // Drop frames if buffer is too full
    if (frameBuffer_.size() >= config_.maxBufferSize)
    {
        return true;
    }

    return false;
}

void JitterBuffer::UpdateStatistics()
{
    UpdateLatency();

    // Calculate buffer occupancy
    if (config_.targetLatencyMs > 0)
    {
        stats_.bufferOccupancyPercent = GetOccupancyPercent();
    }

    // Update jitter history
    if (!frameBuffer_.empty())
    {
        float jitter = CalculateJitter();
        jitterHistory_.push_back(jitter);

        if (jitterHistory_.size() > JITTER_HISTORY_SIZE)
        {
            jitterHistory_.erase(jitterHistory_.begin());
        }

        stats_.avgJitterMs = CalculateJitter();
    }

    // Check for underrun/overrun
    stats_.isUnderrun = frameBuffer_.empty() && isReady_;
    stats_.isOverrun = frameBuffer_.size() > config_.maxBufferSize * 0.8;
}

uint64_t JitterBuffer::GetCurrentTimestamp() const
{
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
    return duration.count();
}

void JitterBuffer::InsertFrameInOrder(const AudioFrame &frame)
{
    // Simple insertion by timestamp
    frameBuffer_[frame.timestamp] = frame;

    // Check for out-of-order frames
    if (firstFrameReceived_)
    {
        if (frame.sequence < lastSequence_)
        {
            stats_.framesReordered++;
        }
    }
    else
    {
        firstFrameReceived_ = true;
    }

    lastSequence_ = frame.sequence;
}

bool JitterBuffer::IsFrameInOrder(uint32_t sequence) const
{
    if (!firstFrameReceived_)
    {
        return true;
    }

    return sequence >= expectedSequence_;
}

void JitterBuffer::HandleOutOfOrderFrame(const AudioFrame &frame)
{
    // For now, just insert by timestamp
    // In a more sophisticated implementation, you might want to reorder frames
    frameBuffer_[frame.timestamp] = frame;
    stats_.framesReordered++;
}

int JitterBuffer::CalculateLatencyMs(uint64_t timestamp) const
{
    uint64_t currentTime = GetCurrentTimestamp();
    return static_cast<int>((currentTime - timestamp) / 1000); // Convert to milliseconds
}

float JitterBuffer::CalculateJitter() const
{
    if (jitterHistory_.empty())
    {
        return 0.0f;
    }

    float sum = 0.0f;
    for (float jitter : jitterHistory_)
    {
        sum += jitter;
    }

    return sum / jitterHistory_.size();
}
