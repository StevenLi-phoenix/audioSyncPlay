#include "jitter_buffer.h"
#include "logger.h"
#include <algorithm>
#include <cmath>

// Placeholder jitter buffer implementation
// This will be fully implemented in Week 3
JitterBuffer::JitterBuffer()
    : m_targetLatencyMs(100), m_maxLatencyMs(200), m_frameSize(1024), m_sampleRate(44100), m_channels(2), m_nextExpectedSequence(0), m_lastSequenceReceived(0), m_lastFrameTime(std::chrono::steady_clock::now())
{
    ResetStats();
    LOG_INFO_FMT("Jitter buffer initialized with target latency: {} ms, max latency: {} ms",
                 m_targetLatencyMs, m_maxLatencyMs);
}

JitterBuffer::~JitterBuffer()
{
    Clear();
    LOG_INFO("Jitter buffer destroyed");
}

void JitterBuffer::PushFrame(const std::vector<uint8_t> &frame, uint64_t timestamp, uint32_t sequence_number)
{
    auto now = std::chrono::steady_clock::now();

    // Create audio frame
    AudioFrame audioFrame(frame, timestamp, sequence_number);

    // Check if frame is too old
    if (IsFrameTooOld(audioFrame))
    {
        m_stats.frames_dropped++;
        LOG_DEBUG_FMT("Dropping old frame with sequence: {}", sequence_number);
        return;
    }

    // Insert frame into buffer
    m_frameBuffer[sequence_number] = audioFrame;
    m_stats.total_frames_received++;

    // Update last sequence received
    if (sequence_number > m_lastSequenceReceived)
    {
        m_lastSequenceReceived = sequence_number;
    }

    // Reorder frames if needed
    ReorderFrames();

    // Update statistics
    UpdateStats();

    // Adaptive buffer adjustment
    AdaptiveBufferAdjustment();

    // Periodic memory optimization (every 100 frames)
    static uint32_t frameCounter = 0;
    frameCounter++;
    if (frameCounter % 100 == 0)
    {
        OptimizeMemoryUsage();
    }

    LOG_DEBUG_FMT("Pushed frame {} into jitter buffer. Buffer size: {}",
                  sequence_number, m_frameBuffer.size());
}

bool JitterBuffer::GetFrame(std::vector<uint8_t> &frame_out)
{
    if (m_outputQueue.empty())
    {
        return false;
    }

    uint32_t nextSequence = m_outputQueue.front();
    auto it = m_frameBuffer.find(nextSequence);

    if (it == m_frameBuffer.end())
    {
        // Frame missing, skip it
        m_outputQueue.pop();
        m_stats.frames_dropped++;
        LOG_WARNING_FMT("Missing frame in sequence: {}", nextSequence);
        return GetFrame(frame_out); // Try next frame
    }

    // Get the frame
    frame_out = it->second.data;
    m_frameBuffer.erase(it);
    m_outputQueue.pop();

    // Update expected sequence
    m_nextExpectedSequence = nextSequence + 1;

    // Update statistics
    m_stats.frames_buffered++;
    UpdateStats();

    LOG_DEBUG_FMT("Retrieved frame {} from jitter buffer", nextSequence);
    return true;
}

void JitterBuffer::SetTargetLatencyMs(int latency)
{
    m_targetLatencyMs = std::max(1, std::min(50, latency)); // Clamp between 1-50ms for ultra-low latency
    LOG_INFO_FMT("Jitter buffer target latency set to: {} ms", m_targetLatencyMs);
}

void JitterBuffer::SetMaxLatencyMs(int max_latency)
{
    m_maxLatencyMs = std::max(5, std::min(100, max_latency)); // Clamp between 5-100ms for ultra-low latency
    LOG_INFO_FMT("Jitter buffer max latency set to: {} ms", m_maxLatencyMs);
}

void JitterBuffer::SetFrameSize(size_t frame_size)
{
    m_frameSize = frame_size;
    LOG_INFO_FMT("Frame size set to: {} bytes", frame_size);
}

void JitterBuffer::SetSampleRate(uint32_t sample_rate)
{
    m_sampleRate = sample_rate;
    LOG_INFO_FMT("Sample rate set to: {} Hz", sample_rate);
}

void JitterBuffer::SetChannels(uint32_t channels)
{
    m_channels = channels;
    LOG_INFO_FMT("Channels set to: {}", channels);
}

JitterBuffer::BufferStats JitterBuffer::GetStats() const
{
    UpdateStats();
    return m_stats;
}

