#ifndef OPPO_TRANSPORT_HPP
#define OPPO_TRANSPORT_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <unistd.h>
#include "expected.hpp"
#include "itransport.hpp"

namespace oppo {

class UniqueFd {
private:
    int fd_{-1};

public:
    UniqueFd() = default;
    explicit UniqueFd(int fd) : fd_(fd) {}
    ~UniqueFd() { reset(); }

    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    UniqueFd(UniqueFd&& other) noexcept : fd_(other.fd_) {
        other.fd_ = -1;
    }

    UniqueFd& operator=(UniqueFd&& other) noexcept {
        if (this != &other) {
            reset(other.fd_);
            other.fd_ = -1;
        }
        return *this;
    }

    int get() const { return fd_; }
    bool is_valid() const { return fd_ >= 0; }
    
    int release() {
        int old = fd_;
        fd_ = -1;
        return old;
    }

    void reset(int new_fd = -1) {
        if (fd_ >= 0) {
            ::close(fd_);
        }
        fd_ = new_fd;
    }
};

} // namespace oppo

#endif // OPPO_TRANSPORT_HPP
