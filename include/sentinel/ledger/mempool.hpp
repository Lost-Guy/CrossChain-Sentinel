#ifndef SENTINEL_LEDGER_MEMPOOL_HPP
#define SENTINEL_LEDGER_MEMPOOL_HPP

#include <cstdint>
#include <vector>
#include <queue>
#include <mutex>
#include <unordered_set>
#include <string>
#include "sentinel/ledger/transaction.hpp"

namespace sentinel::ledger {

struct TxCompare {
    bool operator()(const Transaction& a, const Transaction& b) const {
        if (a.fee != b.fee) {
            return a.fee < b.fee; // Max-heap based on transaction fee
        }
        return a.nonce > b.nonce;
    }
};

class Mempool {
public:
    explicit Mempool(size_t max_capacity = 10000);

    bool add_transaction(const Transaction& tx);
    std::vector<Transaction> pop_transactions(size_t max_count);
    
    [[nodiscard]] size_t size() const;
    void clear();

private:
    size_t m_max_capacity;
    std::priority_queue<Transaction, std::vector<Transaction>, TxCompare> m_queue;
    std::unordered_set<std::string> m_seen_tx_hashes;
    mutable std::mutex m_mutex;
};

} // namespace sentinel::ledger

#endif // SENTINEL_LEDGER_MEMPOOL_HPP
