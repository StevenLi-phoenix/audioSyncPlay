#pragma once

#include <chrono>
#include <cstdint>
#include <vector>
#include <deque>
#include <algorithm>
#include <string>

class ClockSync
{
public:
    ClockSync();
    ~ClockSync();

    // Clock synchronization
    void SyncWithSender(uint64_t sender_timestamp, uint64_t local_receive_time);
    uint64_t GetAdjustedTimestamp(uint64_t sender_timestamp) const;
    uint64_t GetLocalTimestamp() const;

    // Drift calculation
    float GetDriftRate() const;
    float GetAverageDrift() const;

    // Statistics
    struct SyncStats
    {
        float drift_rate_ppm;    // Drift rate in parts per million
        float avg_drift_ms;      // Average drift in milliseconds
        uint32_t sync_samples;   // Number of sync samples
        uint32_t sync_quality;   // Sync quality (0-100)
        uint64_t last_sync_time; // Last sync timestamp
        bool is_synchronized;    // Whether clock is synchronized
    };

    SyncStats GetStats() const;
    void ResetStats();

    // Summary and reporting
    void PrintSummary() const;
    std::string GetSummaryString() const;

    // Configuration
    void SetSyncWindowSize(size_t window_size);
    void SetMinSyncSamples(size_t min_samples);
    void SetDriftThreshold(float threshold_ppm);

private:
    // Clock state
    mutable std::chrono::high_resolution_clock::time_point m_startTime;
    uint64_t m_clockOffset; // Offset between sender and receiver clocks
    float m_driftRate;      // Clock drift rate (ppm)

    // Sync history
    struct SyncSample
    {
        uint64_t sender_time;
        uint64_t local_time;
        uint64_t round_trip_time;
        float drift;
    };

    std::deque<SyncSample> m_syncHistory;
    size_t m_syncWindowSize;
    size_t m_minSyncSamples;
    float m_driftThreshold;

    // Statistics
    mutable SyncStats m_stats;

    // Helper methods
    void UpdateDriftRate();
    void UpdateStats() const;
    float CalculateDrift(const SyncSample &sample) const;
    bool IsSampleValid(const SyncSample &sample) const;
    void CleanupOldSamples();
};
