#include "oppo/itransport.hpp"
#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2bth.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "irprop.lib")

namespace oppo {

class WindowsBluetoothTransport : public ITransport {
private:
    SOCKET sock_{INVALID_SOCKET};
    bool connected_{false};
    bool wsa_initialized_{false};

public:
    WindowsBluetoothTransport() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0) {
            wsa_initialized_ = true;
        }
    }

    ~WindowsBluetoothTransport() override {
        disconnect();
        if (wsa_initialized_) {
            WSACleanup();
        }
    }

    Result<bool> connect(const std::string& mac_address, uint8_t channel = 15, int timeout_sec = 5) override {
        (void)timeout_sec;
        disconnect();

        if (!wsa_initialized_) {
            return FrameError::SocketError;
        }

        sock_ = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
        if (sock_ == INVALID_SOCKET) {
            return FrameError::SocketError;
        }

        std::string clean = normalize_mac(mac_address);
        BTH_ADDR bth_addr = 0;
        for (char c : clean) {
            if (c != ':') {
                uint8_t val = (c >= '0' && c <= '9') ? (c - '0') : (c - 'A' + 10);
                bth_addr = (bth_addr << 4) | val;
            }
        }

        SOCKADDR_BTH addr{};
        addr.addressFamily = AF_BTH;
        addr.btAddr = bth_addr;
        addr.port = channel;

        if (::connect(sock_, reinterpret_cast<SOCKADDR*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
            closesocket(sock_);
            sock_ = INVALID_SOCKET;
            return FrameError::SocketError;
        }

        connected_ = true;
        return true;
    }

    void disconnect() override {
        if (sock_ != INVALID_SOCKET) {
            closesocket(sock_);
            sock_ = INVALID_SOCKET;
        }
        connected_ = false;
    }

    bool is_connected() const override { return connected_; }

    Result<size_t> send(const std::vector<uint8_t>& data) override {
        return send(data.data(), data.size());
    }

    Result<size_t> send(const uint8_t* data, size_t len) override {
        if (!connected_ || sock_ == INVALID_SOCKET) return FrameError::SocketError;
        int ret = ::send(sock_, reinterpret_cast<const char*>(data), static_cast<int>(len), 0);
        if (ret == SOCKET_ERROR) {
            connected_ = false;
            return FrameError::SocketError;
        }
        return static_cast<size_t>(ret);
    }

    Result<std::vector<uint8_t>> receive(size_t max_bytes = 1024, int timeout_ms = 2000) override {
        (void)timeout_ms;
        if (!connected_ || sock_ == INVALID_SOCKET) return FrameError::SocketError;
        std::vector<uint8_t> buf(max_bytes);
        int ret = ::recv(sock_, reinterpret_cast<char*>(buf.data()), static_cast<int>(max_bytes), 0);
        if (ret <= 0) {
            connected_ = false;
            return FrameError::SocketError;
        }
        buf.resize(static_cast<size_t>(ret));
        return buf;
    }
};

std::unique_ptr<ITransport> create_platform_transport() {
    return std::make_unique<WindowsBluetoothTransport>();
}

} // namespace oppo
#endif
