#pragma once

#include <vector>
#include <queue>
#include <map>
#include <cstdint>
#include <chrono>

struct AudioFrame
{
    std::vector<uint8_t> data;
    uint64_t timestamp;
    uint32_t sequence_number;
    std::chrono::steady_clock::time_point receive_time;

    AudioFrame() : timestamp(0), sequence_number(0) {}
    AudioFrame(const std::vector<uint8_t> &frame_data, uint64_t ts, uint32_t seq)
        : data(frame_data), timestamp(ts), sequence_number(seq)
    {
        receive_time = std::chrono::steady_clock::now();
    }
};

class JitterBuffer
{
public:
    JitterBuffer();
    ~JitterBuffer();

    // Core interface
    void PushFrame(const std::vector<uint8_t> &frame, uint64_t timestamp, uint32_t sequence_number);
    bool GetFrame(std::vector<uint8_t> &frame_out);
    void SetTargetLatencyMs(int latency);
    void SetMaxLatencyMs(int max_latency);

    // Configuration
    void SetFrameSize(size_t frame_size);
    void SetSampleRate(uint32_t sample_rate);
    void SetChannels(uint32_t channels);

    // Statistics
    struct BufferStats
    {
        int current_latency_ms;
        int target_latency_ms;
        int buffer_occupancy_percent;
        uint32_t frames_dropped;
        uint32_t frames_reordered;
        uint32_t frames_buffered;
        uint32_t total_frames_received;
        float avg_jitter_ms;
    };
    BufferStats GetStats() const;
    void ResetStats();

    // Buffer management
    void Clear();
    bool IsEmpty() const;
    size_t GetBufferSize() const;

private:
    // Buffer storage
    std::map<uint32_t, AudioFrame> m_frameBuffer;
    std::queue<uint32_t> m_outputQueue;

    // Configuration
    int m_targetLatencyMs;
    int m_maxLatencyMs;
    size_t m_frameSize;
    uint32_t m_sampleRate;
    uint32_t m_channels;

    // Statistics
    mutable BufferStats m_stats;
    uint32_t m_nextExpectedSequence;
    uint32_t m_lastSequenceReceived;

    // Timing
    std::chrono::steady_clock::time_point m_lastFrameTime;
    std::vector<float> m_jitterHistory;

    // Helper methods
    void UpdateStats() const;
    void AdaptiveBufferAdjustment();
    bool IsFrameTooOld(const AudioFrame &frame) const;
    void ReorderFrames();
    float CalculateJitter() const;
    int CalculateCurrentLatency() const;
};
