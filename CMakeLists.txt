cmake_minimum_required(VERSION 3.16)
project(oppo-control-cpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include)

set(CORE_SOURCES
    src/protocol.cpp
    src/messages.cpp
    src/router.cpp
    src/device.cpp
)

if(UNIX AND NOT APPLE)
    # Linux Build (BlueZ / POSIX RFCOMM)
    message(STATUS "[oppo-control] Configuring for Linux (BlueZ / POSIX RFCOMM)")
    add_library(oppo_core STATIC ${CORE_SOURCES} src/transport_linux.cpp)

elseif(WIN32)
    # Windows Build (Winsock2 / WS2Bth)
    message(STATUS "[oppo-control] Configuring for Windows (Winsock2 Bluetooth)")
    add_library(oppo_core STATIC ${CORE_SOURCES} src/transport_win.cpp)
    target_link_libraries(oppo_core PUBLIC ws2_32 irprop)

elseif(APPLE)
    # macOS Build (IOBluetooth Objective-C++)
    message(STATUS "[oppo-control] Configuring for macOS (IOBluetooth Objective-C++)")
    enable_language(OBJCXX)
    add_library(oppo_core STATIC ${CORE_SOURCES} src/transport_mac.mm)
    find_library(IOBLUETOOTH_FRAMEWORK IOBluetooth)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    target_link_libraries(oppo_core PUBLIC ${IOBLUETOOTH_FRAMEWORK} ${FOUNDATION_FRAMEWORK})
endif()

# CLI Driver Target
add_executable(oppoctl-cpp src/main_cli.cpp)
target_link_libraries(oppoctl-cpp PRIVATE oppo_core)

# C++ Unit Test Suite
add_executable(test_cpp_protocol tests/test_cpp_protocol.cpp)
target_link_libraries(test_cpp_protocol PRIVATE oppo_core)
