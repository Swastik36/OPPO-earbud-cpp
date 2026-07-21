#include "oppo/device.hpp"

namespace oppo {

Result<bool> OppoDevice::connect(const std::string& mac_address, uint8_t channel) {
    mac_address_ = mac_address;
    state_ = ConnectionState::Connecting;

    auto res = socket_->connect(mac_address, channel);
    if (!res.has_value() || !res.value()) {
        state_ = ConnectionState::Disconnected;
        return res.error();
    }

    state_ = ConnectionState::Connected;
    return true;
}

void OppoDevice::disconnect() {
    socket_->disconnect();
    state_ = ConnectionState::Disconnected;
}

void OppoDevice::poll_incoming(int timeout_ms) {
    if (state_ == ConnectionState::Connected) {
        router_.poll_incoming(timeout_ms);
    }
}

Result<EarbudsBattery> OppoDevice::get_battery() {
    if (state_ != ConnectionState::Connected) return FrameError::SocketError;
    uint8_t seq = router_.next_seq();
    OppoFrame req = MessageCodec::encode_battery_read(seq);

    auto res = router_.send_and_receive(req);
    if (!res.has_value()) {
        return res.error();
    }

    return MessageCodec::parse_battery(res.value());
}

Result<EQPreset> OppoDevice::get_eq() {
    if (state_ != ConnectionState::Connected) return FrameError::SocketError;
    uint8_t seq = router_.next_seq();
    OppoFrame req = MessageCodec::encode_eq_read(seq);

    auto res = router_.send_and_receive(req);
    if (!res.has_value()) {
        return res.error();
    }

    return MessageCodec::parse_eq(res.value());
}

Result<bool> OppoDevice::set_eq(EQPreset preset) {
    if (state_ != ConnectionState::Connected) return FrameError::SocketError;
    uint8_t seq = router_.next_seq();
    OppoFrame req = MessageCodec::encode_eq_write(seq, preset);

    auto res = router_.send_and_receive(req, 1500);
    return res.has_value() || res.error() == FrameError::Timeout;
}

Result<bool> OppoDevice::get_game_mode() {
    if (state_ != ConnectionState::Connected) return FrameError::SocketError;
    uint8_t seq = router_.next_seq();
    OppoFrame req = MessageCodec::encode_game_mode_read(seq);

    auto res = router_.send_and_receive(req);
    if (!res.has_value()) {
        return res.error();
    }

    return MessageCodec::parse_game_mode(res.value()).enabled;
}

Result<bool> OppoDevice::set_game_mode(bool enable) {
    if (state_ != ConnectionState::Connected) return FrameError::SocketError;
    uint8_t seq = router_.next_seq();
    OppoFrame req = MessageCodec::encode_game_mode_write(seq, enable);

    auto res = router_.send_and_receive(req, 1500);
    return res.has_value() || res.error() == FrameError::Timeout;
}

Result<TouchGestureList> OppoDevice::get_gestures() {
    if (state_ != ConnectionState::Connected) return FrameError::SocketError;
    uint8_t seq = router_.next_seq();
    OppoFrame req = MessageCodec::encode_gestures_read(seq);

    auto res = router_.send_and_receive(req);
    if (!res.has_value()) {
        return res.error();
    }

    return MessageCodec::parse_gestures(res.value());
}

Result<FirmwareInfo> OppoDevice::get_firmware() {
    if (state_ != ConnectionState::Connected) return FrameError::SocketError;
    uint8_t seq = router_.next_seq();
    OppoFrame req = MessageCodec::encode_firmware_read(seq);

    auto res = router_.send_and_receive(req);
    if (!res.has_value()) {
        return res.error();
    }

    return MessageCodec::parse_firmware(res.value());
}

} // namespace oppo
