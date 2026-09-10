#include "sentinel/metrics/benchmark_engine.hpp"
#include "sentinel/crypto/crypto_engine.hpp"
#include "sentinel/core/logger.hpp"

namespace sentinel::metrics {

BenchmarkEngine::BenchmarkEngine(
    std::shared_ptr<sentinel::ledger::ChainState> chain_a,
    std::shared_ptr<sentinel::ledger::ChainState> chain_b,
    std::shared_ptr<sentinel::relayer::RelayerEngine> relayer_engine)
    : m_chain_a(std::move(chain_a)),
      m_chain_b(std::move(chain_b)),
      m_relayer_engine(std::move(relayer_engine)) {}

BenchmarkReport BenchmarkEngine::run_throughput_benchmark(size_t num_threads, size_t locks_per_thread) {
    m_collector.reset();
    LOG_INFO("Starting Concurrency Benchmark: {} worker threads, {} transactions/thread...", num_threads, locks_per_thread);

    std::vector<std::thread> workers;
    workers.reserve(num_threads);

    for (size_t t = 0; t < num_threads; ++t) {
        workers.emplace_back([this, locks_per_thread]() {
            auto user_kp = sentinel::crypto::CryptoEngine::generate_keypair();
            m_chain_a->state().set_balance(user_kp.public_key, 1000000000);

            for (size_t i = 0; i < locks_per_thread; ++i) {
                sentinel::ledger::Transaction tx{
                    .chain_id = m_chain_a->chain_id(),
                    .type = sentinel::ledger::TxType::BRIDGE_LOCK,
                    .nonce = static_cast<uint64_t>(i),
                    .sender = user_kp.public_key,
                    .recipient = user_kp.public_key,
                    .amount = 100,
                    .fee = 1,
                    .lock_event_nullifier = {},
                    .signature = {}
                };

                if (m_chain_a->submit_transaction(tx)) {
                    m_collector.record_lock_submitted();
                }
            }
        });
    }

    for (auto& w : workers) {
        if (w.joinable()) w.join();
    }

    // Process blocks and relay to Chain B
    m_chain_a->produce_block(10000);

    auto start_relay = std::chrono::high_resolution_clock::now();
    (void)m_relayer_engine->poll_and_process_events(*m_chain_a);
    size_t submitted = m_relayer_engine->submit_pending_mints(*m_chain_b);
    m_chain_b->produce_block(10010);
    auto end_relay = std::chrono::high_resolution_clock::now();

    double relay_duration_ms = std::chrono::duration<double, std::milli>(end_relay - start_relay).count();
    for (size_t i = 0; i < submitted; ++i) {
        m_collector.record_mint_completed(relay_duration_ms / static_cast<double>(submitted > 0 ? submitted : 1));
    }

    m_collector.stop_timer();
    auto report = m_collector.generate_report();

    LOG_INFO("Benchmark Completed: Locks Submitted={}, Mints Completed={}, Duration={:.4f}s, Locks/sec={:.2f}, Mints/sec={:.2f}",
             report.total_locks_submitted, report.total_mints_completed,
             report.total_duration_seconds, report.throughput_locks_per_sec, report.throughput_mints_per_sec);

    return report;
}

} // namespace sentinel::metrics
