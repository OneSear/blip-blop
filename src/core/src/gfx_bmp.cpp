#include "bb/core/gfx/bmp.hpp"

#include <cstdint>
#include <fstream>
#include <vector>

namespace bb::core::gfx {

namespace {

void put16(std::vector<std::uint8_t>& out, std::uint32_t v) {
    out.push_back(static_cast<std::uint8_t>(v & 0xFFu));
    out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFFu));
}

void put32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    put16(out, v & 0xFFFFu);
    put16(out, v >> 16);
}

}  // namespace

bool writeBmp(const Pixmap& pixmap, const std::string& path) {
    const auto w = static_cast<std::uint32_t>(pixmap.width());
    const auto h = static_cast<std::uint32_t>(pixmap.height());
    const std::uint32_t rowBytes = (w * 3u + 3u) & ~3u;
    const std::uint32_t imageBytes = rowBytes * h;

    std::vector<std::uint8_t> out;
    out.reserve(54u + imageBytes);
    out.push_back('B');
    out.push_back('M');
    put32(out, 54u + imageBytes);  // file size
    put32(out, 0);                 // reserved
    put32(out, 54);                // pixel data offset
    put32(out, 40);                // BITMAPINFOHEADER
    put32(out, w);
    put32(out, h);   // positive: bottom-up rows
    put16(out, 1);   // planes
    put16(out, 24);  // bits per pixel
    put32(out, 0);   // no compression
    put32(out, imageBytes);
    put32(out, 2835);  // 72 dpi
    put32(out, 2835);
    put32(out, 0);
    put32(out, 0);

    for (std::uint32_t y = h; y-- > 0;) {
        const Pixel* row = pixmap.row(static_cast<int>(y));
        for (std::uint32_t x = 0; x < w; ++x) {
            out.push_back(blue(row[x]));
            out.push_back(green(row[x]));
            out.push_back(red(row[x]));
        }
        for (std::uint32_t pad = w * 3u; pad < rowBytes; ++pad) {
            out.push_back(0);
        }
    }

    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(out.data()), static_cast<std::streamsize>(out.size()));
    return static_cast<bool>(file);
}

}  // namespace bb::core::gfx
