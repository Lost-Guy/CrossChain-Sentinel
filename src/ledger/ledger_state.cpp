#include "sentinel/ledger/ledger_state.hpp"

namespace sentinel::ledger {

namespace {
std::string data_to_string(std::span<const uint8_t> d) {
    return std::string(reinterpret_cast<const char*>(d.data()), d.size());
}
} // anonymous namespace

void LedgerState::set_balance(const sentinel::crypto::PublicKey& account, uint64_t balance) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_balances[data_to_string(account)] = balance;
}

uint64_t LedgerState::get_balance(const sentinel::crypto::PublicKey& account) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_balances.find(data_to_string(account));
    return (it != m_balances.end()) ? it->second : 0;
}

uint64_t LedgerState::get_nonce(const sentinel::crypto::PublicKey& account) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_nonces.find(data_to_string(account));
    return (it != m_nonces.end()) ? it->second : 0;
}

bool LedgerState::apply_transfer(const sentinel::crypto::PublicKey& sender, const sentinel::crypto::PublicKey& recipient, uint64_t amount, uint64_t fee) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string sender_str = data_to_string(sender);
    std::string recipient_str = data_to_string(recipient);

    uint64_t total_required = amount + fee;
    if (m_balances[sender_str] < total_required) return false;

    m_balances[sender_str] -= total_required;
    m_balances[recipient_str] += amount;
    m_nonces[sender_str]++;
    return true;
}

bool LedgerState::apply_lock(const sentinel::crypto::PublicKey& sender, uint64_t amount, uint64_t fee) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string sender_str = data_to_string(sender);

    uint64_t total_required = amount + fee;
    if (m_balances[sender_str] < total_required) return false;

    m_balances[sender_str] -= total_required;
    m_nonces[sender_str]++;
    return true;
}

bool LedgerState::apply_mint(const sentinel::crypto::PublicKey& recipient, uint64_t amount, const sentinel::crypto::Hash256& nullifier) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string nullifier_str = data_to_string(nullifier);

    // Strict Double-Spend Check: Reject if nullifier is already recorded
    if (m_spent_nullifiers.count(nullifier_str) > 0) return false;

    std::string recipient_str = data_to_string(recipient);
    m_balances[recipient_str] += amount;
    m_spent_nullifiers.insert(nullifier_str);
    return true;
}

bool LedgerState::is_nullifier_spent(const sentinel::crypto::Hash256& nullifier) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_spent_nullifiers.count(data_to_string(nullifier)) > 0;
}

} // namespace sentinel::ledger
