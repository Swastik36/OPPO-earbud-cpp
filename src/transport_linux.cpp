#include "oppo/itransport.hpp"
#include "oppo/transport.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <algorithm>
#include <sys/time.h>

#ifndef AF_BLUETOOTH
#define AF_BLUETOOTH 31
#endif

#ifndef BTPROTO_RFCOMM
#define BTPROTO_RFCOMM 3
#endif

typedef struct {
    uint8_t b[6];
} __attribute__((packed)) oppo_bdaddr_t;

struct oppo_sockaddr_rc {
    sa_family_t   rc_family;
    oppo_bdaddr_t rc_bdaddr;
    uint8_t       rc_channel;
};

namespace oppo {

class LinuxBluetoothTransport : public ITransport {
private:
    UniqueFd socket_fd_;
    std::string mac_address_;
    uint8_t channel_{15};
    bool connected_{false};

    static bool parse_mac_address(const std::string& str, oppo_bdaddr_t& addr) {
        std::string norm = normalize_mac(str);
        unsigned int b[6];
        if (sscanf(norm.c_str(), "%02X:%02X:%02X:%02X:%02X:%02X",
                   &b[5], &b[4], &b[3], &b[2], &b[1], &b[0]) == 6) {
            for (int i = 0; i < 6; ++i) {
                addr.b[i] = static_cast<uint8_t>(b[i]);
            }
            return true;
        }
        return false;
    }

public:
    LinuxBluetoothTransport() = default;
    ~LinuxBluetoothTransport() override { disconnect(); }

    Result<bool> connect(const std::string& mac_address, uint8_t channel = 15, int timeout_sec = 5) override {
        disconnect();
        mac_address_ = normalize_mac(mac_address);
        channel_ = channel;

        int fd = ::socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
        if (fd < 0) {
            if (errno == EACCES || errno == EPERM) {
                std::cerr << "\n[!] PERMISSION DENIED: AF_BLUETOOTH socket creation failed (errno=" << errno << ").\n"
                          << "    Fix: Add your user to the 'bluetooth' group: sudo usermod -aG bluetooth $USER\n"
                          << "    Or grant raw net capabilities: sudo setcap cap_net_raw+ep <path_to_binary>\n\n";
                return FrameError::PermissionDenied;
            }
            return FrameError::SocketError;
        }

        socket_fd_.reset(fd);

        struct timeval tv;
        tv.tv_sec = timeout_sec;
        tv.tv_usec = 0;
        ::setsockopt(socket_fd_.get(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        ::setsockopt(socket_fd_.get(), SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        oppo_sockaddr_rc addr{};
        addr.rc_family = AF_BLUETOOTH;
        addr.rc_channel = channel;
        if (!parse_mac_address(mac_address_, addr.rc_bdaddr)) {
            std::cerr << "[!] Invalid Bluetooth MAC address format: " << mac_address << "\n";
            return FrameError::SocketError;
        }

        int res = ::connect(socket_fd_.get(), reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
        if (res < 0) {
            if (errno == EACCES || errno == EPERM) {
                std::cerr << "[!] Permission denied when connecting RFCOMM socket.\n";
                return FrameError::PermissionDenied;
            }
            std::cerr << "[!] Bluetooth RFCOMM connect failed to " << mac_address_ << " (errno=" << errno << ": " << strerror(errno) << ")\n";
            socket_fd_.reset();
            return FrameError::SocketError;
        }

        connected_ = true;
        return true;
    }

    void disconnect() override {
        if (socket_fd_.is_valid()) {
            socket_fd_.reset();
        }
        connected_ = false;
    }

    bool is_connected() const override { return connected_; }

    Result<size_t> send(const std::vector<uint8_t>& data) override {
        return send(data.data(), data.size());
    }

    Result<size_t> send(const uint8_t* data, size_t len) override {
        if (!connected_ || !socket_fd_.is_valid()) {
            return FrameError::SocketError;
        }

        ssize_t ret = ::send(socket_fd_.get(), data, len, 0);
        if (ret < 0) {
            if (errno == ECONNRESET || errno == EPIPE) {
                connected_ = false;
            }
            return FrameError::SocketError;
        }
        return static_cast<size_t>(ret);
    }

    Result<std::vector<uint8_t>> receive(size_t max_bytes = 1024, int timeout_ms = 2000) override {
        if (!connected_ || !socket_fd_.is_valid()) {
            return FrameError::SocketError;
        }

        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        ::setsockopt(socket_fd_.get(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        std::vector<uint8_t> buf(max_bytes);
        ssize_t ret = ::recv(socket_fd_.get(), buf.data(), max_bytes, 0);

        if (ret < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return FrameError::Timeout;
            }
            if (errno == ECONNRESET || errno == EPIPE) {
                connected_ = false;
            }
            return FrameError::SocketError;
        }
        if (ret == 0) {
            connected_ = false;
            return FrameError::SocketError;
        }

        buf.resize(static_cast<size_t>(ret));
        return buf;
    }
};

std::unique_ptr<ITransport> create_platform_transport() {
    return std::make_unique<LinuxBluetoothTransport>();
}

} // namespace oppo
