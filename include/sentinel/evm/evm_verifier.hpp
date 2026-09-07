#ifndef SENTINEL_EVM_EVM_VERIFIER_HPP
#define SENTINEL_EVM_EVM_VERIFIER_HPP

#include <cstdint>
#include <array>
#include <vector>
#include <span>
#include <string>
#include "sentinel/crypto/crypto_engine.hpp"
#include "sentinel/ledger/transaction.hpp"

namespace sentinel::evm {

class EVMVerifier {
public:
    // Computes Solidity keccak256(abi.encodePacked(sourceChainId, destChainId, nonce, sender, recipient, amount))
    static sentinel::crypto::Hash256 compute_evm_nullifier(
        uint64_t source_chain_id,
        uint64_t dest_chain_id,
        uint64_t nonce,
        const sentinel::crypto::PublicKey& sender,
        const sentinel::crypto::PublicKey& recipient,
        uint64_t amount
    );

    // Cross-check whether EVM nullifier calculation matches C++ LockEvent computation
    static bool validate_state_transition_parity(
        const sentinel::ledger::Transaction& cpp_tx,
        const sentinel::crypto::Hash256& evm_expected_nullifier
    );
};

} // namespace sentinel::evm

#endif // SENTINEL_EVM_EVM_VERIFIER_HPP
