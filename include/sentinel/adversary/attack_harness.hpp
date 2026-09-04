#ifndef SENTINEL_ADVERSARY_ATTACK_HARNESS_HPP
#define SENTINEL_ADVERSARY_ATTACK_HARNESS_HPP

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include "sentinel/ledger/chain_state.hpp"
#include "sentinel/relayer/relayer_engine.hpp"
#include "sentinel/adversary/fault_injector.hpp"

namespace sentinel::adversary {

struct AttackResult {
    bool replay_attempted{false};
    bool attack_successful{false};
    size_t duplicate_mints_executed{0};
    std::string report{};
};

class AttackHarness {
public:
    AttackHarness(
        std::shared_ptr<sentinel::ledger::ChainState> chain_a,
        std::shared_ptr<sentinel::ledger::ChainState> chain_b,
        std::shared_ptr<sentinel::relayer::RelayerEngine> relayer_engine,
        std::shared_ptr<FaultInjector> fault_injector
    );

    // Replay attack: Intercept a valid lock event nullifier and replay it multiple times
    AttackResult execute_replay_attack(const sentinel::ledger::LockEvent& target_event, size_t replay_count);

    // Split-Brain Double Spend: Trigger lock event while network is partitioned and execute rogue relayer mints
    AttackResult execute_partition_double_spend(
        const sentinel::crypto::KeyPair& victim_kp,
        uint64_t amount,
        const std::vector<uint32_t>& rogue_relayer_ids
    );

private:
    std::shared_ptr<sentinel::ledger::ChainState> m_chain_a;
    std::shared_ptr<sentinel::ledger::ChainState> m_chain_b;
    std::shared_ptr<sentinel::relayer::RelayerEngine> m_relayer_engine;
    std::shared_ptr<FaultInjector> m_fault_injector;
};

} // namespace sentinel::adversary

#endif // SENTINEL_ADVERSARY_ATTACK_HARNESS_HPP
