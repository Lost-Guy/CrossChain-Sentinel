#include <gtest/gtest.h>
#include "sentinel/ledger/chain_state.hpp"
#include "sentinel/crypto/crypto_engine.hpp"

class LedgerTest : public ::testing::Test {
protected:
    void SetUp() override {
        sentinel::crypto::CryptoEngine::init();
        sender_kp = sentinel::crypto::CryptoEngine::generate_keypair();
        recipient_kp = sentinel::crypto::CryptoEngine::generate_keypair();
    }

    sentinel::crypto::KeyPair sender_kp;
    sentinel::crypto::KeyPair recipient_kp;
};

TEST_F(LedgerTest, MemoryStateBalanceAndTransfer) {
    sentinel::ledger::LedgerState state;
    state.set_balance(sender_kp.public_key, 1000);

    EXPECT_EQ(state.get_balance(sender_kp.public_key), 1000u);
    EXPECT_EQ(state.get_balance(recipient_kp.public_key), 0u);

    bool transfer_ok = state.apply_transfer(sender_kp.public_key, recipient_kp.public_key, 400, 10);
    EXPECT_TRUE(transfer_ok);

    EXPECT_EQ(state.get_balance(sender_kp.public_key), 590u);
    EXPECT_EQ(state.get_balance(recipient_kp.public_key), 400u);
}

TEST_F(LedgerTest, BridgeLockAndDoubleMintRejection) {
    sentinel::ledger::ChainState chain_a(1);   // Source Chain
    sentinel::ledger::ChainState chain_b(137); // Destination Chain

    chain_a.state().set_balance(sender_kp.public_key, 5000);

    // 1. Submit LOCK transaction on Chain A
    sentinel::ledger::Transaction lock_tx{
        .chain_id = 1,
        .type = sentinel::ledger::TxType::BRIDGE_LOCK,
        .nonce = 0,
        .sender = sender_kp.public_key,
        .recipient = recipient_kp.public_key,
        .amount = 2000,
        .fee = 50
    };

    EXPECT_TRUE(chain_a.submit_transaction(lock_tx));
    auto block_a = chain_a.produce_block(1000);
    ASSERT_TRUE(block_a.has_value());
    EXPECT_EQ(chain_a.state().get_balance(sender_kp.public_key), 2950u);

    auto events = chain_a.get_lock_events_since(0);
    ASSERT_EQ(events.size(), 1u);
    auto nullifier = events[0].lock_nullifier;

    // 2. Submit MINT transaction on Chain B using lock nullifier
    sentinel::ledger::Transaction mint_tx{
        .chain_id = 137,
        .type = sentinel::ledger::TxType::BRIDGE_MINT,
        .nonce = 0,
        .sender = recipient_kp.public_key,
        .recipient = recipient_kp.public_key,
        .amount = 2000,
        .fee = 0,
        .lock_event_nullifier = nullifier
    };

    EXPECT_TRUE(chain_b.submit_transaction(mint_tx));
    chain_b.produce_block(1010);
    EXPECT_EQ(chain_b.state().get_balance(recipient_kp.public_key), 2000u);

    // 3. Attempt Double-Spend (Replay same mint payload)
    sentinel::ledger::Transaction replay_mint_tx = mint_tx;
    replay_mint_tx.nonce = 1;

    EXPECT_TRUE(chain_b.submit_transaction(replay_mint_tx));
    chain_b.produce_block(1020);

    // Balance must remain unchanged due to nullifier deduplication
    EXPECT_EQ(chain_b.state().get_balance(recipient_kp.public_key), 2000u);
}
