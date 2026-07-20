#include "oppo/transport.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <cstdio>
#include <iostream>
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

UniqueFd::~UniqueFd() {
    reset();
}

UniqueFd::UniqueFd(UniqueFd&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

UniqueFd& UniqueFd::operator=(UniqueFd&& other) noexcept {
    if (this != &other) {
        reset(other.fd_);
        other.fd_ = -1;
    }
    return *this;
}

int UniqueFd::release() {
    int old = fd_;
    fd_ = -1;
    return old;
}

void UniqueFd::reset(int new_fd) {
    if (fd_ >= 0) {
        ::close(fd_);
    }
    fd_ = new_fd;
}

static bool parse_mac_address(const std::string& str, oppo_bdaddr_t& addr) {
    unsigned int b[6];
    if (sscanf(str.c_str(), "%02X:%02X:%02X:%02X:%02X:%02X",
               &b[5], &b[4], &b[3], &b[2], &b[1], &b[0]) == 6 ||
        sscanf(str.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",
               &b[5], &b[4], &b[3], &b[2], &b[1], &b[0]) == 6) {
        for (int i = 0; i < 6; ++i) {
            addr.b[i] = static_cast<uint8_t>(b[i]);
        }
        return true;
    }
    return false;
}

Result<bool> BluetoothSocket::connect(const std::string& mac_address, uint8_t channel, int timeout_sec) {
    disconnect();

    mac_address_ = mac_address;
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

    // Set receive/send timeouts
    struct timeval tv;
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;
    ::setsockopt(socket_fd_.get(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    ::setsockopt(socket_fd_.get(), SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    oppo_sockaddr_rc addr{};
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = channel;
    if (!parse_mac_address(mac_address, addr.rc_bdaddr)) {
        std::cerr << "[!] Invalid Bluetooth MAC address format: " << mac_address << "\n";
        return FrameError::SocketError;
    }

    int res = ::connect(socket_fd_.get(), reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
    if (res < 0) {
        if (errno == EACCES || errno == EPERM) {
            std::cerr << "[!] Permission denied when connecting RFCOMM socket.\n";
            return FrameError::PermissionDenied;
        }
        std::cerr << "[!] Bluetooth RFCOMM connect failed to " << mac_address << " (errno=" << errno << ": " << strerror(errno) << ")\n";
        socket_fd_.reset();
        return FrameError::SocketError;
    }

    connected_ = true;
    return true;
}

void BluetoothSocket::disconnect() {
    if (socket_fd_.is_valid()) {
        socket_fd_.reset();
    }
    connected_ = false;
}

Result<size_t> BluetoothSocket::send(const std::vector<uint8_t>& data) {
    return send(data.data(), data.size());
}

Result<size_t> BluetoothSocket::send(const uint8_t* data, size_t len) {
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

Result<std::vector<uint8_t>> BluetoothSocket::receive(size_t max_bytes, int timeout_ms) {
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
        connected_ = false; // Closed by peer
        return FrameError::SocketError;
    }

    buf.resize(static_cast<size_t>(ret));
    return buf;
}

} // namespace oppo
