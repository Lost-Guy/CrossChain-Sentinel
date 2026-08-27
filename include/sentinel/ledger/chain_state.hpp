#ifndef SENTINEL_LEDGER_CHAIN_STATE_HPP
#define SENTINEL_LEDGER_CHAIN_STATE_HPP

#include <cstdint>
#include <vector>
#include <mutex>
#include <optional>
#include "sentinel/ledger/ledger_state.hpp"
#include "sentinel/ledger/mempool.hpp"

namespace sentinel::ledger {

struct LockEvent {
    uint64_t source_chain_id;
    uint64_t nonce;
    sentinel::crypto::PublicKey sender;
    sentinel::crypto::PublicKey recipient;
    uint64_t amount;
    sentinel::crypto::Hash256 lock_nullifier;
};

class ChainState {
public:
    ChainState(uint64_t chain_id, size_t max_block_txs = 50);

    [[nodiscard]] uint64_t chain_id() const { return m_chain_id; }
    [[nodiscard]] uint64_t block_height() const;

    bool submit_transaction(const Transaction& tx);
    std::optional<Block> produce_block(uint64_t timestamp_ms);

    [[nodiscard]] LedgerState& state() { return m_state; }
    [[nodiscard]] const LedgerState& state() const { return m_state; }

    [[nodiscard]] std::vector<LockEvent> get_lock_events_since(uint64_t start_block) const;
    [[nodiscard]] std::vector<Block> get_blocks() const;

private:
    uint64_t m_chain_id;
    size_t m_max_block_txs;
    LedgerState m_state;
    Mempool m_mempool;
    std::vector<Block> m_chain;
    std::vector<LockEvent> m_lock_events;
    sentinel::crypto::Hash256 m_latest_block_hash{};
    mutable std::mutex m_mutex;
};

} // namespace sentinel::ledger

#endif // SENTINEL_LEDGER_CHAIN_STATE_HPP
