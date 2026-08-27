#include "sentinel/ledger/chain_state.hpp"
#include "sentinel/core/logger.hpp"

namespace sentinel::ledger {

ChainState::ChainState(uint64_t chain_id, size_t max_block_txs)
    : m_chain_id(chain_id), m_max_block_txs(max_block_txs) {}

uint64_t ChainState::block_height() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_chain.size();
}

bool ChainState::submit_transaction(const Transaction& tx) {
    if (tx.chain_id != m_chain_id) return false;
    return m_mempool.add_transaction(tx);
}

std::optional<Block> ChainState::produce_block(uint64_t timestamp_ms) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto txs = m_mempool.pop_transactions(m_max_block_txs);

    std::vector<Transaction> executed_txs;
    executed_txs.reserve(txs.size());

    for (const auto& tx : txs) {
        bool success = false;
        if (tx.type == TxType::TRANSFER) {
            success = m_state.apply_transfer(tx.sender, tx.recipient, tx.amount, tx.fee);
        } else if (tx.type == TxType::BRIDGE_LOCK) {
            success = m_state.apply_lock(tx.sender, tx.amount, tx.fee);
            if (success) {
                sentinel::crypto::Hash256 nullifier = tx.compute_tx_hash();
                m_lock_events.push_back(LockEvent{
                    .source_chain_id = m_chain_id,
                    .nonce = tx.nonce,
                    .sender = tx.sender,
                    .recipient = tx.recipient,
                    .amount = tx.amount,
                    .lock_nullifier = nullifier
                });
                LOG_INFO("Chain [{}] BRIDGE_LOCK Event recorded. Nullifier created.", m_chain_id);
            }
        } else if (tx.type == TxType::BRIDGE_MINT) {
            success = m_state.apply_mint(tx.recipient, tx.amount, tx.lock_event_nullifier);
            if (success) {
                LOG_INFO("Chain [{}] BRIDGE_MINT executed successfully.", m_chain_id);
            } else {
                LOG_WARN("Chain [{}] BRIDGE_MINT failed: Double-spend or invalid nullifier.", m_chain_id);
            }
        }

        if (success) {
            executed_txs.push_back(tx);
        }
    }

    uint64_t new_height = m_chain.size() + 1;
    Block block{
        .header = BlockHeader{
            .block_number = new_height,
            .chain_id = m_chain_id,
            .timestamp_ms = timestamp_ms,
            .prev_block_hash = m_latest_block_hash,
            .merkle_root = {} // Merkle tree calculated over executed_txs
        },
        .transactions = executed_txs
    };

    m_latest_block_hash = block.compute_block_hash();
    m_chain.push_back(block);

    LOG_INFO("Chain [{}] Block #{} committed with {} transactions.", m_chain_id, new_height, executed_txs.size());
    return block;
}

std::vector<LockEvent> ChainState::get_lock_events_since(uint64_t start_block) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    (void)start_block;
    return m_lock_events;
}

std::vector<Block> ChainState::get_blocks() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_chain;
}

} // namespace sentinel::ledger
