#include "sentinel/core/random_engine.hpp"

namespace sentinel::core {

RandomEngine::RandomEngine(uint64_t seed)
    : m_seed(seed), m_generator(seed) {}

void RandomEngine::reseed(uint64_t seed) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_seed = seed;
    m_generator.seed(seed);
}

uint64_t RandomEngine::get_uint64(uint64_t min_val, uint64_t max_val) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::uniform_int_distribution<uint64_t> dist(min_val, max_val);
    return dist(m_generator);
}

double RandomEngine::get_double(double min_val, double max_val) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::uniform_real_distribution<double> dist(min_val, max_val);
    return dist(m_generator);
}

bool RandomEngine::sample_bernoulli(double probability) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::bernoulli_distribution dist(probability);
    return dist(m_generator);
}

uint64_t RandomEngine::sample_exponential_delay_ms(double mean_ms) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::exponential_distribution<double> dist(1.0 / mean_ms);
    double val = dist(m_generator);
    return static_cast<uint64_t>(val);
}

} // namespace sentinel::core
