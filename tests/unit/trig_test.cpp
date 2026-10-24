#include <doctest.h>

#include <cmath>

#include "bb/game/trig.hpp"

TEST_CASE("The trigonometry table is the one the 2002 game computed") {
    const double pi = 3.141592654;  // the 2002 PI
    for (int i = 0; i < 360; ++i) {
        CAPTURE(i);
        CHECK(bb::game::sine1024(i) == static_cast<int>(1024 * std::sin(i / (180 / pi))));
        CHECK(bb::game::cosine1024(i) == static_cast<int>(1024 * std::cos(i / (180 / pi))));
    }
}

TEST_CASE("The integer forms give what the 2002 floating-point code gave (D-56)") {
    // int(sqrt(n)): every n up to 2^22, then a spread up to INT_MAX.
    for (int n = 0; n <= (1 << 22); ++n) {
        if (bb::game::squareRoot(n) != static_cast<int>(std::sqrt(static_cast<double>(n)))) {
            FAIL("squareRoot(" << n << ")");
        }
    }
    for (long long n = 1 << 22; n <= 0x7FFFFFFF; n += 9973) {
        const int i = static_cast<int>(n);
        if (bb::game::squareRoot(i) != static_cast<int>(std::sqrt(static_cast<double>(i)))) {
            FAIL("squareRoot(" << i << ")");
        }
    }
    CHECK(bb::game::squareRoot(0x7FFFFFFF) == 46340);
    CHECK(bb::game::squareRoot(-5) == 0);

    // A plumber's jump: int(-9 + speed * 0.25).
    for (int speed = -1000; speed <= 1000; ++speed) {
        CHECK((speed - 36) / 4 == static_cast<int>(-9 + speed * 0.25));
    }

    // The cruisers' shots: int(2 - (dx * 2 * 0.25) / dy) and its opposite.
    constexpr int kSpeed = 2;
    for (int dy = -1000; dy <= 1000; ++dy) {
        if (dy == 0) {
            continue;
        }
        for (int dx = -1000; dx <= 1000; ++dx) {
            const int up = (4 * kSpeed * dy - dx * kSpeed) / (4 * dy);
            const int down = (-4 * kSpeed * dy + dx * kSpeed) / (4 * dy);
            if (up != static_cast<int>(kSpeed - (dx * kSpeed * 0.25) / dy) ||
                down != static_cast<int>(-kSpeed + (dx * kSpeed * 0.25) / dy)) {
                FAIL("dx " << dx << ", dy " << dy);
            }
        }
    }
}
