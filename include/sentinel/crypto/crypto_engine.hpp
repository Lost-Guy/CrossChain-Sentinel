#ifndef SENTINEL_CRYPTO_CRYPTO_ENGINE_HPP
#define SENTINEL_CRYPTO_CRYPTO_ENGINE_HPP

#include <cstdint>
#include <array>
#include <vector>
#include <span>
#include <cstddef>
#include <optional>
#include <sodium.h>

namespace sentinel::crypto {

constexpr size_t PUBLIC_KEY_SIZE = crypto_sign_PUBLICKEYBYTES; // 32 bytes
constexpr size_t SECRET_KEY_SIZE = crypto_sign_SECRETKEYBYTES; // 64 bytes
constexpr size_t SIGNATURE_SIZE  = crypto_sign_BYTES;          // 64 bytes
constexpr size_t HASH_256_SIZE    = 32;                         // 32 bytes

using PublicKey  = std::array<uint8_t, PUBLIC_KEY_SIZE>;
using SecretKey  = std::array<uint8_t, SECRET_KEY_SIZE>;
using Signature  = std::array<uint8_t, SIGNATURE_SIZE>;
using Hash256    = std::array<uint8_t, HASH_256_SIZE>;

struct KeyPair {
    PublicKey public_key;
    SecretKey secret_key;
};

class CryptoEngine {
public:
    static bool init();

    static KeyPair generate_keypair();

    static Signature sign(std::span<const std::byte> message, const SecretKey& secret_key);
    static bool verify(std::span<const std::byte> message, const Signature& signature, const PublicKey& public_key);

    static Hash256 sha256(std::span<const std::byte> data);
    static Hash256 keccak256(std::span<const std::byte> data);
};

} // namespace sentinel::crypto

#endif // SENTINEL_CRYPTO_CRYPTO_ENGINE_HPP