void JitterBuffer::ResetStats()
{
    m_stats = BufferStats();
    m_stats.target_latency_ms = m_targetLatencyMs;
    m_nextExpectedSequence = 0;
    m_lastSequenceReceived = 0;
    m_jitterHistory.clear();
    LOG_INFO("Jitter buffer statistics reset");
}

void JitterBuffer::Clear()
{
    m_frameBuffer.clear();
    while (!m_outputQueue.empty())
    {
        m_outputQueue.pop();
    }
    m_nextExpectedSequence = 0;
    m_lastSequenceReceived = 0;
    LOG_INFO("Jitter buffer cleared");
}

bool JitterBuffer::IsEmpty() const
{
    return m_outputQueue.empty();
}

size_t JitterBuffer::GetBufferSize() const
{
    return m_frameBuffer.size();
}

void JitterBuffer::UpdateStats() const
{
    // Calculate current latency
    m_stats.current_latency_ms = CalculateCurrentLatency();

    // Calculate buffer occupancy
    int maxFrames = (m_maxLatencyMs * m_sampleRate) / (1000 * m_frameSize / (m_channels * 2));
    m_stats.buffer_occupancy_percent = maxFrames > 0 ? (m_frameBuffer.size() * 100) / maxFrames : 0;

    // Calculate average jitter
    m_stats.avg_jitter_ms = CalculateJitter();

    // Update target latency in stats
    m_stats.target_latency_ms = m_targetLatencyMs;
}

void JitterBuffer::AdaptiveBufferAdjustment()
{
    if (m_frameBuffer.empty())
        return;

    // OPTIMIZATION: Improved adaptive buffer adjustment using EWMA
    float currentJitter = CalculateJitter();
    int currentLatency = CalculateCurrentLatency();

    // Use exponential weighted moving average for smoother adjustments
    static float ewmaJitter = currentJitter;
    static float ewmaLatency = static_cast<float>(currentLatency);
    static int adjustmentCounter = 0;

    const float alpha = 0.1f; // EWMA smoothing factor
    ewmaJitter = alpha * currentJitter + (1.0f - alpha) * ewmaJitter;
    ewmaLatency = alpha * currentLatency + (1.0f - alpha) * ewmaLatency;

    // Only adjust every N frames to prevent oscillation
    adjustmentCounter++;
    if (adjustmentCounter < 10) return;
    adjustmentCounter = 0;

    // Calculate buffer occupancy percentage
    int maxFrames = (m_maxLatencyMs * m_sampleRate) / (1000 * m_frameSize / (m_channels * 2));
    float occupancyRatio = maxFrames > 0 ? static_cast<float>(m_frameBuffer.size()) / maxFrames : 0.0f;

    // Adaptive thresholds based on network conditions
    float jitterThresholdHigh = std::max(2.0f, m_targetLatencyMs * 0.3f);
    float jitterThresholdLow = std::max(0.5f, m_targetLatencyMs * 0.1f);
    float latencyThresholdHigh = m_targetLatencyMs * 1.5f;
    float latencyThresholdLow = m_targetLatencyMs * 0.5f;

    // Decision logic for buffer adjustment
    bool shouldIncrease = false;
    bool shouldDecrease = false;

    if (ewmaJitter > jitterThresholdHigh || ewmaLatency > latencyThresholdHigh ||
        occupancyRatio > 0.8f || m_stats.frames_dropped > 0)
    {
        shouldIncrease = true;
    }
    else if (ewmaJitter < jitterThresholdLow && ewmaLatency < latencyThresholdLow &&
             occupancyRatio < 0.3f && m_stats.frames_dropped == 0)
    {
        shouldDecrease = true;
    }

    // Apply adjustments with hysteresis
    if (shouldIncrease && m_targetLatencyMs < m_maxLatencyMs)
    {
        int oldTarget = m_targetLatencyMs;
        m_targetLatencyMs = std::min(m_maxLatencyMs,
                                   static_cast<int>(m_targetLatencyMs * 1.2f));

        LOG_DEBUG_FMT("Buffer adjustment: jitter={:.2f}ms, latency={:.2f}ms, occupancy={:.1f}%, "
                      "target: {}ms -> {}ms (increase)",
                      ewmaJitter, ewmaLatency, occupancyRatio * 100, oldTarget, m_targetLatencyMs);
    }
    else if (shouldDecrease && m_targetLatencyMs > 1)
    {
        int oldTarget = m_targetLatencyMs;
        m_targetLatencyMs = std::max(1,
                                   static_cast<int>(m_targetLatencyMs * 0.9f));

        LOG_DEBUG_FMT("Buffer adjustment: jitter={:.2f}ms, latency={:.2f}ms, occupancy={:.1f}%, "
                      "target: {}ms -> {}ms (decrease)",
                      ewmaJitter, ewmaLatency, occupancyRatio * 100, oldTarget, m_targetLatencyMs);
    }

    // Age out old frames to prevent memory bloat
    AgeOutOldFrames();
}

