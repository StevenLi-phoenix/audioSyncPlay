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
    m_targetLatencyMs = std::max(10, std::min(500, latency));
    LOG_INFO_FMT("Target latency set to: {} ms", m_targetLatencyMs);
}

void JitterBuffer::SetMaxLatencyMs(int max_latency)
{
    m_maxLatencyMs = std::max(m_targetLatencyMs + 50, max_latency);
    LOG_INFO_FMT("Max latency set to: {} ms", m_maxLatencyMs);
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
    float currentJitter = CalculateJitter();

    // Adjust target latency based on jitter
    if (currentJitter > m_targetLatencyMs * 0.5f)
    {
        // High jitter detected, increase buffer
        int newTarget = std::min(m_maxLatencyMs,
                                 m_targetLatencyMs + static_cast<int>(currentJitter * 0.5f));
        if (newTarget != m_targetLatencyMs)
        {
            m_targetLatencyMs = newTarget;
            LOG_INFO_FMT("Adaptive adjustment: increased target latency to {} ms (jitter: {:.2f} ms)",
                         m_targetLatencyMs, currentJitter);
        }
    }
    else if (currentJitter < m_targetLatencyMs * 0.2f && m_targetLatencyMs > 50)
    {
        // Low jitter, can reduce buffer
        int newTarget = std::max(50,
                                 m_targetLatencyMs - static_cast<int>(currentJitter * 0.3f));
        if (newTarget != m_targetLatencyMs)
        {
            m_targetLatencyMs = newTarget;
            LOG_INFO_FMT("Adaptive adjustment: decreased target latency to {} ms (jitter: {:.2f} ms)",
                         m_targetLatencyMs, currentJitter);
        }
    }
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
