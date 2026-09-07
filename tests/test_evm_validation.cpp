#include <gtest/gtest.h>
#include "sentinel/evm/evm_verifier.hpp"
#include "sentinel/crypto/crypto_engine.hpp"

TEST(EVMValidationTest, EVMNullifierParityCheck) {
    sentinel::crypto::CryptoEngine::init();
    auto sender = sentinel::crypto::CryptoEngine::generate_keypair();
    auto recipient = sentinel::crypto::CryptoEngine::generate_keypair();

    uint64_t source_chain = 1;
    uint64_t dest_chain = 137;
    uint64_t nonce = 42;
    uint64_t amount = 50000;

    auto evm_nullifier = sentinel::evm::EVMVerifier::compute_evm_nullifier(
        source_chain, dest_chain, nonce, sender.public_key, recipient.public_key, amount
    );

    sentinel::ledger::Transaction cpp_tx{
        .chain_id = source_chain,
        .type = sentinel::ledger::TxType::BRIDGE_LOCK,
        .nonce = nonce,
        .sender = sender.public_key,
        .recipient = recipient.public_key,
        .amount = amount,
        .fee = 0,
        .lock_event_nullifier = {},
        .signature = {}
    };

    EXPECT_EQ(evm_nullifier.size(), 32u);
    EXPECT_TRUE(sentinel::evm::EVMVerifier::validate_state_transition_parity(cpp_tx, cpp_tx.compute_tx_hash()));
}
