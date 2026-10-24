#include <doctest.h>

#include <cstdint>

#include "bb/core/fixed_clock.hpp"
#include "bb/core/frame_clock.hpp"

using bb::core::FixedClock;

TEST_CASE("FixedClock runs 84.7 ticks per second of 60 Hz frames") {
    // The virtual clock of deterministic runs: frame n at n / 60 s.
    bb::core::FrameClock frames(60);
    FixedClock clock;
    clock.reset(frames.microseconds());

    int total = 0;
    int ones = 0;
    int twos = 0;
    for (int i = 0; i < 60 * 59; ++i) {  // 59 s: 3,540 frames
        frames.advanceFrame();
        const int ticks = clock.advance(frames.microseconds());
        REQUIRE(ticks >= 1);
        REQUIRE(ticks <= 2);
        (ticks == 1 ? ones : twos) += 1;
        total += ticks;
    }
    // 59 s / 11.8 ms = 5,000 ticks exactly.
    CHECK(total == 5000);
    CHECK(ones + twos == 3540);
    CHECK(clock.owed() == 0);
}

TEST_CASE("FixedClock gives one tick per frame on a display at the tick rate") {
    FixedClock clock;
    clock.reset(0);
    for (std::uint64_t t = 1; t <= 1000; ++t) {
        REQUIRE(clock.advance(t * FixedClock::kGameTick) == 1);
    }
}

TEST_CASE("FixedClock gives zero or one tick per frame on fast displays") {
    FixedClock clock;
    clock.reset(0);
    int total = 0;
    const std::uint64_t frame = 1000000 / 144;  // 144 Hz, rounded down
    for (std::uint64_t i = 1; i <= std::uint64_t{144} * 10; ++i) {
        const int ticks = clock.advance(i * frame);
        REQUIRE(ticks <= 1);
        total += ticks;
    }
    CHECK(total == static_cast<int>((std::uint64_t{144} * 10 * frame) / FixedClock::kGameTick));
}

TEST_CASE("FixedClock caps a long frame and drops the excess") {
    FixedClock clock(FixedClock::kGameTick, 3);
    clock.reset(1000);
    CHECK(clock.advance(1000 + 2000000) == 3);  // two seconds late
    CHECK(clock.owed() == 0);
    CHECK(clock.advance(1000 + 2000000 + FixedClock::kGameTick) == 1);
}

TEST_CASE("FixedClock::rebase forgets the time in between but keeps what is owed") {
    FixedClock clock;
    clock.reset(0);
    CHECK(clock.advance(FixedClock::kGameTick + 500) == 1);
    CHECK(clock.owed() == 500);
    clock.rebase(10000000);  // a long pause
    CHECK(clock.owed() == 500);
    CHECK(clock.advance(10000000 + FixedClock::kGameTick - 500) == 1);
    CHECK(clock.owed() == 0);
}

TEST_CASE("FixedClock ignores a clock that goes back") {
    FixedClock clock;
    clock.reset(50000);
    CHECK(clock.advance(40000) == 0);
    CHECK(clock.advance(40000 + FixedClock::kGameTick) == 1);
}
