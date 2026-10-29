#pragma once

#include <cstdint>

namespace bb::platform {

// Monotonic wall-clock time.
class Clock {
public:
    virtual ~Clock() = default;
    virtual std::uint64_t microseconds() const = 0;  // since the clock was created
    virtual void sleep(std::uint64_t microseconds) = 0;
};

}  // namespace bb::platform
