#include "bb/core/gfx/fingerprint.hpp"

#include "bb/core/hash.hpp"

namespace bb::core::gfx {

namespace {

std::uint32_t visible(Pixel p) {
    std::uint32_t r = red(p);
    std::uint32_t g = green(p);
    std::uint32_t b = blue(p);
    const std::uint32_t a = alpha(p);
    if (a != 255) {
        r = r * a / 255;
        g = g * a / 255;
        b = b * a / 255;
    }
    return (r << 16) | (g << 8) | b;
}

}  // namespace

std::uint64_t fingerprint(const Pixmap& pixmap) {
    WordHash64 h;
    h.add(static_cast<std::uint32_t>(pixmap.width()), static_cast<std::uint32_t>(pixmap.height()));
    for (int y = 0; y < pixmap.height(); ++y) {
        const Pixel* row = pixmap.row(y);
        int x = 0;
        for (; x + 1 < pixmap.width(); x += 2) {
            h.add(visible(row[x]), visible(row[x + 1]));
        }
        if (x < pixmap.width()) {
            h.add(visible(row[x]));
        }
    }
    return h.value();
}

}  // namespace bb::core::gfx
