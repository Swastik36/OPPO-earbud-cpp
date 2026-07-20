#ifndef OPPO_TRANSPORT_HPP
#define OPPO_TRANSPORT_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include "expected.hpp"

namespace oppo {

class UniqueFd {
private:
    int fd_{-1};

public:
    UniqueFd() = default;
    explicit UniqueFd(int fd) : fd_(fd) {}
    ~UniqueFd();

    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    UniqueFd(UniqueFd&& other) noexcept;
    UniqueFd& operator=(UniqueFd&& other) noexcept;

    int get() const { return fd_; }
    bool is_valid() const { return fd_ >= 0; }
    int release();
    void reset(int new_fd = -1);
};

class BluetoothSocket {
private:
    UniqueFd socket_fd_;
    std::string mac_address_;
    uint8_t channel_{15};
    bool connected_{false};

public:
    BluetoothSocket() = default;
    ~BluetoothSocket() = default;

    // Connect to target BDADDR on RFCOMM server channel (default 15 / DLCI 30)
    Result<bool> connect(const std::string& mac_address, uint8_t channel = 15, int timeout_sec = 5);
    void disconnect();

    bool is_connected() const { return connected_; }

    Result<size_t> send(const uint8_t* data, size_t len);
    Result<size_t> send(const std::vector<uint8_t>& data);

    Result<std::vector<uint8_t>> receive(size_t max_bytes = 1024, int timeout_ms = 2000);
};

} // namespace oppo

#endif // OPPO_TRANSPORT_HPP
