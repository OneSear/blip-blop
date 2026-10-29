#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace bb::core {

// 64-bit FNV-1a. Used for frame and state fingerprints in regression
// replays: fast, endian-independent when fed bytes in a defined order, and
// trivially portable.
class Fnv1a64 {
public:
    static constexpr std::uint64_t kOffset = 0xcbf29ce484222325ull;
    static constexpr std::uint64_t kPrime = 0x100000001b3ull;

    void addBytes(const void* data, std::size_t size) noexcept {
        const auto* p = static_cast<const unsigned char*>(data);
        for (std::size_t i = 0; i < size; ++i) {
            value_ ^= p[i];
            value_ *= kPrime;
        }
    }

    // Integers are hashed as little-endian byte sequences so the result
    // does not depend on the host byte order.
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    void add(T v) noexcept {
        using U = std::make_unsigned_t<T>;
        auto u = static_cast<U>(v);
        for (std::size_t i = 0; i < sizeof(T); ++i) {
            value_ ^= static_cast<unsigned char>(u & 0xFFu);
            value_ *= kPrime;
            u = static_cast<U>(u >> 4 >> 4);
        }
    }

    std::uint64_t value() const noexcept { return value_; }
    void reset() noexcept { value_ = kOffset; }

private:
    std::uint64_t value_ = kOffset;
};

// Fast 64-bit hash over 32-bit words, for whole-frame fingerprints (one
// 640x480 frame is 307,200 words). Works on values, not bytes, so it does
// not depend on the host byte order. Not cryptographic: it only has to make
// accidental collisions between two frames negligible.
class WordHash64 {
public:
    void add(std::uint32_t a, std::uint32_t b) noexcept {
        std::uint64_t w = static_cast<std::uint64_t>(a) | (static_cast<std::uint64_t>(b) << 32);
        w *= 0x9E3779B97F4A7C15ull;
        w ^= w >> 29;
        state_ = (state_ ^ w) * 0xBF58476D1CE4E5B9ull;
        state_ = (state_ << 23) | (state_ >> 41);
        ++count_;
    }
    void add(std::uint32_t a) noexcept { add(a, 0x5A5A5A5Au); }

    std::uint64_t value() const noexcept {
        std::uint64_t h = state_ ^ (count_ * 0x94D049BB133111EBull);
        h ^= h >> 33;
        h *= 0xFF51AFD7ED558CCDull;
        h ^= h >> 33;
        h *= 0xC4CEB9FE1A85EC53ull;
        h ^= h >> 33;
        return h;
    }

private:
    std::uint64_t state_ = 0x243F6A8885A308D3ull;
    std::uint64_t count_ = 0;
};

}  // namespace bb::core
