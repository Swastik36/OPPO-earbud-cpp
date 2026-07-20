#include "oppo/protocol.hpp"
#include <sstream>
#include <iomanip>

namespace oppo {

std::vector<uint8_t> OppoFrame::to_bytes() const {
    std::vector<uint8_t> out;
    uint32_t payload_len = static_cast<uint32_t>(payload.size());
    // 5 bytes (session 2B + group 1B + cmd 1B + seq 1B) + 2 bytes (payload_len LE) + payload.size()
    uint32_t rem_len = 5 + 2 + payload_len;

    out.push_back(SYNC_BYTE);

    // Build 7-bit varint length header
    uint32_t val = rem_len;
    while (true) {
        uint8_t t = val & 0x7F;
        val >>= 7;
        if (val > 0) {
            out.push_back(t | 0x80);
        } else {
            out.push_back(t);
            break;
        }
    }

    // Link header padding (0x00, 0x00)
    out.push_back(0x00);
    out.push_back(0x00);

    // Group, Cmd, Seq
    out.push_back(group);
    out.push_back(cmd_id);
    out.push_back(seq_id);

    // Payload length in Little Endian (2 bytes)
    out.push_back(static_cast<uint8_t>(payload_len & 0xFF));
    out.push_back(static_cast<uint8_t>((payload_len >> 8) & 0xFF));

    // Append payload bytes
    out.insert(out.end(), payload.begin(), payload.end());

    return out;
}

Result<OppoFrame> OppoFrame::from_bytes(const std::vector<uint8_t>& data) {
    return from_bytes(data.data(), data.size());
}

Result<OppoFrame> OppoFrame::from_bytes(const uint8_t* data, size_t len) {
    if (len < 7) {
        return FrameError::DataTooShort;
    }
    if (data[0] != SYNC_BYTE) {
        return FrameError::InvalidSyncByte;
    }

    // Parse varint length header with maximum 5-byte LEB128 bound check
    size_t idx = 1;
    uint32_t rem_len = 0;
    uint32_t shift = 0;
    size_t varint_bytes = 0;

    while (true) {
        if (idx >= len) {
            return FrameError::MalformedVarint;
        }
        if (varint_bytes >= 5) { // Prevent infinite loop / overflow attack
            return FrameError::MalformedVarint;
        }
        uint8_t b = data[idx++];
        varint_bytes++;
        rem_len |= static_cast<uint32_t>(b & 0x7F) << shift;
        shift += 7;
        if (!(b & 0x80)) {
            break;
        }
    }

    if (len < idx + rem_len) {
        return FrameError::IncompleteFrame;
    }

    // Skip 2 bytes session header (0x00 0x00)
    size_t inner_offset = idx + 2;
    if (inner_offset + 5 > len) {
        return FrameError::IncompleteFrame;
    }

    uint8_t group = data[inner_offset];
    uint8_t cmd_id = data[inner_offset + 1];
    uint8_t seq_id = data[inner_offset + 2];

    // Explicit little-endian 16-bit payload length
    uint16_t payload_len = static_cast<uint16_t>(data[inner_offset + 3]) |
                          (static_cast<uint16_t>(data[inner_offset + 4]) << 8);

    if (inner_offset + 5 + payload_len > len) {
        return FrameError::IncompleteFrame;
    }

    std::vector<uint8_t> payload(data + inner_offset + 5, data + inner_offset + 5 + payload_len);
    return OppoFrame(group, cmd_id, seq_id, std::move(payload));
}

std::string OppoFrame::to_string() const {
    std::ostringstream ss;
    ss << "OppoFrame(group=0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(group)
       << ", cmd=0x" << std::setw(2) << static_cast<int>(cmd_id)
       << ", seq=0x" << std::setw(2) << static_cast<int>(seq_id)
       << ", payload_len=" << std::dec << payload.size() << " [";
    for (size_t i = 0; i < payload.size(); ++i) {
        if (i > 0) ss << " ";
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(payload[i]);
    }
    ss << "])";
    return ss.str();
}

std::vector<OppoFrame> OppoStreamParser::feed(const std::vector<uint8_t>& data) {
    return feed(data.data(), data.size());
}

std::vector<OppoFrame> OppoStreamParser::feed(const uint8_t* data, size_t len) {
    buffer_.insert(buffer_.end(), data, data + len);
    std::vector<OppoFrame> frames;

    while (true) {
        // Find next SYNC_BYTE
        size_t sync_pos = buffer_.size();
        for (size_t i = 0; i < buffer_.size(); ++i) {
            if (buffer_[i] == OppoFrame::SYNC_BYTE) {
                sync_pos = i;
                break;
            }
        }

        if (sync_pos == buffer_.size()) {
            buffer_.clear();
            break;
        }

        if (sync_pos > 0) {
            buffer_.erase(buffer_.begin(), buffer_.begin() + sync_pos);
        }

        if (buffer_.size() < 7) { // Sync(1) + Varint(min 1) + Session(2) + Group/Cmd/Seq(3)
            break;
        }

        // Try parsing varint length
        size_t p_idx = 1;
        uint32_t rem_len = 0;
        uint32_t shift = 0;
        size_t varint_count = 0;
        bool valid_varint = false;

        while (p_idx < buffer_.size()) {
            if (varint_count >= 5) {
                break;
            }
            uint8_t b = buffer_[p_idx++];
            varint_count++;
            rem_len |= static_cast<uint32_t>(b & 0x7F) << shift;
            shift += 7;
            if (!(b & 0x80)) {
                valid_varint = true;
                break;
            }
        }

        if (!valid_varint) {
            if (buffer_.size() >= 6) { // Bad varint header, discard corrupted sync byte
                buffer_.erase(buffer_.begin(), buffer_.begin() + 1);
                continue;
            }
            break; // Wait for more data
        }

        size_t total_len = p_idx + rem_len;
        if (rem_len > 512 || (p_idx + 1 < buffer_.size() && (buffer_[p_idx] != 0x00 || buffer_[p_idx + 1] != 0x00))) {
            // Implausible frame size or invalid session ID bytes (0x00 0x00 required)
            buffer_.erase(buffer_.begin(), buffer_.begin() + 1);
            continue;
        }
        if (buffer_.size() < total_len) {
            break;
        }

        std::vector<uint8_t> frame_bytes = std::vector<uint8_t>(buffer_.begin(), buffer_.begin() + total_len);
        auto res = OppoFrame::from_bytes(frame_bytes);
        if (res.has_value()) {
            frames.push_back(res.value());
            buffer_.erase(buffer_.begin(), buffer_.begin() + total_len);
        } else {
            buffer_.erase(buffer_.begin(), buffer_.begin() + 1);
        }
    }

    return frames;
}

} // namespace oppo
