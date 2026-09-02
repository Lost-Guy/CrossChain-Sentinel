#include "sentinel/adversary/fault_injector.hpp"
#include "sentinel/core/logger.hpp"

namespace sentinel::adversary {

FaultInjector::FaultInjector(std::shared_ptr<sentinel::core::RandomEngine> rng)
    : m_rng(std::move(rng)) {}

void FaultInjector::isolate_peer(uint32_t peer_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isolated_peers.insert(peer_id);
    LOG_WARN("Adversary Isolated Peer [{}] from network mesh.", peer_id);
}

void FaultInjector::create_partition(const std::vector<uint32_t>& partition_a, const std::vector<uint32_t>& partition_b) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_partition_group_a.clear();
    m_partition_group_b.clear();

    for (uint32_t id : partition_a) m_partition_group_a.insert(id);
    for (uint32_t id : partition_b) m_partition_group_b.insert(id);

    m_has_active_partition = true;
    LOG_WARN("Network Split-Brain Partition active: Group A ({}) vs Group B ({})", 
             partition_a.size(), partition_b.size());
}

void FaultInjector::heal_partitions() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isolated_peers.clear();
    m_partition_group_a.clear();
    m_partition_group_b.clear();
    m_has_active_partition = false;
    LOG_INFO("Network partitions healed. Mesh communication restored.");
}

void FaultInjector::set_packet_drop_rate(double drop_rate) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_packet_drop_rate = drop_rate;
}

void FaultInjector::set_mean_latency_ms(double mean_latency_ms) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mean_latency_ms = mean_latency_ms;
}

bool FaultInjector::is_partitioned(uint32_t sender_id, uint32_t receiver_id) const {
    if (m_isolated_peers.count(sender_id) > 0 || m_isolated_peers.count(receiver_id) > 0) {
        return true;
    }

    if (!m_has_active_partition) return false;

    bool sender_in_a = m_partition_group_a.count(sender_id) > 0;
    bool receiver_in_b = m_partition_group_b.count(receiver_id) > 0;
    bool sender_in_b = m_partition_group_b.count(sender_id) > 0;
    bool receiver_in_a = m_partition_group_a.count(receiver_id) > 0;

    return (sender_in_a && receiver_in_b) || (sender_in_b && receiver_in_a);
}

bool FaultInjector::should_drop_packet(uint32_t sender_id, uint32_t receiver_id) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (is_partitioned(sender_id, receiver_id)) {
        LOG_TRACE("Packet dropped due to Network Partition: Peer [{}] -> Peer [{}]", sender_id, receiver_id);
        return true;
    }

    if (m_packet_drop_rate > 0.0 && m_rng) {
        if (m_rng->sample_bernoulli(m_packet_drop_rate)) {
            LOG_TRACE("Packet dropped due to Fault Jitter: Peer [{}] -> Peer [{}]", sender_id, receiver_id);
            return true;
        }
    }

    return false;
}

uint64_t FaultInjector::calculate_latency_ms() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_mean_latency_ms <= 0.0 || !m_rng) return 0;
    return m_rng->sample_exponential_delay_ms(m_mean_latency_ms);
}

} // namespace sentinel::adversary
