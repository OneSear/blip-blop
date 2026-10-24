#pragma once

#include <cstdint>

namespace bb::core {

// Fixed-step simulation clock (migration.md, sections 3.4 and 3.10): the
// time that passes between frames is accumulated and handed out as whole
// ticks of a fixed duration. The simulation therefore runs at the same
// speed whatever the display rate, and two runs that see the same frame
// times run the same ticks.
//
// The tick of Blip & Blop is 11.8 ms (about 84.7 Hz, kGameTick). At most
// maxTicksPerFrame ticks are handed out per frame; time beyond that is
// dropped, so that a slow frame (loading, a debugger) cannot cause a
// spiral of catch-up ticks.
class FixedClock {
public:
    static constexpr std::uint64_t kGameTick = 11800;  // microseconds

    explicit FixedClock(std::uint64_t tickMicroseconds = kGameTick, int maxTicksPerFrame = 3) noexcept
        : tick_(tickMicroseconds == 0 ? 1 : tickMicroseconds),
          maxTicks_(maxTicksPerFrame < 1 ? 1 : maxTicksPerFrame) {}

    // Starts counting at now with no time owed (start of a level).
    void reset(std::uint64_t now) noexcept {
        last_ = now;
        owed_ = 0;
    }

    // Starts counting at now again, keeping the time already owed: the
    // time spent in between (a pause menu, a dialogue) is not simulated.
    void rebase(std::uint64_t now) noexcept { last_ = now; }

    // For a new frame at time now: the number of ticks to run, which are
    // then considered done.
    int advance(std::uint64_t now) noexcept {
        if (now > last_) {
            owed_ += now - last_;
        }
        last_ = now;

        std::uint64_t ticks = owed_ / tick_;
        if (ticks > static_cast<std::uint64_t>(maxTicks_)) {
            ticks = static_cast<std::uint64_t>(maxTicks_);
            owed_ = 0;
        } else {
            owed_ -= ticks * tick_;
        }
        return static_cast<int>(ticks);
    }

    // Time owed that is not a whole tick yet.
    std::uint64_t owed() const noexcept { return owed_; }
    std::uint64_t tickMicroseconds() const noexcept { return tick_; }

private:
    std::uint64_t tick_;
    int maxTicks_;
    std::uint64_t last_ = 0;
    std::uint64_t owed_ = 0;
};

}  // namespace bb::core
