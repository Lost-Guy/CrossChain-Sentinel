cmake_minimum_required(VERSION 3.20)

# Suppress Boost CMake policy warning on CMake 3.30+
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

project(crosschain-sentinel 
    VERSION 0.1.0 
    DESCRIPTION "P2P Cross-Chain Bridge Replay & Double-Spend Simulator"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable export of compile commands for LSPs (clangd)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Strict Compiler Flags
if(MSVC)
    add_compile_options(/W4 /WX /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow)
endif()

# Find Dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSODIUM REQUIRED IMPORTED_TARGET libsodium)

find_package(Boost REQUIRED COMPONENTS system thread)
find_package(OpenSSL REQUIRED)
find_package(spdlog REQUIRED)
find_package(GTest REQUIRED)
find_package(ZLIB REQUIRED)

# Core Library Target
add_library(sentinel_core
    src/core/logger.cpp
    src/utils/config.cpp
	src/core/random_engine.cpp
    src/core/event_scheduler.cpp
	src/core/protocol.cpp
	src/net/peer_session.cpp
    src/net/peer_manager.cpp
	src/crypto/crypto_engine.cpp
    src/crypto/signature_verifier.cpp
)

target_include_directories(sentinel_core PUBLIC include)
target_link_libraries(sentinel_core PUBLIC
    spdlog::spdlog
    Boost::system
    Boost::thread
    OpenSSL::Crypto
    PkgConfig::LIBSODIUM
	ZLIB::ZLIB
)

# Executable Target
add_executable(sentinel_bin src/main.cpp)
target_link_libraries(sentinel_bin PRIVATE sentinel_core)
set_target_properties(sentinel_bin PROPERTIES OUTPUT_NAME "crosschain-sentinel")

# Unit Tests Target
enable_testing()
add_executable(sentinel_tests 
    tests/test_logger.cpp
    tests/test_event_scheduler.cpp
	tests/test_protocol.cpp
	tests/test_p2p_network.cpp
	tests/test_crypto.cpp
)
target_link_libraries(sentinel_tests PRIVATE sentinel_core GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(sentinel_tests)
