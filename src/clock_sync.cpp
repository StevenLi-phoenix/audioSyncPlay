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

    // OPTIMIZATION: Estimate round-trip time based on timestamp differences
    // This is a simplified approach - in production, use proper RTT measurement
    if (m_syncHistory.size() > 0)
    {
        // Calculate RTT based on timestamp variance
        auto &lastSample = m_syncHistory.back();
        uint64_t timeDiff = (local_receive_time > lastSample.local_time) ?
                           (local_receive_time - lastSample.local_time) : 0;
        uint64_t senderDiff = (sender_timestamp > lastSample.sender_time) ?
                             (sender_timestamp - lastSample.sender_time) : 0;

        if (timeDiff > 0 && senderDiff > 0)
        {
            // Simple RTT estimation based on time differences
            sample.round_trip_time = std::abs(static_cast<int64_t>(timeDiff - senderDiff));
            sample.round_trip_time = std::max(100ULL, std::min(10000ULL, sample.round_trip_time)); // Clamp 0.1-10ms
        }
        else
        {
            sample.round_trip_time = 1000; // 1ms default
        }
    }
    else
    {
        sample.round_trip_time = 1000; // 1ms default for first sample
    }

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

    // OPTIMIZATION: Add overflow protection to timestamp calculations
    int64_t senderTime = static_cast<int64_t>(sender_timestamp);
    int64_t offsetTime = senderTime + static_cast<int64_t>(m_clockOffset);

    // Check for overflow/underflow
    if (offsetTime < 0)
    {
        LOG_WARNING("Clock offset caused underflow, using sender timestamp");
        return sender_timestamp;
    }

    // Apply drift correction with overflow protection
    if (m_driftRate != 0.0f && m_stats.last_sync_time != 0)
    {
        uint64_t currentTime = GetLocalTimestamp();
        if (currentTime > m_stats.last_sync_time)
        {
            uint64_t timeSinceSync = currentTime - m_stats.last_sync_time;

            // Limit time since sync to prevent excessive drift correction
            timeSinceSync = std::min(timeSinceSync, 10000000ULL); // Max 10 seconds

            // Calculate drift correction with safe arithmetic
            double driftCorrectionFloat = static_cast<double>(timeSinceSync) * m_driftRate / 1000000.0;
            int64_t driftCorrection = static_cast<int64_t>(driftCorrectionFloat);

            // Apply correction with overflow check
            int64_t finalTime = offsetTime + driftCorrection;

            // Check for overflow
            if ((driftCorrection > 0 && finalTime < offsetTime) ||
                (driftCorrection < 0 && finalTime > offsetTime))
            {
                LOG_WARNING("Drift correction caused overflow, using base offset");
                return static_cast<uint64_t>(std::max(0LL, offsetTime));
            }

            return static_cast<uint64_t>(std::max(0LL, finalTime));
        }
    }

    return static_cast<uint64_t>(std::max(0LL, offsetTime));
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

    // OPTIMIZATION: Use linear regression for more accurate drift calculation
    size_t n = m_syncHistory.size();
    if (n < 2) return;

    // Calculate linear regression: y = ax + b, where x is time, y is drift
    double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
    uint64_t baseTime = m_syncHistory[0].local_time;

    // Use the last N samples for better accuracy
    size_t startIdx = (n > 50) ? n - 50 : 0;
    size_t count = n - startIdx;

    for (size_t i = startIdx; i < n; ++i)
    {
        const auto &sample = m_syncHistory[i];

        // Use relative time to avoid overflow
        double x = static_cast<double>(sample.local_time - baseTime) / 1000000.0; // Convert to seconds
        double y = sample.drift / 1000.0; // Convert to milliseconds

        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumX2 += x * x;
    }

    // Calculate slope (drift rate) using least squares
    double denominator = count * sumX2 - sumX * sumX;
    if (std::abs(denominator) > 1e-10) // Avoid division by zero
    {
        double slope = (count * sumXY - sumX * sumY) / denominator; // ms/s
        double intercept = (sumY - slope * sumX) / count;

        // Convert slope to ppm (parts per million)
        m_driftRate = static_cast<float>(slope * 1000.0); // Convert ms/s to ppm

        // Clamp drift rate to reasonable bounds
        m_driftRate = std::max(-1000.0f, std::min(1000.0f, m_driftRate)); // ±1000 ppm max

        // Update clock offset using the latest sample with RTT compensation
        if (!m_syncHistory.empty())
        {
            const auto &latest = m_syncHistory.back();
            int64_t offsetWithRTT = static_cast<int64_t>(latest.local_time) -
                                   static_cast<int64_t>(latest.sender_time) -
                                   static_cast<int64_t>(latest.round_trip_time / 2);
            m_clockOffset = offsetWithRTT;
        }

        LOG_DEBUG_FMT("Clock sync regression: slope={:.3f} ppm, intercept={:.3f} ms, samples={}",
                      m_driftRate, intercept, count);
    }
    else
    {
        // Fallback to simple average if regression fails
        double avgDrift = sumY / count;
        m_driftRate = static_cast<float>(avgDrift * 1000.0); // Convert to ppm

        LOG_DEBUG_FMT("Clock sync fallback: avg drift={:.3f} ppm, samples={}", m_driftRate, count);
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
