#include "clock_sync.h"
#include "logger.h"
#include <cmath>
#include <numeric>

ClockSync::ClockSync()
    : m_startTime(std::chrono::high_resolution_clock::now()), m_clockOffset(0), m_driftRate(0.0f), m_syncWindowSize(100), m_minSyncSamples(10), m_driftThreshold(100.0f) // 100 ppm threshold
{
    ResetStats();
    LOG_INFO("Clock synchronization module initialized");
}

ClockSync::~ClockSync()
{
    LOG_INFO("Clock synchronization module destroyed");
}

void ClockSync::SyncWithSender(uint64_t sender_timestamp, uint64_t local_receive_time)
{
    // Create sync sample
    SyncSample sample;
    sample.sender_time = sender_timestamp;
    sample.local_time = local_receive_time;

    // Estimate round-trip time (simplified - assumes symmetric network)
    // In practice, you'd measure actual RTT
    sample.round_trip_time = 1000; // 1ms estimate for local network

    // Calculate drift for this sample
    sample.drift = CalculateDrift(sample);

    // Add to history if valid
    if (IsSampleValid(sample))
    {
        m_syncHistory.push_back(sample);
        CleanupOldSamples();

        // Update drift rate if we have enough samples
        if (m_syncHistory.size() >= m_minSyncSamples)
        {
            UpdateDriftRate();
            UpdateStats();

            // Log sync progress only every 100 samples to reduce flooding
            if (m_syncHistory.size() % 10000 == 0)
            {
                LOG_INFO_FMT("Clock sync: {} samples, drift: {:.2f} ppm, quality: {}%",
                             m_syncHistory.size(), m_driftRate, m_stats.sync_quality);
            }
        }
    }
}

uint64_t ClockSync::GetAdjustedTimestamp(uint64_t sender_timestamp) const
{
    if (!m_stats.is_synchronized)
    {
        // Return local timestamp if not synchronized
        return GetLocalTimestamp();
    }

    // Apply clock offset and drift correction
    uint64_t adjusted = sender_timestamp + m_clockOffset;

    // Apply drift correction (simplified linear correction)
    if (m_driftRate != 0.0f)
    {
        uint64_t timeSinceSync = GetLocalTimestamp() - m_stats.last_sync_time;
        int64_t driftCorrection = static_cast<int64_t>(timeSinceSync * m_driftRate / 1000000.0f);
        adjusted += driftCorrection;
    }

    return adjusted;
}

