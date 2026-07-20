#ifndef OPPO_ITRANSPORT_HPP
#define OPPO_ITRANSPORT_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include "expected.hpp"

namespace oppo {

class ITransport {
public:
    virtual ~ITransport() = default;

    virtual Result<bool> connect(const std::string& mac_address, uint8_t channel = 15, int timeout_sec = 5) = 0;
    virtual void disconnect() = 0;
    virtual bool is_connected() const = 0;

    virtual Result<size_t> send(const uint8_t* data, size_t len) = 0;
    virtual Result<size_t> send(const std::vector<uint8_t>& data) = 0;

    virtual Result<std::vector<uint8_t>> receive(size_t max_bytes = 1024, int timeout_ms = 2000) = 0;

    // Cross-platform MAC address normalization helper
    static std::string normalize_mac(const std::string& raw_mac);
};

// Cross-platform transport factory
std::unique_ptr<ITransport> create_platform_transport();

} // namespace oppo

#endif // OPPO_ITRANSPORT_HPP
