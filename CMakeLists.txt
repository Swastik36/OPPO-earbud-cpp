cmake_minimum_required(VERSION 3.16)
project(oppo-control-cpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include)

# Core Library
add_library(oppo_core STATIC
    src/protocol.cpp
    src/messages.cpp
    src/transport.cpp
    src/router.cpp
    src/device.cpp
)

# CLI Driver Target
add_executable(oppoctl-cpp src/main_cli.cpp)
target_link_libraries(oppoctl-cpp PRIVATE oppo_core)

# C++ Unit Test Suite
add_executable(test_cpp_protocol tests/test_cpp_protocol.cpp)
target_link_libraries(test_cpp_protocol PRIVATE oppo_core)
