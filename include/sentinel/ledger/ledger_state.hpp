#ifndef SENTINEL_LEDGER_LEDGER_STATE_HPP
#define SENTINEL_LEDGER_LEDGER_STATE_HPP

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <mutex>
#include "sentinel/crypto/crypto_engine.hpp"

namespace sentinel::ledger {

class LedgerState {
public:
    LedgerState() = default;

    void set_balance(const sentinel::crypto::PublicKey& account, uint64_t balance);
    [[nodiscard]] uint64_t get_balance(const sentinel::crypto::PublicKey& account) const;
    [[nodiscard]] uint64_t get_nonce(const sentinel::crypto::PublicKey& account) const;

    bool apply_transfer(const sentinel::crypto::PublicKey& sender, const sentinel::crypto::PublicKey& recipient, uint64_t amount, uint64_t fee);
    bool apply_lock(const sentinel::crypto::PublicKey& sender, uint64_t amount, uint64_t fee);
    bool apply_mint(const sentinel::crypto::PublicKey& recipient, uint64_t amount, const sentinel::crypto::Hash256& nullifier);

    [[nodiscard]] bool is_nullifier_spent(const sentinel::crypto::Hash256& nullifier) const;

private:
    std::unordered_map<std::string, uint64_t> m_balances;
    std::unordered_map<std::string, uint64_t> m_nonces;
    std::unordered_set<std::string> m_spent_nullifiers;
    mutable std::mutex m_mutex;
};

} // namespace sentinel::ledger

#endif // SENTINEL_LEDGER_LEDGER_STATE_HPP