bool JitterBuffer::IsFrameTooOld(const AudioFrame &frame) const
{
    auto now = std::chrono::steady_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - frame.receive_time);

    return age.count() > m_maxLatencyMs;
}

void JitterBuffer::ReorderFrames()
{
    // Clear output queue
    while (!m_outputQueue.empty())
    {
        m_outputQueue.pop();
    }

    // Find the earliest sequence number
    if (m_frameBuffer.empty())
    {
        return;
    }

    uint32_t earliestSequence = m_frameBuffer.begin()->first;
    for (const auto &pair : m_frameBuffer)
    {
        if (pair.first < earliestSequence)
        {
            earliestSequence = pair.first;
        }
    }

    // Build ordered output queue
    std::vector<uint32_t> sequences;
    for (const auto &pair : m_frameBuffer)
    {
        sequences.push_back(pair.first);
    }
    std::sort(sequences.begin(), sequences.end());

    // Add sequences to output queue
    for (uint32_t seq : sequences)
    {
        m_outputQueue.push(seq);
    }

    // Count reordered frames
    if (sequences.size() > 1)
    {
        uint32_t reordered = 0;
        for (size_t i = 1; i < sequences.size(); ++i)
        {
            if (sequences[i] != sequences[i - 1] + 1)
            {
                reordered++;
            }
        }
        m_stats.frames_reordered += reordered;
    }
}

float JitterBuffer::CalculateJitter() const
{
    if (m_frameBuffer.size() < 2)
    {
        return 0.0f;
    }

    std::vector<float> jitterValues;
    auto it = m_frameBuffer.begin();
    auto prevTime = it->second.receive_time;
    ++it;

    for (; it != m_frameBuffer.end(); ++it)
    {
        auto currentTime = it->second.receive_time;
        auto interval = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - prevTime);
        float expectedInterval = (m_frameSize * 1000000.0f) / (m_sampleRate * m_channels * 2);
        float jitter = std::abs(interval.count() - expectedInterval) / 1000.0f; // Convert to ms
        jitterValues.push_back(jitter);
        prevTime = currentTime;
    }

    // Calculate average jitter
    if (jitterValues.empty())
    {
        return 0.0f;
    }

    float sum = 0.0f;
    for (float jitter : jitterValues)
    {
        sum += jitter;
    }

    return sum / jitterValues.size();
}

int JitterBuffer::CalculateCurrentLatency() const
{
    if (m_frameBuffer.empty())
    {
        return 0;
    }

    auto now = std::chrono::steady_clock::now();
    auto oldestFrame = m_frameBuffer.begin()->second;
    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(now - oldestFrame.receive_time);

    return static_cast<int>(latency.count());
}

void JitterBuffer::AgeOutOldFrames()
{
    auto now = std::chrono::steady_clock::now();
    auto maxAge = std::chrono::milliseconds(m_maxLatencyMs * 2); // Age out frames older than 2x max latency

    auto it = m_frameBuffer.begin();
    size_t removedCount = 0;

    while (it != m_frameBuffer.end())
    {
        auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second.receive_time);

        if (age > maxAge)
        {
            // Remove old frame
            it = m_frameBuffer.erase(it);
            removedCount++;
            m_stats.frames_dropped++;
        }
        else
        {
            ++it;
        }
    }

    if (removedCount > 0)
    {
        LOG_DEBUG_FMT("Aged out {} old frames from jitter buffer", removedCount);

        // Rebuild output queue after removing old frames
        ReorderFrames();
    }
}

void JitterBuffer::OptimizeMemoryUsage()
{
    // Limit buffer size to prevent excessive memory usage
    size_t maxBufferSize = (m_maxLatencyMs * m_sampleRate) / (1000 * m_frameSize / (m_channels * 2)) * 2;

    if (m_frameBuffer.size() > maxBufferSize)
    {
        // Remove oldest frames if buffer is too large
        auto it = m_frameBuffer.begin();
        size_t toRemove = m_frameBuffer.size() - maxBufferSize;

        for (size_t i = 0; i < toRemove && it != m_frameBuffer.end(); ++i)
        {
            it = m_frameBuffer.erase(it);
            m_stats.frames_dropped++;
        }

        LOG_WARNING_FMT("Buffer size exceeded limit, removed {} frames", toRemove);

        // Rebuild output queue
        ReorderFrames();
    }
}
