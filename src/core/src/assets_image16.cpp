#include "bb/core/assets/image16.hpp"

#include <algorithm>

#include "bb/core/assets/byte_io.hpp"

namespace bb::core::assets {

namespace {

// Nearest level of a component with maxLevel steps, matching the expansion
// level * 255 / maxLevel (rounded down) exactly for the values it produces.
unsigned quantise(unsigned value8, unsigned maxLevel) noexcept {
    unsigned best = 0;
    unsigned bestDistance = 256;
    for (unsigned level = 0; level <= maxLevel; ++level) {
        const unsigned expanded = level * 255u / maxLevel;
        const unsigned distance = expanded > value8 ? expanded - value8 : value8 - expanded;
        if (distance < bestDistance) {
            best = level;
            bestDistance = distance;
        }
    }
    return best;
}

}  // namespace

std::uint16_t fromPixel(gfx::Pixel pixel, PixelFormat16 format) noexcept {
    const unsigned r = gfx::red(pixel);
    const unsigned g = gfx::green(pixel);
    const unsigned b = gfx::blue(pixel);
    if (format == PixelFormat16::Rgb565) {
        return static_cast<std::uint16_t>((quantise(r, 31) << 11) | (quantise(g, 63) << 5) | quantise(b, 31));
    }
    return static_cast<std::uint16_t>((quantise(r, 31) << 10) | (quantise(g, 31) << 5) | quantise(b, 31));
}

gfx::Pixmap toPixmap(const Image16& image, bool withColorKey) {
    gfx::Pixmap pixmap(image.width, image.height);
    gfx::Pixel* out = pixmap.data();
    for (const std::uint16_t value : image.pixels) {
        *out++ = toPixel(value, image.format);
    }
    if (withColorKey) {
        pixmap.setColorKey(toPixel(colorKey16(image.format), image.format));
    }
    return pixmap;
}

std::optional<Image16> decodeLgx(const std::uint8_t* data, std::size_t size, std::string& error) {
    ByteReader in(data, size);
    std::uint8_t magic[3];
    in.bytes(magic, 3);
    const std::uint8_t version = in.u8();
    const std::uint16_t width = in.u16();
    const std::uint16_t height = in.u16();
    const std::uint16_t depth = in.u16();
    if (!in.ok() || magic[0] != 'L' || magic[1] != 'G' || magic[2] != 'X') {
        error = "not an LGX image";
        return std::nullopt;
    }
    if (version > 1) {
        error = "unknown LGX version " + std::to_string(version);
        return std::nullopt;
    }
    if (depth != 16) {
        error = "unsupported LGX depth " + std::to_string(depth);
        return std::nullopt;
    }

    Image16 image;
    image.width = width;
    image.height = height;
    image.format = version == 0 ? PixelFormat16::Rgb565 : PixelFormat16::Rgb555;
    const std::size_t count = static_cast<std::size_t>(width) * height;
    // The data must be able to hold that many pixels (a word each, or a run
    // of at most 0x7FFF in two words) before the image is allocated.
    const std::size_t available = in.remaining() / 2;
    if (version == 0 ? available < count : (available / 2) * 0x7FFFu + available < count) {
        error = "truncated LGX pixels";
        return std::nullopt;
    }
    image.pixels.resize(count);
    std::uint16_t* out = image.pixels.data();

    // Words are read straight from the buffer: this runs over 120 MB of art
    // when a game starts, so it must stay fast in debug builds too.
    const std::uint8_t* p = in.current();
    const std::size_t words = in.remaining() / 2;
    const auto word = [p](std::size_t i) {
        return static_cast<std::uint16_t>(p[2 * i] | (p[2 * i + 1] << 8));
    };

    if (version == 0) {
        if (words < count) {
            error = "truncated LGX pixels";
            return std::nullopt;
        }
        for (std::size_t i = 0; i < count; ++i) {
            out[i] = word(i);
        }
        return image;
    }

    std::size_t filled = 0;
    std::size_t w = 0;
    while (filled < count) {
        if (w >= words) {
            error = "truncated LGX pixels";
            return std::nullopt;
        }
        const std::uint16_t value = word(w++);
        if ((value & 0x8000u) == 0) {
            out[filled++] = value;
            continue;
        }
        const std::size_t run = value & 0x7FFFu;
        if (w >= words) {
            error = "truncated LGX pixels";
            return std::nullopt;
        }
        const std::uint16_t color = word(w++);
        if ((color & 0x8000u) != 0) {
            error = "LGX colour word with bit 15 set";
            return std::nullopt;
        }
        if (run > count - filled) {
            error = "LGX run past the end of the image";
            return std::nullopt;
        }
        std::fill(out + filled, out + filled + run, color);
        filled += run;
    }
    return image;
}

std::vector<std::uint8_t> encodeLgx(const Image16& image) {
    std::vector<std::uint8_t> out;
    out.reserve(10 + 2 * image.pixels.size() + 2);
    const auto put = [&out](std::uint16_t v) {
        out.push_back(static_cast<std::uint8_t>(v & 0xFFu));
        out.push_back(static_cast<std::uint8_t>(v >> 8));
    };
    out.push_back('L');
    out.push_back('G');
    out.push_back('X');
    out.push_back(image.format == PixelFormat16::Rgb565 ? 0 : 1);
    put(static_cast<std::uint16_t>(image.width));
    put(static_cast<std::uint16_t>(image.height));
    put(16);

    const std::uint16_t* pixels = image.pixels.data();
    const std::size_t count = image.pixels.size();
    if (image.format == PixelFormat16::Rgb565) {
        for (std::size_t i = 0; i < count; ++i) {
            put(pixels[i]);
        }
        return out;
    }

    std::size_t i = 0;
    while (i < count) {
        const std::uint16_t color = pixels[i];
        std::size_t run = 1;
        while (i + run < count && pixels[i + run] == color && run < 0x7FFF) {
            ++run;
        }
        if (run >= 2) {
            put(static_cast<std::uint16_t>(0x8000u | run));
        }
        put(color);
        i += run;
    }
    return out;
}

}  // namespace bb::core::assets
