#ifndef OPPO_MESSAGES_HPP
#define OPPO_MESSAGES_HPP

#include <string>
#include <vector>
#include <variant>
#include <cstdint>
#include "protocol.hpp"

namespace oppo {

enum class EQPreset : uint8_t {
    ORIGINAL = 0,
    VOCALS = 1,
    BASS = 2
};

inline std::string eq_preset_to_string(EQPreset preset) {
    switch (preset) {
        case EQPreset::ORIGINAL: return "Original";
        case EQPreset::VOCALS: return "Clear Vocals";
        case EQPreset::BASS: return "Bass Boost";
    }
    return "Original";
}

struct BatteryVal {
    int percentage{0};
    bool is_charging{false};
};

struct EarbudsBattery {
    BatteryVal left;
    BatteryVal right;
    BatteryVal case_val;
    bool is_push_notification{false};

    std::string to_string() const;
};

struct GameModeState {
    bool enabled{false};
};

struct TouchGesture {
    uint8_t earbud_id{0};  // 1 = Left, 2 = Right
    uint8_t action_type{0};
    uint8_t trigger_id{0}; // 1=Single, 2=Double, 3=Triple, 4=Long Press
    uint8_t function_id{0};

    std::string to_string() const;
};

struct TouchGestureList {
    std::vector<TouchGesture> gestures;
};

struct FirmwareInfo {
    std::string version;
};

struct UnknownMessage {
    uint8_t group{0};
    uint8_t cmd_id{0};
    std::vector<uint8_t> payload;
};

// C++20 Value Semantics: Variant containing all message types
using OppoMessage = std::variant<
    EarbudsBattery,
    EQPreset,
    GameModeState,
    TouchGestureList,
    FirmwareInfo,
    UnknownMessage
>;

class MessageCodec {
public:
    // Decoders
    static EarbudsBattery parse_battery(const OppoFrame& frame);
    static EQPreset parse_eq(const OppoFrame& frame);
    static GameModeState parse_game_mode(const OppoFrame& frame);
    static TouchGestureList parse_gestures(const OppoFrame& frame);
    static FirmwareInfo parse_firmware(const OppoFrame& frame);

    // Encoders
    static OppoFrame encode_battery_read(uint8_t seq_id);
    static OppoFrame encode_eq_read(uint8_t seq_id);
    static OppoFrame encode_eq_write(uint8_t seq_id, EQPreset preset);
    static OppoFrame encode_game_mode_read(uint8_t seq_id);
    static OppoFrame encode_game_mode_write(uint8_t seq_id, bool enable);
    static OppoFrame encode_gestures_read(uint8_t seq_id);
    static OppoFrame encode_firmware_read(uint8_t seq_id);

    // Universal message parser
    static OppoMessage decode_frame(const OppoFrame& frame);
};

} // namespace oppo

#endif // OPPO_MESSAGES_HPP
