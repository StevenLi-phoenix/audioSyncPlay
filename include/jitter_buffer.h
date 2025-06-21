#pragma once

#include <vector>
#include <queue>
#include <map>
#include <cstdint>
#include <mutex>
#include <atomic>
#include <chrono>

/**
 * @brief Jitter buffer for audio frame management
 *
 * This class provides intelligent buffering of audio frames to handle
 * network jitter and ensure smooth playback with configurable latency.
 */
class JitterBuffer
{
public:
    // Audio frame with metadata
    struct AudioFrame
    {
        std::vector<uint8_t> data;
        uint64_t timestamp;
        uint32_t sequence;
        bool isValid;

        AudioFrame() : timestamp(0), sequence(0), isValid(false) {}
        AudioFrame(const std::vector<uint8_t> &frameData, uint64_t ts, uint32_t seq)
            : data(frameData), timestamp(ts), sequence(seq), isValid(true) {}
    };

    // Buffer statistics
    struct BufferStats
    {
        int currentLatencyMs = 0;
        int targetLatencyMs = 100;
        int maxLatencyMs = 200;
        int bufferOccupancyPercent = 0;
        uint32_t framesDropped = 0;
        uint32_t framesReordered = 0;
        uint32_t framesBuffered = 0;
        uint32_t totalFramesReceived = 0;
        float avgJitterMs = 0.0f;
        bool isUnderrun = false;
        bool isOverrun = false;
    };

    // Configuration
    struct BufferConfig
    {
        int targetLatencyMs = 100;
        int maxLatencyMs = 200;
        int minLatencyMs = 50;
        int adaptiveThresholdMs = 20;
        bool enableAdaptiveLatency = true;
        bool enableFrameReordering = true;
        size_t maxBufferSize = 1000; // Maximum frames in buffer
    };

    JitterBuffer();
    ~JitterBuffer();

    /**
     * @brief Push audio frame into buffer
     * @param frame Audio frame data
     * @param timestamp Frame timestamp
     * @param sequence Frame sequence number
     */
    void PushFrame(const std::vector<uint8_t> &frame, uint64_t timestamp, uint32_t sequence = 0);

    /**
     * @brief Get next audio frame for playback
     * @param frame_out Output buffer for audio frame
     * @return true if frame retrieved successfully
     */
    bool GetFrame(std::vector<uint8_t> &frame_out);

    /**
     * @brief Set target latency
     * @param latencyMs Target latency in milliseconds
     */
    void SetTargetLatencyMs(int latencyMs);

    /**
     * @brief Set maximum latency
     * @param latencyMs Maximum latency in milliseconds
     */
    void SetMaxLatencyMs(int latencyMs);

    /**
     * @brief Set buffer configuration
     * @param config Buffer configuration
     */
    void SetConfig(const BufferConfig &config);

    /**
     * @brief Get current buffer statistics
     * @return Buffer statistics
     */
    BufferStats GetStats() const;

    /**
     * @brief Reset buffer and statistics
     */
    void Reset();

    /**
     * @brief Check if buffer has enough data for playback
     * @return true if buffer is ready
     */
    bool IsReady() const;

    /**
     * @brief Get current buffer occupancy percentage
     * @return Occupancy percentage (0-100)
     */
    int GetOccupancyPercent() const;

    /**
     * @brief Flush buffer (remove all frames)
     */
    void Flush();

private:
    // Private methods
    void UpdateLatency();
    void AdaptiveLatencyAdjustment();
    void CleanupOldFrames();
    bool ShouldDropFrame(const AudioFrame &frame) const;
    void UpdateStatistics();
    uint64_t GetCurrentTimestamp() const;

    // Frame ordering and validation
    void InsertFrameInOrder(const AudioFrame &frame);
    bool IsFrameInOrder(uint32_t sequence) const;
    void HandleOutOfOrderFrame(const AudioFrame &frame);

    // Helper methods
    int CalculateLatencyMs(uint64_t timestamp) const;
    float CalculateJitter() const;

    // Member variables
    std::map<uint64_t, AudioFrame> frameBuffer_; // Ordered by timestamp
    std::queue<AudioFrame> playbackQueue_;       // Ready for playback

    BufferConfig config_;
    BufferStats stats_;

    // Threading
    mutable std::mutex bufferMutex_;
    std::atomic<bool> isReady_;

    // Timing
    uint64_t lastPlaybackTime_;
    uint64_t lastFrameTime_;
    std::vector<float> jitterHistory_;

    // State
    uint32_t expectedSequence_;
    uint32_t lastSequence_;
    bool firstFrameReceived_;

    // Constants
    static constexpr size_t JITTER_HISTORY_SIZE = 50;
    static constexpr uint64_t MAX_FRAME_AGE_MS = 5000; // 5 seconds
    static constexpr float LATENCY_ADJUSTMENT_FACTOR = 0.1f;
};
