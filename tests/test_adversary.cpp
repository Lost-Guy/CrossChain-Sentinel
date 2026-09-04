#include <gtest/gtest.h>
#include "sentinel/adversary/fault_injector.hpp"
#include "sentinel/adversary/attack_harness.hpp"
#include "sentinel/crypto/crypto_engine.hpp"

class AdversaryTest : public ::testing::Test {
protected:
    void SetUp() override {
        sentinel::crypto::CryptoEngine::init();
        rng = std::make_shared<sentinel::core::RandomEngine>(1337);
        fault_injector = std::make_shared<sentinel::adversary::FaultInjector>(rng);

        chain_a = std::make_shared<sentinel::ledger::ChainState>(1);
        chain_b = std::make_shared<sentinel::ledger::ChainState>(137);

        victim_kp = sentinel::crypto::CryptoEngine::generate_keypair();
        chain_a->state().set_balance(victim_kp.public_key, 100000);

        std::vector<std::shared_ptr<sentinel::relayer::RelayerNode>> relayers;
        for (uint32_t i = 1; i <= 5; ++i) {
            relayers.push_back(std::make_shared<sentinel::relayer::RelayerNode>(
                i, sentinel::crypto::CryptoEngine::generate_keypair()
            ));
        }

        relayer_engine = std::make_shared<sentinel::relayer::RelayerEngine>(1, 137, 3, relayers);
    }

    std::shared_ptr<sentinel::core::RandomEngine> rng;
    std::shared_ptr<sentinel::adversary::FaultInjector> fault_injector;
    std::shared_ptr<sentinel::ledger::ChainState> chain_a;
    std::shared_ptr<sentinel::ledger::ChainState> chain_b;
    std::shared_ptr<sentinel::relayer::RelayerEngine> relayer_engine;
    sentinel::crypto::KeyPair victim_kp;
};

TEST_F(AdversaryTest, NetworkPartitionFiltering) {
    fault_injector->create_partition({1, 2}, {3, 4});

    EXPECT_TRUE(fault_injector->is_partitioned(1, 3));
    EXPECT_TRUE(fault_injector->is_partitioned(2, 4));
    EXPECT_FALSE(fault_injector->is_partitioned(1, 2));
    EXPECT_FALSE(fault_injector->is_partitioned(3, 4));

    fault_injector->heal_partitions();
    EXPECT_FALSE(fault_injector->is_partitioned(1, 3));
}

TEST_F(AdversaryTest, ReplayAttackBlockedByNullifier) {
    sentinel::adversary::AttackHarness harness(chain_a, chain_b, relayer_engine, fault_injector);

    // Submit legitimate lock
    sentinel::ledger::Transaction lock_tx{
        .chain_id = 1,
        .type = sentinel::ledger::TxType::BRIDGE_LOCK,
        .nonce = 0,
        .sender = victim_kp.public_key,
        .recipient = victim_kp.public_key,
        .amount = 10000,
        .fee = 100
    };

    chain_a->submit_transaction(lock_tx);
    chain_a->produce_block(1000);

    auto events = chain_a->get_lock_events_since(0);
    ASSERT_EQ(events.size(), 1u);

    // Initial mint execution
    relayer_engine->poll_and_process_events(*chain_a);
    relayer_engine->submit_pending_mints(*chain_b);
    chain_b->produce_block(1010);
    EXPECT_EQ(chain_b->state().get_balance(victim_kp.public_key), 10000u);

    // Launch Replay Attack 5 times with intercepted event payload
    auto attack_result = harness.execute_replay_attack(events[0], 5);
    
    EXPECT_FALSE(attack_result.attack_successful);
    EXPECT_EQ(attack_result.duplicate_mints_executed, 0u);
    EXPECT_EQ(chain_b->state().get_balance(victim_kp.public_key), 10000u);
}

TEST_F(AdversaryTest, PartitionDoubleSpendDefense) {
    sentinel::adversary::AttackHarness harness(chain_a, chain_b, relayer_engine, fault_injector);

    auto attack_result = harness.execute_partition_double_spend(victim_kp, 5000, {4, 5});

    EXPECT_FALSE(attack_result.attack_successful);
    EXPECT_EQ(chain_b->state().get_balance(victim_kp.public_key), 5000u);
}
