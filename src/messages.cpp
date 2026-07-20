#include "oppo/messages.hpp"
#include <sstream>
#include <algorithm>

namespace oppo {

std::string EarbudsBattery::to_string() const {
    std::ostringstream ss;
    ss << "Left: " << left.percentage << "% [Charging=" << (left.is_charging ? "Yes" : "No") << "], "
       << "Right: " << right.percentage << "% [Charging=" << (right.is_charging ? "Yes" : "No") << "], "
       << "Case: " << case_val.percentage << "% [Charging=" << (case_val.is_charging ? "Yes" : "No") << "]";
    if (is_push_notification) {
        ss << " (Real-time Push Notification)";
    }
    return ss.str();
}

std::string TouchGesture::to_string() const {
    std::ostringstream ss;
    ss << (earbud_id == 1 ? "Left " : "Right ");
    switch (trigger_id) {
        case 1: ss << "Single Tap"; break;
        case 2: ss << "Double Tap"; break;
        case 3: ss << "Triple Tap"; break;
        case 4: ss << "Long Press"; break;
        case 5: ss << "Double & Hold"; break;
        case 6: ss << "Slide"; break;
        default: ss << "Trigger-" << static_cast<int>(trigger_id); break;
    }
    ss << " -> Func-0x" << std::hex << static_cast<int>(function_id);
    return ss.str();
}

EarbudsBattery MessageCodec::parse_battery(const OppoFrame& frame) {
    EarbudsBattery result;
    result.left = {0, false};
    result.right = {0, false};
    result.case_val = {-1, false};

    const auto& p = frame.payload;
    if (p.size() < 2) {
        return result;
    }

    // Check ASCII CSV format
    if (p.size() > 2 && (p[2] == 0x31 || p[2] == 0x32 || p[2] == 0x33)) {
        std::string csv_str(p.begin() + 2, p.end());
        std::stringstream ss(csv_str);
        std::string item;
        std::vector<std::string> parts;
        while (std::getline(ss, item, ',')) {
            parts.push_back(item);
        }
        for (size_t i = 0; i + 2 < parts.size(); i += 3) {
            std::string dev_id = parts[i];
            std::string chg_state = parts[i + 1];
            int raw_val = 0;
            try {
                raw_val = std::stoi(parts[i + 2]);
            } catch (...) {
                continue;
            }
            bool charging = (chg_state == "2");
            int pct = std::max(0, std::min(100, raw_val - 28));

            if (dev_id == "1") result.left = {pct, charging};
            else if (dev_id == "2") result.right = {pct, charging};
            else if (dev_id == "3") result.case_val = {pct, charging};
        }
        return result;
    }

    // Binary format parsing
    uint8_t count = p[1];
    for (uint8_t i = 0; i < count; ++i) {
        size_t offset = 2 + i * 2;
        if (offset + 1 >= p.size()) break;
        uint8_t b_id = p[offset];
        uint8_t info = p[offset + 1];

        int level = info & 0x7F;
        bool charging = (info & 0x80) != 0;

        BatteryVal val{level, (b_id == 3 ? charging : false)};
        if (b_id == 1) result.left = val;
        else if (b_id == 2) result.right = val;
        else if (b_id == 3) result.case_val = val;
    }

    if (frame.group == 0x02 && frame.cmd_id == 0x04) {
        result.is_push_notification = true;
    }

    return result;
}

EQPreset MessageCodec::parse_eq(const OppoFrame& frame) {
    const auto& p = frame.payload;
    if (p.size() >= 4 && p[0] == 0x02 && p[1] == 0x00 && p[2] == 0x00) {
        uint8_t v = p[3];
        if (v == 1) return EQPreset::VOCALS;
        if (v == 2) return EQPreset::BASS;
        return EQPreset::ORIGINAL;
    }
    if (p.size() >= 3) {
        uint8_t v = p[2];
        if (v == 1) return EQPreset::VOCALS;
        if (v == 2) return EQPreset::BASS;
    }
    return EQPreset::ORIGINAL;
}

GameModeState MessageCodec::parse_game_mode(const OppoFrame& frame) {
    const auto& p = frame.payload;
    for (size_t i = 0; i + 1 < p.size(); ++i) {
        if (p[i] == 0x28) {
            return GameModeState{p[i + 1] != 0};
        }
    }
    return GameModeState{false};
}

TouchGestureList MessageCodec::parse_gestures(const OppoFrame& frame) {
    TouchGestureList list;
    const auto& p = frame.payload;
    if (p.size() < 4) return list;

    uint8_t num_records = p[3];
    size_t offset = 4;
    if (p.size() < offset + num_records * 4) return list;

    for (uint8_t i = 0; i < num_records; ++i) {
        size_t idx = offset + i * 4;
        TouchGesture g;
        g.earbud_id = p[idx];
        g.action_type = p[idx + 1];
        g.trigger_id = p[idx + 2];
        g.function_id = p[idx + 3];
        list.gestures.push_back(g);
    }
    return list;
}

FirmwareInfo MessageCodec::parse_firmware(const OppoFrame& frame) {
    FirmwareInfo info;
    const auto& p = frame.payload;
    if (p.empty()) return info;

    if (p[0] == 0x06 && p.size() >= 8) {
        std::ostringstream ss;
        ss << static_cast<int>(p[3]) << "."
           << static_cast<int>(p[4]) << "."
           << static_cast<int>(p[5]) << "."
           << static_cast<int>(p[6]) << "."
           << static_cast<int>(p[7]);
        info.version = ss.str();
        return info;
    }

    if (p.size() > 2) {
        std::string ascii_str(p.begin() + 2, p.end());
        size_t null_pos = ascii_str.find('\0');
        if (null_pos != std::string::npos) {
            ascii_str = ascii_str.substr(0, null_pos);
        }
        info.version = ascii_str;
    }
    return info;
}

OppoFrame MessageCodec::encode_battery_read(uint8_t seq_id) {
    return OppoFrame(0x06, 0x01, seq_id, {});
}

OppoFrame MessageCodec::encode_eq_read(uint8_t seq_id) {
    return OppoFrame(0x0F, 0x01, seq_id, {});
}

OppoFrame MessageCodec::encode_eq_write(uint8_t seq_id, EQPreset preset) {
    return OppoFrame(0x06, 0x04, seq_id, {static_cast<uint8_t>(preset)});
}

OppoFrame MessageCodec::encode_game_mode_read(uint8_t seq_id) {
    return OppoFrame(0x0D, 0x01, seq_id, {0x06, 0x05, 0x11, 0x06, 0x1B, 0x27, 0x28});
}

OppoFrame MessageCodec::encode_game_mode_write(uint8_t seq_id, bool enable) {
    return OppoFrame(0x03, 0x04, seq_id, {0x28, static_cast<uint8_t>(enable ? 1 : 0)});
}

OppoFrame MessageCodec::encode_gestures_read(uint8_t seq_id) {
    return OppoFrame(0x08, 0x01, seq_id, {0x02, 0x03});
}

OppoFrame MessageCodec::encode_firmware_read(uint8_t seq_id) {
    return OppoFrame(0x07, 0x01, seq_id, {});
}

OppoMessage MessageCodec::decode_frame(const OppoFrame& frame) {
    if ((frame.group == 0x06 && frame.cmd_id == 0x01) ||
        (frame.group == 0x06 && frame.cmd_id == 0x81) ||
        (frame.group == 0x02 && frame.cmd_id == 0x04)) {
        return parse_battery(frame);
    }
    if (frame.group == 0x0F || (frame.group == 0x06 && frame.cmd_id == 0x04)) {
        return parse_eq(frame);
    }
    if (frame.group == 0x0D || (frame.group == 0x03 && frame.cmd_id == 0x04)) {
        return parse_game_mode(frame);
    }
    if (frame.group == 0x08 || frame.group == 0x01) {
        return parse_gestures(frame);
    }
    if (frame.group == 0x07) {
        return parse_firmware(frame);
    }
    return UnknownMessage{frame.group, frame.cmd_id, frame.payload};
}

} // namespace oppo
