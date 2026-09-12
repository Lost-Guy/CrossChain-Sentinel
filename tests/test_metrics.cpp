#include <gtest/gtest.h>
#include "sentinel/metrics/metrics_collector.hpp"

TEST(MetricsTest, RecordAndGenerateReport) {
    sentinel::metrics::MetricsCollector collector;
    collector.start_timer();

    collector.record_lock_submitted();
    collector.record_lock_submitted();
    collector.record_mint_completed(12.5);
    collector.record_mint_completed(15.0);
    collector.record_replay_rejected();
    collector.record_packet_dropped();

    collector.stop_timer();
    auto report = collector.generate_report();

    EXPECT_EQ(report.total_locks_submitted, 2u);
    EXPECT_EQ(report.total_mints_completed, 2u);
    EXPECT_EQ(report.total_replays_rejected, 1u);
    EXPECT_EQ(report.total_packets_dropped, 1u);
    EXPECT_GT(report.mean_latency_ms, 0.0);
}
