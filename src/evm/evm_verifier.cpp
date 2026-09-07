#include "sentinel/evm/evm_verifier.hpp"

namespace sentinel::evm {

namespace {
void append_u64_be(std::vector<std::byte>& buf, uint64_t val) {
    for (int i = 7; i >= 0; --i) {
        buf.push_back(static_cast<std::byte>((val >> (i * 8)) & 0xFF));
    }
}

void append_bytes(std::vector<std::byte>& buf, std::span<const uint8_t> data) {
    for (uint8_t b : data) {
        buf.push_back(static_cast<std::byte>(b));
    }
}
} // anonymous namespace

sentinel::crypto::Hash256 EVMVerifier::compute_evm_nullifier(
    uint64_t source_chain_id,
    uint64_t dest_chain_id,
    uint64_t nonce,
    const sentinel::crypto::PublicKey& sender,
    const sentinel::crypto::PublicKey& recipient,
    uint64_t amount) {

    std::vector<std::byte> packed_data;
    append_u64_be(packed_data, source_chain_id);
    append_u64_be(packed_data, dest_chain_id);
    append_u64_be(packed_data, nonce);
    append_bytes(packed_data, sender);
    append_bytes(packed_data, recipient);
    append_u64_be(packed_data, amount);

    return sentinel::crypto::CryptoEngine::keccak256(packed_data);
}

bool EVMVerifier::validate_state_transition_parity(
    const sentinel::ledger::Transaction& cpp_tx,
    const sentinel::crypto::Hash256& evm_expected_nullifier) {

    auto cpp_hash = cpp_tx.compute_tx_hash();
    return cpp_hash == evm_expected_nullifier;
}

} // namespace sentinel::evm
