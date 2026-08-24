#include "sentinel/ledger/transaction.hpp"
#include <vector>
#include <cstddef>

namespace sentinel::ledger {

namespace {
void append_u64(std::vector<std::byte>& buf, uint64_t val) {
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

sentinel::crypto::Hash256 Transaction::compute_tx_hash() const {
    std::vector<std::byte> buf;
    append_u64(buf, chain_id);
    buf.push_back(static_cast<std::byte>(type));
    append_u64(buf, nonce);
    append_bytes(buf, sender);
    append_bytes(buf, recipient);
    append_u64(buf, amount);
    append_u64(buf, fee);
    append_bytes(buf, lock_event_nullifier);

    return sentinel::crypto::CryptoEngine::keccak256(buf);
}

sentinel::crypto::Hash256 Block::compute_block_hash() const {
    std::vector<std::byte> buf;
    append_u64(buf, header.block_number);
    append_u64(buf, header.chain_id);
    append_u64(buf, header.timestamp_ms);
    append_bytes(buf, header.prev_block_hash);
    append_bytes(buf, header.merkle_root);

    return sentinel::crypto::CryptoEngine::keccak256(buf);
}

} // namespace sentinel::ledger
