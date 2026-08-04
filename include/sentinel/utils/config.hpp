#ifndef SENTINEL_UTILS_CONFIG_HPP
#define SENTINEL_UTILS_CONFIG_HPP

#include <cstdint>
#include <string>

namespace sentinel::utils {

struct SimConfig {
    uint32_t seed{42};
    uint32_t validator_count{100};
    uint32_t thread_pool_size{8};
    uint32_t network_latency_ms{50};
    double packet_drop_rate{0.05};
    std::string chain_a_rpc_endpoint{"http://127.0.0.1:8545"};
    std::string chain_b_rpc_endpoint{"http://127.0.0.1:8546"};

    static SimConfig load_default();
};

} // namespace sentinel::utils

#endif // SENTINEL_UTILS_CONFIG_HPP
