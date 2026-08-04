#include "sentinel/core/logger.hpp"
#include "sentinel/utils/config.hpp"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    sentinel::core::Logger::init("SENTINEL-MAIN", spdlog::level::debug);

    LOG_INFO("Initializing P2P Cross-Chain Bridge Replay & Double-Spend Simulator...");
    
    auto config = sentinel::utils::SimConfig::load_default();
    LOG_INFO("Loaded Configuration: Validators={}, ThreadPool={}, Seed={}", 
             config.validator_count, config.thread_pool_size, config.seed);

    LOG_DEBUG("System runtime ready for component integration.");
    return 0;
}
