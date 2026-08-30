#include "sentinel/relayer/relayer_engine.hpp"
#include "sentinel/core/logger.hpp"

namespace sentinel::relayer {

namespace {
std::string nullifier_to_string(const sentinel::crypto::Hash256& n) {
    return std::string(reinterpret_cast<const char*>(n.data()), n.size());
}
} // anonymous namespace

RelayerEngine::RelayerEngine(
    uint64_t source_chain_id,
    uint64_t dest_chain_id,
    size_t quorum_threshold,
    std::vector<std::shared_ptr<RelayerNode>> relayers)
    : m_source_chain_id(source_chain_id),
      m_dest_chain_id(dest_chain_id),
      m_quorum_threshold(quorum_threshold),
      m_relayers(std::move(relayers)) {

    std::vector<sentinel::crypto::PublicKey> pks;
    pks.reserve(m_relayers.size());
    for (const auto& r : m_relayers) {
        pks.push_back(r->public_key());
    }

    m_verifier = std::make_unique<sentinel::crypto::SignatureVerifier>(m_quorum_threshold, pks);
}

void RelayerEngine::add_relayer(std::shared_ptr<RelayerNode> relayer) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_relayers.push_back(relayer);

    std::vector<sentinel::crypto::PublicKey> pks;
    pks.reserve(m_relayers.size());
    for (const auto& r : m_relayers) {
        pks.push_back(r->public_key());
    }

    m_verifier = std::make_unique<sentinel::crypto::SignatureVerifier>(m_quorum_threshold, pks);
}

size_t RelayerEngine::poll_and_process_events(sentinel::ledger::ChainState& source_chain) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto lock_events = source_chain.get_lock_events_since(m_last_processed_block);
    size_t new_events_signed = 0;

    for (const auto& event : lock_events) {
        std::string null_str = nullifier_to_string(event.lock_nullifier);

        if (m_processed_nullifiers.count(null_str) > 0) continue;

        auto& proof = m_pending_proofs[null_str];
        proof.event = event;

        // Collect signatures from all available relayer nodes
        for (const auto& relayer : m_relayers) {
            auto sig = relayer->sign_lock_event(event);
            proof.signatures.push_back(sig);
        }

        new_events_signed++;
        LOG_INFO("RelayerEngine collected {} signatures for event nullifier on Source Chain {}",
                 proof.signatures.size(), m_source_chain_id);
    }

    m_last_processed_block = source_chain.block_height();
    return new_events_signed;
}

size_t RelayerEngine::submit_pending_mints(sentinel::ledger::ChainState& dest_chain) {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t submitted_count = 0;

    for (auto& [null_str, proof] : m_pending_proofs) {
        if (proof.submitted_to_chain_b) continue;

        std::span<const std::byte> nullifier_bytes(
            reinterpret_cast<const std::byte*>(proof.event.lock_nullifier.data()),
            proof.event.lock_nullifier.size()
        );

        // Verify that the gathered signatures meet the quorum requirement
        if (m_verifier->verify_quorum(nullifier_bytes, proof.signatures)) {
            sentinel::ledger::Transaction mint_tx{
                .chain_id = m_dest_chain_id,
                .type = sentinel::ledger::TxType::BRIDGE_MINT,
                .nonce = static_cast<uint64_t>(submitted_count),
                .sender = proof.event.recipient,
                .recipient = proof.event.recipient,
                .amount = proof.event.amount,
                .fee = 0,
                .lock_event_nullifier = proof.event.lock_nullifier
            };

            if (dest_chain.submit_transaction(mint_tx)) {
                proof.submitted_to_chain_b = true;
                m_processed_nullifiers.insert(null_str);
                submitted_count++;
                LOG_INFO("RelayerEngine successfully submitted MINT transaction to Destination Chain {}", m_dest_chain_id);
            }
        } else {
            LOG_WARN("RelayerEngine failed quorum check for event on Chain {}", m_source_chain_id);
        }
    }

    return submitted_count;
}

size_t RelayerEngine::pending_proof_count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pending_proofs.size();
}

size_t RelayerEngine::relayer_count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_relayers.size();
}

} // namespace sentinel::relayer
