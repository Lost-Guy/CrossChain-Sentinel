#include "sentinel/adversary/attack_harness.hpp"
#include "sentinel/core/logger.hpp"

namespace sentinel::adversary {

AttackHarness::AttackHarness(
    std::shared_ptr<sentinel::ledger::ChainState> chain_a,
    std::shared_ptr<sentinel::ledger::ChainState> chain_b,
    std::shared_ptr<sentinel::relayer::RelayerEngine> relayer_engine,
    std::shared_ptr<FaultInjector> fault_injector)
    : m_chain_a(std::move(chain_a)),
      m_chain_b(std::move(chain_b)),
      m_relayer_engine(std::move(relayer_engine)),
      m_fault_injector(std::move(fault_injector)) {}

AttackResult AttackHarness::execute_replay_attack(const sentinel::ledger::LockEvent& target_event, size_t replay_count) {
    AttackResult result{
        .replay_attempted = true,
        .attack_successful = false,
        .duplicate_mints_executed = 0,
        .report = ""
    };

    uint64_t initial_balance = m_chain_b->state().get_balance(target_event.recipient);

    LOG_WARN("Adversary launching REPLAY ATTACK: Replaying nullifier {} times on Chain {}", 
             replay_count, m_chain_b->chain_id());

    for (size_t i = 0; i < replay_count; ++i) {
        sentinel::ledger::Transaction replay_tx{
            .chain_id = m_chain_b->chain_id(),
            .type = sentinel::ledger::TxType::BRIDGE_MINT,
            .nonce = 9000 + static_cast<uint64_t>(i),
            .sender = target_event.recipient,
            .recipient = target_event.recipient,
            .amount = target_event.amount,
            .fee = 0,
            .lock_event_nullifier = target_event.lock_nullifier
        };

        if (m_chain_b->submit_transaction(replay_tx)) {
            m_chain_b->produce_block(20000 + static_cast<uint64_t>(i * 10));
        }
    }

    uint64_t final_balance = m_chain_b->state().get_balance(target_event.recipient);
    uint64_t excess_minted = (final_balance > initial_balance) ? (final_balance - initial_balance) : 0;

    if (excess_minted > target_event.amount) {
        result.attack_successful = true;
        result.duplicate_mints_executed = (excess_minted / target_event.amount) - 1;
        result.report = "CRITICAL: Replay attack succeeded! Double mint occurred on Chain B.";
        LOG_CRITICAL("{}", result.report);
    } else {
        result.attack_successful = false;
        result.duplicate_mints_executed = 0;
        result.report = "SUCCESS: Defense held. Nullifier index rejected replayed mint proofs.";
        LOG_INFO("{}", result.report);
    }

    return result;
}

AttackResult AttackHarness::execute_partition_double_spend(
    const sentinel::crypto::KeyPair& victim_kp,
    uint64_t amount,
    const std::vector<uint32_t>& rogue_relayer_ids) {

    AttackResult result{
        .replay_attempted = true,
        .attack_successful = false,
        .duplicate_mints_executed = 0,
        .report = ""
    };

    LOG_WARN("Adversary triggering Partition-Window Double Spend scenario...");

    // 1. Isolate rogue relayers into Partition Group B
    std::vector<uint32_t> honest_relayers = {1, 2, 3};
    m_fault_injector->create_partition(honest_relayers, rogue_relayer_ids);

    // 2. Lock event submitted on Chain A
    sentinel::ledger::Transaction lock_tx{
        .chain_id = m_chain_a->chain_id(),
        .type = sentinel::ledger::TxType::BRIDGE_LOCK,
        .nonce = 0,
        .sender = victim_kp.public_key,
        .recipient = victim_kp.public_key,
        .amount = amount,
        .fee = 50
    };

    m_chain_a->submit_transaction(lock_tx);
    m_chain_a->produce_block(30000);

    auto lock_events = m_chain_a->get_lock_events_since(0);
    if (lock_events.empty()) {
        result.report = "Error: Failed to register lock event.";
        return result;
    }

    auto target_event = lock_events.back();

    // 3. Process honest relay mint
    m_relayer_engine->poll_and_process_events(*m_chain_a);
    m_relayer_engine->submit_pending_mints(*m_chain_b);
    m_chain_b->produce_block(30010);

    // 4. Attempt rogue double-spend submission across partitioned segment
    return execute_replay_attack(target_event, 3);
}

} // namespace sentinel::adversary
