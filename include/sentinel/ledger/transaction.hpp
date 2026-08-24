#ifndef SENTINEL_LEDGER_TRANSACTION_HPP
#define SENTINEL_LEDGER_TRANSACTION_HPP

#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <cstddef>
#include "sentinel/crypto/crypto_engine.hpp"

namespace sentinel::ledger {

enum class TxType : uint8_t {
    TRANSFER    = 0x01,
    BRIDGE_LOCK = 0x02,
    BRIDGE_MINT = 0x03
};

struct Transaction {
    uint64_t chain_id;
    TxType type;
    uint64_t nonce;
    sentinel::crypto::PublicKey sender;
    sentinel::crypto::PublicKey recipient;
    uint64_t amount;
    uint64_t fee;
    
    // For BRIDGE_MINT transactions: nullifier generated on Chain A
    sentinel::crypto::Hash256 lock_event_nullifier{};
    sentinel::crypto::Signature signature{};

    [[nodiscard]] sentinel::crypto::Hash256 compute_tx_hash() const;
};

struct BlockHeader {
    uint64_t block_number;
    uint64_t chain_id;
    uint64_t timestamp_ms;
    sentinel::crypto::Hash256 prev_block_hash;
    sentinel::crypto::Hash256 merkle_root;
};

struct Block {
    BlockHeader header;
    std::vector<Transaction> transactions;

    [[nodiscard]] sentinel::crypto::Hash256 compute_block_hash() const;
};

} // namespace sentinel::ledger

#endif // SENTINEL_LEDGER_TRANSACTION_HPP
