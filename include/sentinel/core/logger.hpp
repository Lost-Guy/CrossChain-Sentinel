#ifndef SENTINEL_CORE_LOGGER_HPP
#define SENTINEL_CORE_LOGGER_HPP

#include <memory>
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace sentinel::core {

class Logger {
public:
    static void init(const std::string& logger_name = "SENTINEL", spdlog::level::level_enum level = spdlog::level::info);
    static std::shared_ptr<spdlog::logger>& get();

private:
    static std::shared_ptr<spdlog::logger> s_logger;
};

} // namespace sentinel::core

// Logging Macros
#define LOG_TRACE(...)    ::sentinel::core::Logger::get()->trace(__VA_ARGS__)
#define LOG_DEBUG(...)    ::sentinel::core::Logger::get()->debug(__VA_ARGS__)
#define LOG_INFO(...)     ::sentinel::core::Logger::get()->info(__VA_ARGS__)
#define LOG_WARN(...)     ::sentinel::core::Logger::get()->warn(__VA_ARGS__)
#define LOG_ERROR(...)    ::sentinel::core::Logger::get()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::sentinel::core::Logger::get()->critical(__VA_ARGS__)

#endif // SENTINEL_CORE_LOGGER_HPP
