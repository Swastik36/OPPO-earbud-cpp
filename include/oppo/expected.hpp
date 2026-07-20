#ifndef OPPO_EXPECTED_HPP
#define OPPO_EXPECTED_HPP

#include <variant>
#include <utility>
#include <string>
#include <iostream>

namespace oppo {

enum class FrameError {
    DataTooShort,
    InvalidSyncByte,
    MalformedVarint,
    IncompleteFrame,
    ChecksumMismatch,
    SocketError,
    PermissionDenied,
    Timeout,
    OutofOrderSequence
};

inline std::string error_to_string(FrameError err) {
    switch (err) {
        case FrameError::DataTooShort: return "Data too short for a valid frame";
        case FrameError::InvalidSyncByte: return "Invalid sync byte (expected 0xAA)";
        case FrameError::MalformedVarint: return "Malformed varint length header (overflow or invalid MSB)";
        case FrameError::IncompleteFrame: return "Incomplete frame payload";
        case FrameError::ChecksumMismatch: return "Corrupted frame checksum";
        case FrameError::SocketError: return "RFCOMM socket I/O error";
        case FrameError::PermissionDenied: return "Permission denied (AF_BLUETOOTH requires root, 'bluetooth' group, or CAP_NET_RAW)";
        case FrameError::Timeout: return "Socket operation timed out";
        case FrameError::OutofOrderSequence: return "Sequence ID mismatch";
    }
    return "Unknown frame error";
}

template <typename T, typename E = FrameError>
class Result {
private:
    std::variant<T, E> data_;

public:
    Result(T val) : data_(std::move(val)) {}
    Result(E err) : data_(err) {}

    bool has_value() const { return std::holds_alternative<T>(data_); }
    explicit operator bool() const { return has_value(); }

    const T& value() const { return std::get<T>(data_); }
    T& value() { return std::get<T>(data_); }

    E error() const { return std::get<E>(data_); }

    T value_or(T default_val) const {
        return has_value() ? std::get<T>(data_) : default_val;
    }
};

} // namespace oppo

#endif // OPPO_EXPECTED_HPP
