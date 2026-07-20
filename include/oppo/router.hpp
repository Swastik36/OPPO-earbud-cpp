#ifndef OPPO_ROUTER_HPP
#define OPPO_ROUTER_HPP

#include <functional>
#include <vector>
#include <deque>
#include <cstdint>
#include "protocol.hpp"
#include "transport.hpp"
#include "messages.hpp"

namespace oppo {

using PushCallback = std::function<void(const OppoFrame&, const OppoMessage&)>;

class FrameRouter {
private:
    BluetoothSocket& socket_;
    OppoStreamParser parser_;
    uint8_t current_seq_{1};
    std::vector<PushCallback> push_callbacks_;
    std::deque<OppoFrame> pending_pushes_;
    bool trace_logging_{false};

public:
    explicit FrameRouter(BluetoothSocket& socket) : socket_(socket) {}

    void set_trace(bool enable) { trace_logging_ = enable; }
    
    uint8_t next_seq() {
        uint8_t seq = current_seq_++;
        if (current_seq_ == 0 || current_seq_ >= 255) {
            current_seq_ = 1;
        }
        if (seq == 0 || seq >= 255) {
            seq = 1;
            current_seq_ = 2;
        }
        return seq;
    }

    // Register callback for real-time unsolicited push notifications
    void register_push_callback(PushCallback callback) {
        push_callbacks_.push_back(std::move(callback));
    }

    // Sequence-Aware Send and Receive matched strictly by sequence number
    Result<OppoFrame> send_and_receive(const OppoFrame& request, int timeout_ms = 2500);

    // Optional legacy helper matching group/cmd or sequence
    Result<OppoFrame> send_and_receive(const OppoFrame& request,
                                       uint8_t expected_group,
                                       uint8_t expected_cmd,
                                       int timeout_ms = 2500);

    // Poll socket for any pending incoming frames
    std::vector<OppoFrame> poll_incoming(int timeout_ms = 100);

private:
    void dispatch_frame(const OppoFrame& frame);
    void log_hex(const char* dir, const uint8_t* data, size_t len);
};

} // namespace oppo

#endif // OPPO_ROUTER_HPP
