#ifndef SENTINEL_METRICS_METRICS_COLLECTOR_HPP
#define SENTINEL_METRICS_METRICS_COLLECTOR_HPP

#include <cstdint>
#include <vector>
#include <chrono>
#include <mutex>
#include <atomic>
#include <string>

namespace sentinel::metrics {

struct BenchmarkReport {
    double total_duration_seconds{0.0};
    uint64_t total_locks_submitted{0};
    uint64_t total_mints_completed{0};
    uint64_t total_replays_rejected{0};
    uint64_t total_packets_dropped{0};
    double throughput_locks_per_sec{0.0};
    double throughput_mints_per_sec{0.0};
    double mean_latency_ms{0.0};
    double p99_latency_ms{0.0};
};

class MetricsCollector {
public:
    MetricsCollector();

    void record_lock_submitted();
    void record_mint_completed(double latency_ms);
    void record_replay_rejected();
    void record_packet_dropped();

    void start_timer();
    void stop_timer();

    [[nodiscard]] BenchmarkReport generate_report() const;
    void reset();

private:
    std::chrono::high_resolution_clock::time_point m_start_time;
    std::chrono::high_resolution_clock::time_point m_stop_time;
    bool m_timer_running{false};

    std::atomic<uint64_t> m_locks_submitted{0};
    std::atomic<uint64_t> m_mints_completed{0};
    std::atomic<uint64_t> m_replays_rejected{0};
    std::atomic<uint64_t> m_packets_dropped{0};

    std::vector<double> m_latencies_ms;
    mutable std::mutex m_latency_mutex;
};

} // namespace sentinel::metrics

#endif // SENTINEL_METRICS_METRICS_COLLECTOR_HPP
