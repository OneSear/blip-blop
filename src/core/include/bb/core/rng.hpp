#pragma once

#include <cstdint>

namespace bb::core {

// Deterministic pseudo-random generator.
//
// The 2002 game was built with Visual C++ and called rand() without ever
// seeding it, so it ran on the Microsoft CRT linear congruential generator
// with seed 1. The SDL port inherited whatever rand() the host libc provides
// (different sequences on glibc, macOS and Windows), which made runs
// non-reproducible across platforms. This class reproduces the Microsoft
// generator bit for bit on every platform: same constants, same 15-bit
// output, same default seed.
class Rng {
public:
    static constexpr std::uint32_t kDefaultSeed = 1;
    static constexpr int kMax = 0x7FFF;  // largest value next() returns

    explicit Rng(std::uint32_t seed = kDefaultSeed) noexcept : state_(seed) {}

    void seed(std::uint32_t seed) noexcept { state_ = seed; }
    std::uint32_t state() const noexcept { return state_; }

    // Returns a value in [0, kMax], like MSVC's rand().
    int next() noexcept {
        state_ = state_ * 214013u + 2531011u;
        return static_cast<int>((state_ >> 16) & 0x7FFFu);
    }

private:
    std::uint32_t state_;
};

}  // namespace bb::core
