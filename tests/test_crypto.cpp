#include <gtest/gtest.h>
#include "sentinel/crypto/crypto_engine.hpp"
#include "sentinel/crypto/signature_verifier.hpp"
#include <string>

class CryptoTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(sentinel::crypto::CryptoEngine::init());
    }
};

TEST_F(CryptoTest, KeyPairAndSignatureVerification) {
    auto keypair = sentinel::crypto::CryptoEngine::generate_keypair();
    std::string message = "Cross-Chain Mint Proof Payload";
    std::span<const std::byte> msg_bytes(reinterpret_cast<const std::byte*>(message.data()), message.size());

    auto sig = sentinel::crypto::CryptoEngine::sign(msg_bytes, keypair.secret_key);
    bool valid = sentinel::crypto::CryptoEngine::verify(msg_bytes, sig, keypair.public_key);
    EXPECT_TRUE(valid);

    // Tampering test
    std::string tampered_message = "Cross-Chain Mint Proof Payload Tampered";
    std::span<const std::byte> tampered_bytes(reinterpret_cast<const std::byte*>(tampered_message.data()), tampered_message.size());
    EXPECT_FALSE(sentinel::crypto::CryptoEngine::verify(tampered_bytes, sig, keypair.public_key));
}

TEST_F(CryptoTest, HashingFunctions) {
    std::string data = "sentinel_blockchain_payload";
    std::span<const std::byte> bytes(reinterpret_cast<const std::byte*>(data.data()), data.size());

    auto sha256_hash = sentinel::crypto::CryptoEngine::sha256(bytes);
    auto keccak_hash = sentinel::crypto::CryptoEngine::keccak256(bytes);

    EXPECT_NE(sha256_hash, keccak_hash);
    EXPECT_EQ(sha256_hash.size(), 32u);
    EXPECT_EQ(keccak_hash.size(), 32u);
}

TEST_F(CryptoTest, QuorumMultiSigVerification) {
    std::vector<sentinel::crypto::KeyPair> validator_keys;
    std::vector<sentinel::crypto::PublicKey> authorized_pks;

    for (int i = 0; i < 5; ++i) {
        auto kp = sentinel::crypto::CryptoEngine::generate_keypair();
        validator_keys.push_back(kp);
        authorized_pks.push_back(kp.public_key);
    }

    // 3-of-5 Quorum rule
    sentinel::crypto::SignatureVerifier verifier(3, authorized_pks);

    std::string payload = "Lock Event Block 4001";
    std::span<const std::byte> msg_bytes(reinterpret_cast<const std::byte*>(payload.data()), payload.size());

    std::vector<sentinel::crypto::ValidatorSignature> signatures;
    for (int i = 0; i < 3; ++i) {
        signatures.push_back({
            .public_key = validator_keys[i].public_key,
            .signature = sentinel::crypto::CryptoEngine::sign(msg_bytes, validator_keys[i].secret_key)
        });
    }

    // Should pass with 3 valid signatures
    EXPECT_TRUE(verifier.verify_quorum(msg_bytes, signatures));

    // Duplicate signature replay attempt (should fail)
    std::vector<sentinel::crypto::ValidatorSignature> duplicate_sigs = {
        signatures[0], signatures[0], signatures[1]
    };
    EXPECT_FALSE(verifier.verify_quorum(msg_bytes, duplicate_sigs));
}
