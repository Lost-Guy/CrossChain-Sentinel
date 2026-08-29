#include "sentinel/relayer/relayer_node.hpp"
#include "sentinel/core/logger.hpp"

namespace sentinel::relayer {

RelayerNode::RelayerNode(uint32_t node_id, sentinel::crypto::KeyPair keypair)
    : m_node_id(node_id), m_keypair(std::move(keypair)) {}

sentinel::crypto::ValidatorSignature RelayerNode::sign_lock_event(const sentinel::ledger::LockEvent& event) const {
    std::span<const std::byte> nullifier_bytes(
        reinterpret_cast<const std::byte*>(event.lock_nullifier.data()),
        event.lock_nullifier.size()
    );

    auto sig = sentinel::crypto::CryptoEngine::sign(nullifier_bytes, m_keypair.secret_key);

    LOG_TRACE("RelayerNode [{}] signed lock event with nullifier.", m_node_id);

    return sentinel::crypto::ValidatorSignature{
        .public_key = m_keypair.public_key,
        .signature = sig
    };
}

} // namespace sentinel::relayer
