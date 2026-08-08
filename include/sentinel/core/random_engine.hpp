#ifndef SENTINEL_CORE_RANDOM_ENGINE_HPP
#define SENTINEL_CORE_RANDOM_ENGINE_HPP

#include <cstdint>
#include <random>
#include <mutex>

namespace sentinel::core {

class RandomEngine {
public:
    explicit RandomEngine(uint64_t seed = 42);

    void reseed(uint64_t seed);
    
    // Uniform integer in range [min_val, max_val]
    uint64_t get_uint64(uint64_t min_val, uint64_t max_val);
    
    // Uniform double in range [min_val, max_val]
    double get_double(double min_val, double max_val);
    
    // Bernoulli trial with given success probability [0.0, 1.0]
    bool sample_bernoulli(double probability);
    
    // Exponentially distributed network delay (ms)
    uint64_t sample_exponential_delay_ms(double mean_ms);

private:
    uint64_t m_seed;
    std::mt19937_64 m_generator;
    std::mutex m_mutex;
};

} // namespace sentinel::core

#endif // SENTINEL_CORE_RANDOM_ENGINE_HPP
