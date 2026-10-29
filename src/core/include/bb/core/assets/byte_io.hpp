#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

namespace bb::core::assets {

// Bounds-checked little-endian reader over a byte buffer. A read past the
// end returns zeros and puts the reader in the failed state, so a loader
// can read a whole record and check ok() once.
class ByteReader {
public:
    ByteReader(const std::uint8_t* data, std::size_t size) noexcept : data_(data), size_(size) {}
    explicit ByteReader(const std::vector<std::uint8_t>& bytes) noexcept
        : ByteReader(bytes.data(), bytes.size()) {}

    bool ok() const noexcept { return ok_; }
    std::size_t position() const noexcept { return pos_; }
    std::size_t size() const noexcept { return size_; }
    std::size_t remaining() const noexcept { return size_ - pos_; }
    const std::uint8_t* current() const noexcept { return data_ + pos_; }

    bool canRead(std::size_t n) const noexcept { return ok_ && n <= size_ - pos_; }

    std::uint8_t u8() noexcept {
        if (!take(1)) {
            return 0;
        }
        return data_[pos_ - 1];
    }
    std::uint16_t u16() noexcept {
        if (!take(2)) {
            return 0;
        }
        const std::uint8_t* p = data_ + pos_ - 2;
        return static_cast<std::uint16_t>(p[0] | (p[1] << 8));
    }
    std::uint32_t u32() noexcept {
        if (!take(4)) {
            return 0;
        }
        const std::uint8_t* p = data_ + pos_ - 4;
        return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
               (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
    }
    std::int32_t i32() noexcept { return static_cast<std::int32_t>(u32()); }

    // Copies n bytes (zeros when they are not there).
    void bytes(void* out, std::size_t n) noexcept {
        if (!take(n)) {
            std::memset(out, 0, n);
            return;
        }
        std::memcpy(out, data_ + pos_ - n, n);
    }
    std::vector<std::uint8_t> bytes(std::size_t n) {
        if (!take(n)) {
            return {};
        }
        return std::vector<std::uint8_t>(data_ + pos_ - n, data_ + pos_);
    }
    void skip(std::size_t n) noexcept { take(n); }

private:
    bool take(std::size_t n) noexcept {
        if (!canRead(n)) {
            ok_ = false;
            pos_ = size_;
            return false;
        }
        pos_ += n;
        return true;
    }

    const std::uint8_t* data_;
    std::size_t size_;
    std::size_t pos_ = 0;
    bool ok_ = true;
};

// Little-endian writer appending to a byte vector.
class ByteWriter {
public:
    void u8(std::uint8_t v) { out_.push_back(v); }
    void u16(std::uint16_t v) {
        out_.push_back(static_cast<std::uint8_t>(v & 0xFFu));
        out_.push_back(static_cast<std::uint8_t>(v >> 8));
    }
    void u32(std::uint32_t v) {
        for (int i = 0; i < 4; ++i) {
            out_.push_back(static_cast<std::uint8_t>(v & 0xFFu));
            v >>= 8;
        }
    }
    void i32(std::int32_t v) { u32(static_cast<std::uint32_t>(v)); }
    void bytes(const void* data, std::size_t n) {
        const auto* p = static_cast<const std::uint8_t*>(data);
        out_.insert(out_.end(), p, p + n);
    }
    void bytes(const std::vector<std::uint8_t>& v) { out_.insert(out_.end(), v.begin(), v.end()); }
    void fill(std::uint8_t value, std::size_t n) { out_.insert(out_.end(), n, value); }
    // Pads with zeros to a multiple of alignment.
    void align(std::size_t alignment) {
        while (out_.size() % alignment != 0) {
            out_.push_back(0);
        }
    }
    // Overwrites 4 bytes at offset (for sizes and offsets known later).
    void patchU32(std::size_t offset, std::uint32_t v) {
        for (std::size_t i = 0; i < 4; ++i) {
            out_[offset + i] = static_cast<std::uint8_t>(v & 0xFFu);
            v >>= 8;
        }
    }

    std::size_t size() const noexcept { return out_.size(); }
    const std::vector<std::uint8_t>& data() const noexcept { return out_; }
    std::vector<std::uint8_t> take() { return std::move(out_); }

private:
    std::vector<std::uint8_t> out_;
};

}  // namespace bb::core::assets
