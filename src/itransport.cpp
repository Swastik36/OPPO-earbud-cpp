#include "oppo/itransport.hpp"
#include <cctype>
#include <algorithm>

namespace oppo {

std::string ITransport::normalize_mac(const std::string& raw_mac) {
    std::string clean;
    for (char c : raw_mac) {
        if (std::isxdigit(static_cast<unsigned char>(c))) {
            clean += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
    }
    if (clean.size() != 12) return raw_mac;

    std::string formatted;
    for (size_t i = 0; i < 12; i += 2) {
        if (i > 0) formatted += ":";
        formatted += clean.substr(i, 2);
    }
    return formatted;
}

} // namespace oppo
