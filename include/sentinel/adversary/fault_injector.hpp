#ifndef SENTINEL_ADVERSARY_FAULT_INJECTOR_HPP
#define SENTINEL_ADVERSARY_FAULT_INJECTOR_HPP

#include <cstdint>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <memory>
#include "sentinel/core/random_engine.hpp"

namespace sentinel::adversary {

class FaultInjector {
public:
    explicit FaultInjector(std::shared_ptr<sentinel::core::RandomEngine> rng);

    // Network Partition Management (Split-Brain Simulation)
    void isolate_peer(uint32_t peer_id);
    void create_partition(const std::vector<uint32_t>& partition_a, const std::vector<uint32_t>& partition_b);
    void heal_partitions();

    // Latency & Drop Configuration
    void set_packet_drop_rate(double drop_rate);
    void set_mean_latency_ms(double mean_latency_ms);

    // Filter decision for packet delivery
    [[nodiscard]] bool should_drop_packet(uint32_t sender_id, uint32_t receiver_id);
    [[nodiscard]] uint64_t calculate_latency_ms();

    [[nodiscard]] bool is_partitioned(uint32_t sender_id, uint32_t receiver_id) const;

private:
    std::shared_ptr<sentinel::core::RandomEngine> m_rng;
    double m_packet_drop_rate{0.0};
    double m_mean_latency_ms{0.0};

    // Partition map: peer_id -> partition_group_id
    std::unordered_set<uint32_t> m_isolated_peers;
    bool m_has_active_partition{false};
    std::unordered_set<uint32_t> m_partition_group_a;
    std::unordered_set<uint32_t> m_partition_group_b;

    mutable std::mutex m_mutex;
};

} // namespace sentinel::adversary

#endif // SENTINEL_ADVERSARY_FAULT_INJECTOR_HPP
