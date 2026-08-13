#include "sentinel/core/logger.hpp"
#include <spdlog/sinks/basic_file_sink.h>
#include <vector>

namespace sentinel::core {

std::shared_ptr<spdlog::logger> Logger::s_logger;

void Logger::init(const std::string& logger_name, spdlog::level::level_enum level) {
    if (s_logger) return; // Prevent re-initialization

    // Check if spdlog global registry already holds this logger
    auto existing_logger = spdlog::get(logger_name);
    if (existing_logger) {
        s_logger = existing_logger;
        return;
    }

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [thread %t] %v");

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("sentinel_simulation.log", true);
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [thread %t] %v");

    std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
    s_logger = std::make_shared<spdlog::logger>(logger_name, sinks.begin(), sinks.end());
    
    s_logger->set_level(level);
    s_logger->flush_on(spdlog::level::trace);
    
    spdlog::register_logger(s_logger);
}

std::shared_ptr<spdlog::logger>& Logger::get() {
    if (!s_logger) {
        init(); // Fallback default initialization
    }
    return s_logger;
}

} // namespace sentinel::core
