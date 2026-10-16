cmake_minimum_required(VERSION 3.20)
project(ghostnet_engine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warning flags
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

# Sanitizers option
option(ENABLE_SANITIZERS "Enable Address and Undefined Behavior Sanitizers" OFF)
if(ENABLE_SANITIZERS AND NOT MSVC)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

include(FetchContent)

# 1. nlohmann/json
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.zip
)
FetchContent_MakeAvailable(nlohmann_json)

# Engine Core Library
add_library(ghostnet_core
    src/event.cpp
    src/scheduler.cpp
    src/topology.cpp
    src/engine.cpp
    src/scenario_loader.cpp
)

target_include_directories(ghostnet_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(ghostnet_core PUBLIC nlohmann_json::nlohmann_json)

# Standalone CLI Executable
add_executable(ghostnet_cli
    src/main.cpp
)
target_link_libraries(ghostnet_cli PRIVATE ghostnet_core)

# 2. GoogleTest
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(engine_tests
    tests/test_scheduler.cpp
    tests/test_topology.cpp
    tests/test_request_rng.cpp
    tests/test_engine.cpp
    tests/test_scenario_loader.cpp
    tests/test_faults_and_queues.cpp
)
target_link_libraries(engine_tests PRIVATE
    ghostnet_core
    gtest_main
)

include(GoogleTest)
gtest_discover_tests(engine_tests)