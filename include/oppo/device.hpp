#ifndef OPPO_DEVICE_HPP
#define OPPO_DEVICE_HPP

#include <string>
#include <memory>
#include "itransport.hpp"
#include "router.hpp"
#include "messages.hpp"

namespace oppo {

enum class ConnectionState {
    Disconnected,
    Connecting,
    Syncing,
    Connected
};

inline std::string connection_state_to_string(ConnectionState state) {
    switch (state) {
        case ConnectionState::Disconnected: return "Disconnected";
        case ConnectionState::Connecting: return "Connecting";
        case ConnectionState::Syncing: return "Syncing";
        case ConnectionState::Connected: return "Connected";
    }
    return "Unknown";
}

class OppoDevice {
private:
    std::unique_ptr<ITransport> socket_{create_platform_transport()};
    FrameRouter router_{*socket_};
    ConnectionState state_{ConnectionState::Disconnected};
    std::string mac_address_;

public:
    OppoDevice() = default;
    explicit OppoDevice(std::unique_ptr<ITransport> transport)
        : socket_(std::move(transport)), router_{*socket_} {}
    ~OppoDevice() { disconnect(); }

    void set_trace(bool enable) { router_.set_trace(enable); }
    ConnectionState state() const { return state_; }
    std::string mac_address() const { return mac_address_; }

    Result<bool> connect(const std::string& mac_address = "60:55:56:22:49:A0", uint8_t channel = 15);
    void disconnect();

    // High-level API calls
    Result<EarbudsBattery> get_battery();
    Result<EQPreset> get_eq();
    Result<bool> set_eq(EQPreset preset);
    Result<bool> get_game_mode();
    Result<bool> set_game_mode(bool enable);
    Result<TouchGestureList> get_gestures();
    Result<FirmwareInfo> get_firmware();

    void register_push_callback(PushCallback callback) {
        router_.register_push_callback(std::move(callback));
    }
};

} // namespace oppo

#endif // OPPO_DEVICE_HPP
