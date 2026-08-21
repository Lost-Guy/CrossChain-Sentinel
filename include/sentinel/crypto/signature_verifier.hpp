#ifndef SENTINEL_CRYPTO_SIGNATURE_VERIFIER_HPP
#define SENTINEL_CRYPTO_SIGNATURE_VERIFIER_HPP

#include <cstdint>
#include <vector>
#include <unordered_set>
#include <span>
#include <string>
#include "sentinel/crypto/crypto_engine.hpp"

namespace sentinel::crypto {

struct ValidatorSignature {
    PublicKey public_key;
    Signature signature;
};

class SignatureVerifier {
public:
    SignatureVerifier(size_t threshold_quorum, std::vector<PublicKey> authorized_validators);

    // Verify if a single signature originates from an authorized validator
    [[nodiscard]] bool verify_validator_signature(std::span<const std::byte> message, const ValidatorSignature& val_sig) const;

    // Verify whether a collection of signatures reaches the required quorum (without duplicate signers)
    [[nodiscard]] bool verify_quorum(std::span<const std::byte> message, const std::vector<ValidatorSignature>& signatures) const;

    [[nodiscard]] size_t quorum_threshold() const { return m_threshold_quorum; }
    [[nodiscard]] size_t total_validators() const { return m_authorized_set.size(); }

private:
    size_t m_threshold_quorum;
    std::unordered_set<std::string> m_authorized_set; // Base hex or binary string lookup
};

} // namespace sentinel::crypto

#endif // SENTINEL_CRYPTO_SIGNATURE_VERIFIER_HPP
