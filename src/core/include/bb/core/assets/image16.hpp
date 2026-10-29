#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "bb/core/gfx/pixmap.hpp"

namespace bb::core::assets {

// The two 16-bit layouts of the 2002 art. LGX version 0 images are RGB565,
// version 1 images RGB555 (bit 15 unused). Red is in the high bits.
enum class PixelFormat16 : std::uint8_t {
    Rgb565 = 0,
    Rgb555 = 1,
};

// A 16-bit image exactly as the art was authored: no colour is lost when
// it is decoded from LGX or written back.
struct Image16 {
    int width = 0;
    int height = 0;
    PixelFormat16 format = PixelFormat16::Rgb555;
    std::vector<std::uint16_t> pixels;  // row-major, width * height

    bool operator==(const Image16& o) const {
        return width == o.width && height == o.height && format == o.format && pixels == o.pixels;
    }
    bool operator!=(const Image16& o) const { return !(*this == o); }
};

// The transparent colour of sprite banks, per layout: the editors' key
// RGB (250,206,152) / (250,214,152) after quantisation, i.e. RGB555
// (30,25,18) and RGB565 (30,52,18).
constexpr std::uint16_t colorKey16(PixelFormat16 format) noexcept {
    return format == PixelFormat16::Rgb565 ? static_cast<std::uint16_t>((30u << 11) | (52u << 5) | 18u)
                                           : static_cast<std::uint16_t>((30u << 10) | (25u << 5) | 18u);
}

// 16-bit value to the canonical 32-bit pixel, with the integer expansion of
// the 2002 decoder (component * 255 / max, rounded down) and opaque alpha:
// the key colours become (246,205,148) and (246,210,148).
constexpr gfx::Pixel toPixel(std::uint16_t value, PixelFormat16 format) noexcept {
    if (format == PixelFormat16::Rgb565) {
        const unsigned r = (value >> 11) & 0x1Fu;
        const unsigned g = (value >> 5) & 0x3Fu;
        const unsigned b = value & 0x1Fu;
        return gfx::makePixel(static_cast<std::uint8_t>(r * 255u / 31u),
                              static_cast<std::uint8_t>(g * 255u / 63u),
                              static_cast<std::uint8_t>(b * 255u / 31u));
    }
    const unsigned r = (value >> 10) & 0x1Fu;
    const unsigned g = (value >> 5) & 0x1Fu;
    const unsigned b = value & 0x1Fu;
    return gfx::makePixel(static_cast<std::uint8_t>(r * 255u / 31u),
                          static_cast<std::uint8_t>(g * 255u / 31u),
                          static_cast<std::uint8_t>(b * 255u / 31u));
}

// Inverse of toPixel for the 2^15 / 2^16 values it produces (exact: the
// expansion is strictly increasing). Other colours are rounded to the
// nearest representable level.
std::uint16_t fromPixel(gfx::Pixel pixel, PixelFormat16 format) noexcept;

// Decodes to a pixmap in the canonical format. With withColorKey, the
// layout's key colour is set as the pixmap's colour key.
gfx::Pixmap toPixmap(const Image16& image, bool withColorKey);

// ---------------------------------------------------------------------------
// LGX (docs/formats/lgx.md): 10-byte header 'L','G','X', version (0 or 1),
// width, height, depth (uint16, always 16), then the pixels: raw RGB565
// words (v0) or an RLE stream of RGB555 words (v1).

// nullopt (and error) when the data is not a valid LGX image: bad header,
// truncated pixels, a run past the end of the image, a colour word with
// bit 15 set in a version 1 image.
std::optional<Image16> decodeLgx(const std::uint8_t* data, std::size_t size, std::string& error);

// Encodes as the 2002 editors did (winLGX.cpp): RGB565 as version 0, RGB555
// as version 1 with greedy runs (longest run of equal colours, at most
// 32767, across rows; a run of one is a literal). Re-encoding the shipped
// images gives the shipped bytes.
std::vector<std::uint8_t> encodeLgx(const Image16& image);

}  // namespace bb::core::assets
