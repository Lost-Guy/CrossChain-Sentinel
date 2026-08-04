#include <gtest/gtest.h>
#include "sentinel/core/logger.hpp"
#include "sentinel/utils/config.hpp"

TEST(LoggerTest, InitializationAndLogging) {
    EXPECT_NO_THROW(sentinel::core::Logger::init("TEST-LOGGER", spdlog::level::info));
    EXPECT_NE(sentinel::core::Logger::get(), nullptr);
}

TEST(ConfigTest, DefaultConfigValues) {
    auto config = sentinel::utils::SimConfig::load_default();
    EXPECT_EQ(config.validator_count, 100u);
    EXPECT_EQ(config.seed, 42u);
}
