#pragma once

#include <cstdint>

namespace bb::core {

// Virtual time for deterministic runs: time advances only when a frame is
// completed, by exactly 1/fps second. Two runs that complete the same frames
// observe the same timestamps, whatever the speed of the host.
class FrameClock {
public:
    explicit FrameClock(std::uint32_t framesPerSecond = 60) noexcept : fps_(framesPerSecond) {}

    void advanceFrame() noexcept { ++frame_; }
    std::uint64_t frame() const noexcept { return frame_; }
    std::uint32_t framesPerSecond() const noexcept { return fps_; }

    // Exact rational conversion, rounded down: no drift over long runs.
    std::uint64_t microseconds() const noexcept { return frame_ * 1000000u / fps_; }
    std::uint64_t milliseconds() const noexcept { return frame_ * 1000u / fps_; }

private:
    std::uint64_t frame_ = 0;
    std::uint32_t fps_;
};

}  // namespace bb::core
