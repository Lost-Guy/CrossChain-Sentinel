#include "sentinel/crypto/crypto_engine.hpp"
#include "sentinel/core/logger.hpp"
#include <openssl/evp.h>

namespace sentinel::crypto {

bool CryptoEngine::init() {
    if (sodium_init() < 0) {
        LOG_CRITICAL("Failed to initialize libsodium cryptographic library.");
        return false;
    }
    LOG_INFO("Libsodium cryptographic backend initialized successfully.");
    return true;
}

KeyPair CryptoEngine::generate_keypair() {
    KeyPair kp{};
    crypto_sign_keypair(kp.public_key.data(), kp.secret_key.data());
    return kp;
}

Signature CryptoEngine::sign(std::span<const std::byte> message, const SecretKey& secret_key) {
    Signature sig{};
    unsigned long long sig_len = 0;
    
    crypto_sign_detached(
        sig.data(),
        &sig_len,
        reinterpret_cast<const unsigned char*>(message.data()),
        message.size(),
        secret_key.data()
    );

    return sig;
}

bool CryptoEngine::verify(std::span<const std::byte> message, const Signature& signature, const PublicKey& public_key) {
    int res = crypto_sign_verify_detached(
        signature.data(),
        reinterpret_cast<const unsigned char*>(message.data()),
        message.size(),
        public_key.data()
    );

    return res == 0;
}

Hash256 CryptoEngine::sha256(std::span<const std::byte> data) {
    Hash256 out_hash{};
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (ctx) {
        EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
        EVP_DigestUpdate(ctx, data.data(), data.size());
        unsigned int len = 0;
        EVP_DigestFinal_ex(ctx, out_hash.data(), &len);
        EVP_MD_CTX_free(ctx);
    }
    return out_hash;
}

Hash256 CryptoEngine::keccak256(std::span<const std::byte> data) {
    Hash256 out_hash{};
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (ctx) {
        EVP_DigestInit_ex(ctx, EVP_sha3_256(), nullptr);
        EVP_DigestUpdate(ctx, data.data(), data.size());
        unsigned int len = 0;
        EVP_DigestFinal_ex(ctx, out_hash.data(), &len);
        EVP_MD_CTX_free(ctx);
    }
    return out_hash;
}

} // namespace sentinel::crypto
