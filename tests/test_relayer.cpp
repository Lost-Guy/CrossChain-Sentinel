#include <gtest/gtest.h>
#include "sentinel/relayer/relayer_engine.hpp"
#include "sentinel/ledger/chain_state.hpp"
#include "sentinel/crypto/crypto_engine.hpp"

class RelayerTest : public ::testing::Test {
protected:
    void SetUp() override {
        sentinel::crypto::CryptoEngine::init();
        sender_kp = sentinel::crypto::CryptoEngine::generate_keypair();
        recipient_kp = sentinel::crypto::CryptoEngine::generate_keypair();
    }

    sentinel::crypto::KeyPair sender_kp;
    sentinel::crypto::KeyPair recipient_kp;
};

TEST_F(RelayerTest, EndToEndCrossChainRelay) {
    // 1. Setup Chain A (Source: ID 1) and Chain B (Destination: ID 137)
    sentinel::ledger::ChainState chain_a(1);
    sentinel::ledger::ChainState chain_b(137);

    chain_a.state().set_balance(sender_kp.public_key, 10000);

    // 2. Setup 5 Relayer Nodes with a 3-of-5 threshold
    std::vector<std::shared_ptr<sentinel::relayer::RelayerNode>> relayers;
    for (uint32_t i = 1; i <= 5; ++i) {
        relayers.push_back(std::make_shared<sentinel::relayer::RelayerNode>(
            i, sentinel::crypto::CryptoEngine::generate_keypair()
        ));
    }

    sentinel::relayer::RelayerEngine engine(1, 137, 3, relayers);
    EXPECT_EQ(engine.relayer_count(), 5u);

    // 3. User locks funds on Chain A
    sentinel::ledger::Transaction lock_tx{
        .chain_id = 1,
        .type = sentinel::ledger::TxType::BRIDGE_LOCK,
        .nonce = 0,
        .sender = sender_kp.public_key,
        .recipient = recipient_kp.public_key,
        .amount = 5000,
        .fee = 100
    };

    EXPECT_TRUE(chain_a.submit_transaction(lock_tx));
    chain_a.produce_block(5000);
    EXPECT_EQ(chain_a.state().get_balance(sender_kp.public_key), 4900u);

    // 4. Relayer engine polls events and collects validator signatures
    size_t processed = engine.poll_and_process_events(chain_a);
    EXPECT_EQ(processed, 1u);

    // 5. Relayer engine submits MINT proof to Chain B mempool
    size_t submitted = engine.submit_pending_mints(chain_b);
    EXPECT_EQ(submitted, 1u);

    // 6. Chain B produces a block executing the MINT transaction
    chain_b.produce_block(5010);
    EXPECT_EQ(chain_b.state().get_balance(recipient_kp.public_key), 5000u);
}
