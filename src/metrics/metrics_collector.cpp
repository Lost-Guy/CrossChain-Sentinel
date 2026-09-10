#include "sentinel/metrics/metrics_collector.hpp"
#include <algorithm>
#include <numeric>

namespace sentinel::metrics {

MetricsCollector::MetricsCollector() {
    start_timer();
}

void MetricsCollector::record_lock_submitted() {
    m_locks_submitted.fetch_add(1, std::memory_order_relaxed);
}

void MetricsCollector::record_mint_completed(double latency_ms) {
    m_mints_completed.fetch_add(1, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(m_latency_mutex);
    m_latencies_ms.push_back(latency_ms);
}

void MetricsCollector::record_replay_rejected() {
    m_replays_rejected.fetch_add(1, std::memory_order_relaxed);
}

void MetricsCollector::record_packet_dropped() {
    m_packets_dropped.fetch_add(1, std::memory_order_relaxed);
}

void MetricsCollector::start_timer() {
    m_start_time = std::chrono::high_resolution_clock::now();
    m_timer_running = true;
}

void MetricsCollector::stop_timer() {
    m_stop_time = std::chrono::high_resolution_clock::now();
    m_timer_running = false;
}

BenchmarkReport MetricsCollector::generate_report() const {
    auto end_time = m_timer_running ? std::chrono::high_resolution_clock::now() : m_stop_time;
    std::chrono::duration<double> elapsed = end_time - m_start_time;
    double duration_sec = elapsed.count();
    if (duration_sec <= 0.0) duration_sec = 0.000001;

    BenchmarkReport report{
        .total_duration_seconds = duration_sec,
        .total_locks_submitted = m_locks_submitted.load(std::memory_order_relaxed),
        .total_mints_completed = m_mints_completed.load(std::memory_order_relaxed),
        .total_replays_rejected = m_replays_rejected.load(std::memory_order_relaxed),
        .total_packets_dropped = m_packets_dropped.load(std::memory_order_relaxed),
        .throughput_locks_per_sec = static_cast<double>(m_locks_submitted.load()) / duration_sec,
        .throughput_mints_per_sec = static_cast<double>(m_mints_completed.load()) / duration_sec,
        .mean_latency_ms = 0.0,
        .p99_latency_ms = 0.0
    };

    std::vector<double> latencies_copy;
    {
        std::lock_guard<std::mutex> lock(m_latency_mutex);
        latencies_copy = m_latencies_ms;
    }

    if (!latencies_copy.empty()) {
        double sum = std::accumulate(latencies_copy.begin(), latencies_copy.end(), 0.0);
        report.mean_latency_ms = sum / static_cast<double>(latencies_copy.size());

        std::sort(latencies_copy.begin(), latencies_copy.end());
        size_t p99_idx = static_cast<size_t>(0.99 * static_cast<double>(latencies_copy.size() - 1));
        report.p99_latency_ms = latencies_copy[p99_idx];
    }

    return report;
}

void MetricsCollector::reset() {
    m_locks_submitted.store(0);
    m_mints_completed.store(0);
    m_replays_rejected.store(0);
    m_packets_dropped.store(0);
    {
        std::lock_guard<std::mutex> lock(m_latency_mutex);
        m_latencies_ms.clear();
    }
    start_timer();
}

} // namespace sentinel::metrics
