cmake_minimum_required(VERSION 3.16)
project(oppo-control-cpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PYSIDE6_DIR "/home/swastik/new-project/oppo gui/.venv/lib/python3.12/site-packages/PySide6")
set(PYSIDE6_INC "${PYSIDE6_DIR}/include")

# Add PySide6 CMake paths for Qt6 C++ headers and libraries
list(APPEND CMAKE_PREFIX_PATH "${PYSIDE6_DIR}/lib/cmake/PySide6")
list(APPEND CMAKE_PREFIX_PATH "${PYSIDE6_DIR}/lib/cmake")
list(APPEND CMAKE_PREFIX_PATH "${PYSIDE6_DIR}")

include_directories(
    include
    ${PYSIDE6_INC}
    ${PYSIDE6_INC}/QtCore
    ${PYSIDE6_INC}/QtGui
    ${PYSIDE6_INC}/QtQml
    ${PYSIDE6_INC}/QtQuick
    ${PYSIDE6_INC}/QtConcurrent
)

link_directories("${PYSIDE6_DIR}/Qt/lib")

set(CORE_SOURCES
    src/itransport.cpp
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

# Qt6 QML Desktop GUI Target
find_package(PySide6 COMPONENTS Core Gui Qml Quick Concurrent QUIET)
find_package(Qt6 COMPONENTS Core Gui Qml Quick Concurrent QUIET)

if(PySide6_FOUND OR Qt6_FOUND OR EXISTS "${PYSIDE6_INC}/QtCore/QObject")
    message(STATUS "[oppo-control] Qt6 headers found! Configuring oppoctl-gui desktop GUI target...")

    add_executable(oppoctl-gui
        src/main_gui.cpp
        src/EarbudController.cpp
        include/EarbudController.hpp
    )

    target_include_directories(oppoctl-gui PUBLIC
        ${PYSIDE6_INC}
        ${PYSIDE6_INC}/QtCore
        ${PYSIDE6_INC}/QtGui
        ${PYSIDE6_INC}/QtQml
        ${PYSIDE6_INC}/QtQuick
        ${PYSIDE6_INC}/QtConcurrent
    )

    if(TARGET PySide6::Gui)
        target_link_libraries(oppoctl-gui PRIVATE oppo_core PySide6::Core PySide6::Gui PySide6::Qml PySide6::Quick PySide6::Concurrent)
    elseif(TARGET Qt6::Core)
        target_link_libraries(oppoctl-gui PRIVATE oppo_core Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Concurrent)
    else()
        target_link_libraries(oppoctl-gui PRIVATE oppo_core Qt6Core Qt6Gui Qt6Qml Qt6Quick Qt6Concurrent)
    endif()
endif()
