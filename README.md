# OPPO Enco Buds3 Pro Linux Companion (C++ Native)

A high-performance, compiled C++20 native companion library, CLI driver (`oppoctl-cpp`), and protocol engine for **OPPO Enco Buds3 Pro** (and OPO v1 RFCOMM protocol Bluetooth earbuds).

---

## 1. Features & Architecture

* **Zero-Exception Result Types**: Uses lightweight `Result<T, FrameError>` without C++ exception unwinding overhead.
* **Sequence-Aware Frame Router**: Sequence-number correlated frame dispatcher (`FrameRouter`) that handles unsolicited real-time push notifications (e.g. earbud battery charging pushes `0x0204`) without dropping or misattributing request/response frames.
* **RAII POSIX Transport**: `BluetoothSocket` wrapper over Linux `AF_BLUETOOTH` / `BTPROTO_RFCOMM` with `UniqueFd` cleanup and automatic permission error hints (`EACCES`/`EPERM`).
* **Noise-Resilient Stream Parser**: LEB128 5-byte varint bounds checking and Session ID (`0x0000`) validation for instant resynchronization over noisy Bluetooth streams.
* **Value-Based Message Variant**: `using OppoMessage = std::variant<EarbudsBattery, EQPreset, GameModeState, TouchGestureList, FirmwareInfo, UnknownMessage>` utilizing C++20 value semantics.
* **Ground-Truth Python Oracle Tests**: Includes Python fixture generator (`generate_fixtures.py`) that exports `.bin` binary captures to verify 100% agreement between C++ and Python.

---

## 2. Directory Structure

```text
oppo-control-cpp/
├── Makefile                    # Standard C++20 Makefile
├── CMakeLists.txt              # CMake build configuration
├── generate_fixtures.py        # Python binary oracle generator
├── include/
│   └── oppo/
│       ├── expected.hpp        # Result<T, FrameError> exception-free error type
│       ├── protocol.hpp        # OppoFrame & OppoStreamParser
│       ├── messages.hpp        # Message structs & OppoMessage std::variant
│       ├── transport.hpp       # RAII UniqueFd & BluetoothSocket
│       ├── router.hpp          # Sequence-aware FrameRouter & push dispatcher
│       └── device.hpp          # High-level OppoDevice C++ API
├── src/
│   ├── protocol.cpp
│   ├── messages.cpp
│   ├── transport.cpp
│   ├── router.cpp
│   ├── device.cpp
│   └── main_cli.cpp            # Compiled `oppoctl-cpp` driver
├── fixtures/                   # Binary oracle test files (.bin)
└── tests/
    └── test_cpp_protocol.cpp   # Native C++ unit tests
```

---

## 3. Building & Testing

### Using `make`:
```bash
# Build the CLI utility and test suite
make

# Run the native test suite against ground-truth Python binary fixtures
make test
```

### Using `cmake`:
```bash
mkdir build && cd build
cmake ..
make
```

---

## 4. Running the C++ CLI Driver (`oppoctl-cpp`)

```bash
# Query real-time battery status
./oppoctl-cpp --mac 60:55:56:22:49:A0 battery

# Enable raw TX/RX hex logging
./oppoctl-cpp --mac 60:55:56:22:49:A0 --trace battery

# Update EQ preset (original, clear_vocals, bass_boost)
./oppoctl-cpp eq bass_boost

# Toggle Low-Latency Game Mode
./oppoctl-cpp gamemode on

# Dump touch gestures configuration
./oppoctl-cpp gestures
```

---

## 5. Linux Permissions Note (`EACCES` / `EPERM`)

Standard Linux user accounts running raw RFCOMM socket operations over `AF_BLUETOOTH` may require appropriate system permissions. If you encounter permission errors:

```bash
# Add your user to the bluetooth group:
sudo usermod -aG bluetooth $USER

# Or set cap_net_raw capabilities on the compiled binary:
sudo setcap cap_net_raw+ep ./oppoctl-cpp
```
