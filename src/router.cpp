#include "oppo/router.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>

namespace oppo {

void FrameRouter::log_hex(const char* dir, const uint8_t* data, size_t len) {
    if (!trace_logging_) return;
    std::cout << "[TRACE] " << dir << " (" << std::dec << len << " B): ";
    for (size_t i = 0; i < len; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]) << " ";
    }
    std::cout << std::dec << "\n";
}

void FrameRouter::dispatch_frame(const OppoFrame& frame) {
    bool is_push = (frame.seq_id == 0 || frame.seq_id == 255) ||
                   (frame.group == 0x02 && frame.cmd_id == 0x04) ||
                   (frame.group == 0x04 && frame.cmd_id == 0x02) ||
                   (frame.group == 0x01 && frame.cmd_id == 0x02) ||
                   (frame.group == 0x03 && frame.cmd_id == 0x05) ||
                   (frame.group == 0x04 && frame.cmd_id == 0x05);

    if (is_push) {
        OppoMessage msg = MessageCodec::decode_frame(frame);
        for (const auto& cb : push_callbacks_) {
            if (cb) cb(frame, msg);
        }
    } else {
        pending_pushes_.push_back(frame);
    }
}

std::vector<OppoFrame> FrameRouter::poll_incoming(int timeout_ms) {
    std::vector<OppoFrame> frames;
    auto res = socket_.receive(1024, timeout_ms);
    if (res.has_value() && !res.value().empty()) {
        const auto& raw = res.value();
        log_hex("RX", raw.data(), raw.size());
        frames = parser_.feed(raw);
        for (const auto& f : frames) {
            dispatch_frame(f);
        }
    }
    return frames;
}

Result<OppoFrame> FrameRouter::send_and_receive(const OppoFrame& request, int timeout_ms) {
    std::vector<uint8_t> bytes = request.to_bytes();
    log_hex("TX", bytes.data(), bytes.size());

    auto send_res = socket_.send(bytes);
    if (!send_res.has_value()) {
        return send_res.error();
    }

    auto start_time = std::chrono::steady_clock::now();

    while (true) {
        // 1. Check pending queue first for matching sequence or command group
        for (auto it = pending_pushes_.begin(); it != pending_pushes_.end(); ++it) {
            bool matches = (it->seq_id == request.seq_id) ||
                           (it->group == request.group && (it->cmd_id == request.cmd_id || it->cmd_id == (request.cmd_id | 0x80)));
            if (matches) {
                OppoFrame matched = *it;
                pending_pushes_.erase(it);
                return matched;
            }
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();
        if (elapsed >= timeout_ms) {
            return FrameError::Timeout;
        }

        int rem_timeout = static_cast<int>(timeout_ms - elapsed);
        auto recv_res = socket_.receive(1024, std::min(rem_timeout, 200));

        // 🛡️ Abort immediately on fatal socket error rather than spinning
        if (!recv_res.has_value()) {
            if (recv_res.error() != FrameError::Timeout) {
                return recv_res.error();
            }
            continue;
        }

        if (!recv_res.value().empty()) {
            const auto& raw = recv_res.value();
            log_hex("RX", raw.data(), raw.size());
            auto parsed_frames = parser_.feed(raw);
            
            OppoFrame matched_frame;
            bool found_match = false;

            // 🛡️ Dispatch ALL frames so no telemetry is lost when multiple frames arrive in one buffer
            for (const auto& frame : parsed_frames) {
                bool matches = (frame.seq_id == request.seq_id) ||
                               (frame.group == request.group && (frame.cmd_id == request.cmd_id || frame.cmd_id == (request.cmd_id | 0x80)));
                if (matches && !found_match) {
                    matched_frame = frame;
                    found_match = true;
                } else {
                    dispatch_frame(frame);
                }
            }

            if (found_match) {
                return matched_frame;
            }
        }
    }
}

Result<OppoFrame> FrameRouter::send_and_receive(const OppoFrame& request,
                                                uint8_t expected_group,
                                                uint8_t expected_cmd,
                                                int timeout_ms) {
    (void)expected_group;
    (void)expected_cmd;
    return send_and_receive(request, timeout_ms);
}

} // namespace oppo
