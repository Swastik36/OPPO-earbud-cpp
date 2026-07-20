#ifndef OPPO_PROTOCOL_HPP
#define OPPO_PROTOCOL_HPP

#include <vector>
#include <deque>
#include <cstdint>
#include <string>
#include "expected.hpp"

namespace oppo {

struct OppoFrame {
    static constexpr uint8_t SYNC_BYTE = 0xAA;

    uint8_t group{0};
    uint8_t cmd_id{0};
    uint8_t seq_id{0};
    std::vector<uint8_t> payload;

    OppoFrame() = default;
    OppoFrame(uint8_t g, uint8_t c, uint8_t s, std::vector<uint8_t> p)
        : group(g), cmd_id(c), seq_id(s), payload(std::move(p)) {}

    // Binary serialization
    std::vector<uint8_t> to_bytes() const;

    // Parsing single frame from raw bytes
    static Result<OppoFrame> from_bytes(const std::vector<uint8_t>& data);
    static Result<OppoFrame> from_bytes(const uint8_t* data, size_t len);

    // Diagnostic string helper
    std::string to_string() const;
};

class OppoStreamParser {
private:
    std::deque<uint8_t> buffer_;

public:
    OppoStreamParser() = default;

    // Feed chunk of data and extract all valid complete frames
    std::vector<OppoFrame> feed(const uint8_t* data, size_t len);
    std::vector<OppoFrame> feed(const std::vector<uint8_t>& data);

    void clear() { buffer_.clear(); }
    size_t buffered_bytes() const { return buffer_.size(); }
};

} // namespace oppo

#endif // OPPO_PROTOCOL_HPP
