#include "sentinel/crypto/signature_verifier.hpp"
#include "sentinel/core/logger.hpp"

namespace sentinel::crypto {

namespace {
std::string pk_to_string(const PublicKey& pk) {
    return std::string(reinterpret_cast<const char*>(pk.data()), pk.size());
}
} // anonymous namespace

SignatureVerifier::SignatureVerifier(size_t threshold_quorum, std::vector<PublicKey> authorized_validators)
    : m_threshold_quorum(threshold_quorum) {
    for (const auto& pk : authorized_validators) {
        m_authorized_set.insert(pk_to_string(pk));
    }
}

bool SignatureVerifier::verify_validator_signature(std::span<const std::byte> message, const ValidatorSignature& val_sig) const {
    std::string pk_str = pk_to_string(val_sig.public_key);
    if (m_authorized_set.find(pk_str) == m_authorized_set.end()) {
        LOG_WARN("Signature verification failed: Public key is not in authorized validator set.");
        return false;
    }

    return CryptoEngine::verify(message, val_sig.signature, val_sig.public_key);
}

bool SignatureVerifier::verify_quorum(std::span<const std::byte> message, const std::vector<ValidatorSignature>& signatures) const {
    std::unordered_set<std::string> seen_signers;
    size_t valid_count = 0;

    for (const auto& val_sig : signatures) {
        std::string pk_str = pk_to_string(val_sig.public_key);

        // Disallow duplicate signature replay from the same validator key
        if (seen_signers.count(pk_str) > 0) {
            LOG_WARN("Duplicate validator signature detected in quorum verification.");
            continue;
        }

        if (verify_validator_signature(message, val_sig)) {
            seen_signers.insert(pk_str);
            valid_count++;

            if (valid_count >= m_threshold_quorum) {
                LOG_DEBUG("Quorum successfully reached ({}/{} signatures).", valid_count, m_threshold_quorum);
                return true;
            }
        }
    }

    LOG_WARN("Quorum verification failed. Valid signatures: {}/{}.", valid_count, m_threshold_quorum);
    return false;
}

} // namespace sentinel::crypto