uint64_t ClockSync::GetLocalTimestamp() const
{
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now - m_startTime;
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

float ClockSync::GetDriftRate() const
{
    return m_driftRate;
}

float ClockSync::GetAverageDrift() const
{
    if (m_syncHistory.empty())
        return 0.0f;

    float sum = 0.0f;
    for (const auto &sample : m_syncHistory)
    {
        sum += sample.drift;
    }
    return sum / m_syncHistory.size();
}

ClockSync::SyncStats ClockSync::GetStats() const
{
    UpdateStats();
    return m_stats;
}

void ClockSync::ResetStats()
{
    m_stats = SyncStats();
    m_stats.is_synchronized = false;
    m_syncHistory.clear();
    LOG_INFO("Clock sync statistics reset");
}

void ClockSync::SetSyncWindowSize(size_t window_size)
{
    m_syncWindowSize = window_size;
    CleanupOldSamples();
    LOG_INFO_FMT("Clock sync window size set to: {}", window_size);
}

void ClockSync::SetMinSyncSamples(size_t min_samples)
{
    m_minSyncSamples = min_samples;
    LOG_INFO_FMT("Clock sync minimum samples set to: {}", min_samples);
}

void ClockSync::SetDriftThreshold(float threshold_ppm)
{
    m_driftThreshold = threshold_ppm;
    LOG_INFO_FMT("Clock sync drift threshold set to: {:.2f} ppm", threshold_ppm);
}

void ClockSync::UpdateDriftRate()
{
    if (m_syncHistory.size() < m_minSyncSamples)
        return;

    // Calculate average drift rate using linear regression
    std::vector<float> drifts;
    for (const auto &sample : m_syncHistory)
    {
        drifts.push_back(sample.drift);
    }

    // Simple average for now (could use more sophisticated algorithms)
    float avgDrift = std::accumulate(drifts.begin(), drifts.end(), 0.0f) / drifts.size();

    // Convert to ppm (parts per million)
    m_driftRate = avgDrift * 1000000.0f / 1000000.0f; // Assuming microsecond precision

    // Update clock offset
    if (!m_syncHistory.empty())
    {
        const auto &latest = m_syncHistory.back();
        m_clockOffset = latest.local_time - latest.sender_time;
    }
}

void ClockSync::UpdateStats() const
{
    m_stats.drift_rate_ppm = m_driftRate;
    m_stats.avg_drift_ms = GetAverageDrift() / 1000.0f; // Convert to milliseconds
    m_stats.sync_samples = static_cast<uint32_t>(m_syncHistory.size());
    m_stats.last_sync_time = GetLocalTimestamp();

    // Calculate sync quality (0-100)
    if (m_syncHistory.size() >= m_minSyncSamples)
    {
        // Quality based on sample count and drift stability
        float sampleQuality = std::min(100.0f, static_cast<float>(m_syncHistory.size()) / m_syncWindowSize * 100.0f);
        float driftQuality = std::max(0.0f, 100.0f - std::abs(m_driftRate) / m_driftThreshold * 100.0f);
        m_stats.sync_quality = static_cast<uint32_t>((sampleQuality + driftQuality) / 2.0f);
        m_stats.is_synchronized = m_stats.sync_quality > 50;
    }
    else
    {
        m_stats.sync_quality = 0;
        m_stats.is_synchronized = false;
    }
}

float ClockSync::CalculateDrift(const SyncSample &sample) const
{
    // Calculate clock drift: (local_time - sender_time) / time_elapsed
    // This is a simplified calculation
    if (sample.sender_time == 0)
        return 0.0f;

    int64_t timeDiff = static_cast<int64_t>(sample.local_time) - static_cast<int64_t>(sample.sender_time);
    return static_cast<float>(timeDiff);
}

bool ClockSync::IsSampleValid(const SyncSample &sample) const
{
    // Basic validation
    if (sample.sender_time == 0)
        return false;

    // Check for reasonable time differences (not too large)
    int64_t timeDiff = std::abs(static_cast<int64_t>(sample.local_time) - static_cast<int64_t>(sample.sender_time));
    if (timeDiff > 1000000) // More than 1 second difference
        return false;

    return true;
}

void ClockSync::CleanupOldSamples()
{
    while (m_syncHistory.size() > m_syncWindowSize)
    {
        m_syncHistory.pop_front();
    }
}

void ClockSync::PrintSummary() const
{
    UpdateStats();

    LOG_INFO("=== Clock Sync Summary ===");
    LOG_INFO_FMT("Synchronized: {}", m_stats.is_synchronized ? "Yes" : "No");
    LOG_INFO_FMT("Sync Quality: {}%", m_stats.sync_quality);
    LOG_INFO_FMT("Sync Samples: {}", m_stats.sync_samples);
    LOG_INFO_FMT("Drift Rate: {:.2f} ppm", m_stats.drift_rate_ppm);
    LOG_INFO_FMT("Average Drift: {:.2f} ms", m_stats.avg_drift_ms);
    LOG_INFO_FMT("Clock Offset: {} us", m_clockOffset);
    LOG_INFO("=========================");
}

std::string ClockSync::GetSummaryString() const
{
    UpdateStats();

    std::string summary = "Clock Sync: ";
    summary += m_stats.is_synchronized ? "SYNCED" : "UNSYNCED";
    summary += " | Quality: " + std::to_string(m_stats.sync_quality) + "%";
    summary += " | Drift: " + std::to_string(static_cast<int>(m_stats.drift_rate_ppm)) + " ppm";
    summary += " | Samples: " + std::to_string(m_stats.sync_samples);

    return summary;
}
