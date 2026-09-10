#ifndef SENTINEL_METRICS_BENCHMARK_ENGINE_HPP
#define SENTINEL_METRICS_BENCHMARK_ENGINE_HPP

#include <cstdint>
#include <memory>
#include <thread>
#include <vector>
#include "sentinel/metrics/metrics_collector.hpp"
#include "sentinel/ledger/chain_state.hpp"
#include "sentinel/relayer/relayer_engine.hpp"

namespace sentinel::metrics {

class BenchmarkEngine {
public:
    BenchmarkEngine(
        std::shared_ptr<sentinel::ledger::ChainState> chain_a,
        std::shared_ptr<sentinel::ledger::ChainState> chain_b,
        std::shared_ptr<sentinel::relayer::RelayerEngine> relayer_engine
    );

    // Run parallel multi-threaded load test simulating concurrent user bridge locks
    BenchmarkReport run_throughput_benchmark(size_t num_threads, size_t locks_per_thread);

    [[nodiscard]] MetricsCollector& collector() { return m_collector; }

private:
    std::shared_ptr<sentinel::ledger::ChainState> m_chain_a;
    std::shared_ptr<sentinel::ledger::ChainState> m_chain_b;
    std::shared_ptr<sentinel::relayer::RelayerEngine> m_relayer_engine;
    MetricsCollector m_collector;
};

} // namespace sentinel::metrics

#endif // SENTINEL_METRICS_BENCHMARK_ENGINE_HPP
