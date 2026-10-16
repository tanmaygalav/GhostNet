cmake_minimum_required(VERSION 3.20)
project(ghostnet_engine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Common compiler warning flags
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

# Sanitizers option for Debug builds
option(ENABLE_SANITIZERS "Enable Address and Undefined Behavior Sanitizers" OFF)
if(ENABLE_SANITIZERS AND NOT MSVC)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# Engine Core Library
add_library(ghostnet_core
    src/event.cpp
    src/scheduler.cpp
    src/topology.cpp
)

target_include_directories(ghostnet_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Standalone CLI Executable
add_executable(ghostnet_cli
    src/main.cpp
)
target_link_libraries(ghostnet_cli PRIVATE ghostnet_core)

# Testing using GoogleTest
include(FetchContent)
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
)
target_link_libraries(engine_tests PRIVATE
    ghostnet_core
    gtest_main
)

include(GoogleTest)
gtest_discover_tests(engine_tests)