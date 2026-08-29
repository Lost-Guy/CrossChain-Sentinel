#ifndef SENTINEL_RELAYER_RELAYER_NODE_HPP
#define SENTINEL_RELAYER_RELAYER_NODE_HPP

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <span>
#include "sentinel/crypto/crypto_engine.hpp"
#include "sentinel/crypto/signature_verifier.hpp"
#include "sentinel/ledger/chain_state.hpp"

namespace sentinel::relayer {

class RelayerNode {
public:
    RelayerNode(uint32_t node_id, sentinel::crypto::KeyPair keypair);

    [[nodiscard]] uint32_t node_id() const { return m_node_id; }
    [[nodiscard]] sentinel::crypto::PublicKey public_key() const { return m_keypair.public_key; }

    // Sign a lock event nullifier payload to attest to its validity on Chain A
    [[nodiscard]] sentinel::crypto::ValidatorSignature sign_lock_event(const sentinel::ledger::LockEvent& event) const;

private:
    uint32_t m_node_id;
    sentinel::crypto::KeyPair m_keypair;
};

} // namespace sentinel::relayer

#endif // SENTINEL_RELAYER_RELAYER_NODE_HPP
