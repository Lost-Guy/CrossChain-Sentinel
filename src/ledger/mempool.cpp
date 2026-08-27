#include "sentinel/ledger/mempool.hpp"

namespace sentinel::ledger {

namespace {
std::string hash_to_string(const sentinel::crypto::Hash256& h) {
    return std::string(reinterpret_cast<const char*>(h.data()), h.size());
}
} // anonymous namespace

Mempool::Mempool(size_t max_capacity)
    : m_max_capacity(max_capacity) {}

bool Mempool::add_transaction(const Transaction& tx) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_queue.size() >= m_max_capacity) return false;

    auto hash_str = hash_to_string(tx.compute_tx_hash());
    if (m_seen_tx_hashes.count(hash_str) > 0) return false; // Reject duplicate submission

    m_seen_tx_hashes.insert(hash_str);
    m_queue.push(tx);
    return true;
}

std::vector<Transaction> Mempool::pop_transactions(size_t max_count) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Transaction> batch;
    batch.reserve(max_count);

    while (!m_queue.empty() && batch.size() < max_count) {
        batch.push_back(m_queue.top());
        auto hash_str = hash_to_string(m_queue.top().compute_tx_hash());
        m_seen_tx_hashes.erase(hash_str);
        m_queue.pop();
    }

    return batch;
}

size_t Mempool::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}

void Mempool::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::priority_queue<Transaction, std::vector<Transaction>, TxCompare> empty_q;
    std::swap(m_queue, empty_q);
    m_seen_tx_hashes.clear();
}

} // namespace sentinel::ledger
