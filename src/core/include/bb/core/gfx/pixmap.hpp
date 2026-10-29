#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace bb::core::gfx {

// An axis-aligned rectangle; empty when w <= 0 or h <= 0.
struct Rect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;

    bool empty() const noexcept { return w <= 0 || h <= 0; }
};

// Intersection of two rectangles (empty result when they do not overlap).
Rect intersect(const Rect& a, const Rect& b) noexcept;

// A 32-bit pixel: 0xAABBGGRR as an integer, i.e. bytes R, G, B, A in
// memory on a little-endian machine. This is the layout the legacy SDL
// surfaces used, which keeps the LGX decoder and the frame fingerprints
// unchanged.
using Pixel = std::uint32_t;

constexpr Pixel kAlphaMask = 0xFF000000u;
constexpr Pixel kRgbMask = 0x00FFFFFFu;

constexpr Pixel makePixel(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 0xFF) noexcept {
    return static_cast<Pixel>(r) | (static_cast<Pixel>(g) << 8) | (static_cast<Pixel>(b) << 16) |
           (static_cast<Pixel>(a) << 24);
}
constexpr std::uint8_t red(Pixel p) noexcept {
    return static_cast<std::uint8_t>(p & 0xFFu);
}
constexpr std::uint8_t green(Pixel p) noexcept {
    return static_cast<std::uint8_t>((p >> 8) & 0xFFu);
}
constexpr std::uint8_t blue(Pixel p) noexcept {
    return static_cast<std::uint8_t>((p >> 16) & 0xFFu);
}
constexpr std::uint8_t alpha(Pixel p) noexcept {
    return static_cast<std::uint8_t>(p >> 24);
}

// Owning 2-D pixel buffer with an optional transparent colour key.
class Pixmap {
public:
    Pixmap() = default;
    Pixmap(int width, int height, Pixel fill = kAlphaMask);

    int width() const noexcept { return width_; }
    int height() const noexcept { return height_; }
    Rect bounds() const noexcept { return {0, 0, width_, height_}; }
    // Bytes from one row to the next.
    std::size_t pitch() const noexcept { return static_cast<std::size_t>(width_) * sizeof(Pixel); }

    Pixel* data() noexcept { return pixels_.data(); }
    const Pixel* data() const noexcept { return pixels_.data(); }
    Pixel* row(int y) noexcept {
        return pixels_.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(width_);
    }
    const Pixel* row(int y) const noexcept {
        return pixels_.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(width_);
    }
    Pixel& at(int x, int y) noexcept { return row(y)[x]; }
    Pixel at(int x, int y) const noexcept { return row(y)[x]; }

    // Pixels whose RGB bits equal the key's are skipped when this pixmap is
    // the source of a blit (the alpha bits are not compared).
    void setColorKey(std::optional<Pixel> key) noexcept { colorKey_ = key; }
    std::optional<Pixel> colorKey() const noexcept { return colorKey_; }

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<Pixel> pixels_;
    std::optional<Pixel> colorKey_;
};

// Copies src (or the srcRect part of it) to dst at (x, y), with the
// clipping rules of SDL_BlitSurface: srcRect is clipped to the source and
// the destination origin shifted accordingly, then the result is clipped to
// the destination. Honours the source colour key. src and dst may be the
// same pixmap; overlapping copies behave as if the source were copied to a
// temporary first (like SDL's copy blitter). Returns the rectangle written.
Rect blit(Pixmap& dst, int x, int y, const Pixmap& src, const Rect* srcRect = nullptr);

// Fills rect (the whole pixmap when null), clipped to the pixmap.
void fill(Pixmap& dst, const Rect* rect, Pixel color);

// Halves the red, green and blue of every pixel of rect (the whole pixmap
// when null) and makes them opaque: the "half-tone" behind menus and
// dialogue boxes.
void darken(Pixmap& dst, const Rect* rect);

}  // namespace bb::core::gfx
