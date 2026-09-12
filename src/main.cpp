#include <iostream>
#include <memory>
#include <vector>
#include "sentinel/core/logger.hpp"
#include "sentinel/crypto/crypto_engine.hpp"
#include "sentinel/ledger/chain_state.hpp"
#include "sentinel/relayer/relayer_engine.hpp"
#include "sentinel/adversary/fault_injector.hpp"
#include "sentinel/adversary/attack_harness.hpp"
#include "sentinel/metrics/benchmark_engine.hpp"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    sentinel::core::Logger::init("SENTINEL", spdlog::level::info);
    LOG_INFO("======================================================================");
    LOG_INFO("   CROSSCHAIN-SENTINEL: P2P Cross-Chain Security & Replay Simulator   ");
    LOG_INFO("======================================================================");

    if (!sentinel::crypto::CryptoEngine::init()) {
        std::cerr << "Failed to initialize cryptographic engine." << std::endl;
        return 1;
    }

    // 1. Initialize Dual Chain State Engines
    auto chain_a = std::make_shared<sentinel::ledger::ChainState>(1);   // Source Chain (Ethereum)
    auto chain_b = std::make_shared<sentinel::ledger::ChainState>(137); // Destination Chain (Polygon)

    // 2. Spawn Decentralized Relayer Network Pool (5 Relayers, 3-of-5 Quorum)
    std::vector<std::shared_ptr<sentinel::relayer::RelayerNode>> relayers;
    for (uint32_t i = 1; i <= 5; ++i) {
        relayers.push_back(std::make_shared<sentinel::relayer::RelayerNode>(
            i, sentinel::crypto::CryptoEngine::generate_keypair()
        ));
    }

    auto relayer_engine = std::make_shared<sentinel::relayer::RelayerEngine>(1, 137, 3, relayers);
    auto rng = std::make_shared<sentinel::core::RandomEngine>(42);
    auto fault_injector = std::make_shared<sentinel::adversary::FaultInjector>(rng);

    // 3. Execute Concurrency Benchmark Workload
    sentinel::metrics::BenchmarkEngine bench_engine(chain_a, chain_b, relayer_engine);
    auto bench_report = bench_engine.run_throughput_benchmark(4, 250);

    // 4. Run Adversarial Replay Attack Simulation
    sentinel::adversary::AttackHarness attack_harness(chain_a, chain_b, relayer_engine, fault_injector);

    auto alice_kp = sentinel::crypto::CryptoEngine::generate_keypair();
    chain_a->state().set_balance(alice_kp.public_key, 50000);

    sentinel::ledger::Transaction lock_tx{
        .chain_id = 1,
        .type = sentinel::ledger::TxType::BRIDGE_LOCK,
        .nonce = 0,
        .sender = alice_kp.public_key,
        .recipient = alice_kp.public_key,
        .amount = 10000,
        .fee = 10,
        .lock_event_nullifier = {},
        .signature = {}
    };

    chain_a->submit_transaction(lock_tx);
    chain_a->produce_block(50000);

    relayer_engine->poll_and_process_events(*chain_a);
    relayer_engine->submit_pending_mints(*chain_b);
    chain_b->produce_block(50010);

    auto lock_events = chain_a->get_lock_events_since(0);
    if (!lock_events.empty()) {
        LOG_INFO("Executing Adversarial Replay Attack Test...");
        auto attack_res = attack_harness.execute_replay_attack(lock_events.back(), 5);
        LOG_INFO("Adversary Attack Result: {}", attack_res.report);
    }

    // 5. Output Summary Metrics Report
    LOG_INFO("----------------------------------------------------------------------");
    LOG_INFO("                  BENCHMARK & RESILIENCE REPORT                       ");
    LOG_INFO("----------------------------------------------------------------------");
    LOG_INFO(" Total Simulated Lock Txs  : {}", bench_report.total_locks_submitted);
    LOG_INFO(" Total Completed Mint Txs  : {}", bench_report.total_mints_completed);
    LOG_INFO(" Peak Lock Throughput      : {:.2f} tx/sec", bench_report.throughput_locks_per_sec);
    LOG_INFO(" Peak Mint Throughput      : {:.2f} tx/sec", bench_report.throughput_mints_per_sec);
    LOG_INFO(" Mean Relay Processing Time: {:.4f} ms", bench_report.mean_latency_ms);
    LOG_INFO("----------------------------------------------------------------------");
    LOG_INFO("CROSSCHAIN-SENTINEL Simulation Execution Finished Successfully.");

    return 0;
}
