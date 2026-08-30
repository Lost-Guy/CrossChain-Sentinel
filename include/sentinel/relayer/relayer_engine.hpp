#ifndef SENTINEL_RELAYER_RELAYER_ENGINE_HPP
#define SENTINEL_RELAYER_RELAYER_ENGINE_HPP

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <memory>
#include <string>
#include "sentinel/relayer/relayer_node.hpp"
#include "sentinel/crypto/signature_verifier.hpp"
#include "sentinel/ledger/chain_state.hpp"

namespace sentinel::relayer {

struct PendingProof {
    sentinel::ledger::LockEvent event;
    std::vector<sentinel::crypto::ValidatorSignature> signatures;
    bool submitted_to_chain_b{false};
};

class RelayerEngine {
public:
    RelayerEngine(
        uint64_t source_chain_id,
        uint64_t dest_chain_id,
        size_t quorum_threshold,
        std::vector<std::shared_ptr<RelayerNode>> relayers
    );

    // Register a new relayer node into the network pool
    void add_relayer(std::shared_ptr<RelayerNode> relayer);

    // Poll source chain for unprocessed lock events and collect relayer signatures
    size_t poll_and_process_events(sentinel::ledger::ChainState& source_chain);

    // Submit all quorum-verified mint transactions to destination chain mempool
    size_t submit_pending_mints(sentinel::ledger::ChainState& dest_chain);

    [[nodiscard]] size_t pending_proof_count() const;
    [[nodiscard]] size_t relayer_count() const;

private:
    uint64_t m_source_chain_id;
    uint64_t m_dest_chain_id;
    size_t m_quorum_threshold;

    std::vector<std::shared_ptr<RelayerNode>> m_relayers;
    std::unique_ptr<sentinel::crypto::SignatureVerifier> m_verifier;

    uint64_t m_last_processed_block{0};
    std::unordered_map<std::string, PendingProof> m_pending_proofs;
    std::unordered_set<std::string> m_processed_nullifiers;
    mutable std::mutex m_mutex;
};

} // namespace sentinel::relayer

#endif // SENTINEL_RELAYER_RELAYER_ENGINE_HPP
