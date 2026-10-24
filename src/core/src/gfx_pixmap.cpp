#include "bb/core/gfx/pixmap.hpp"

#include <algorithm>
#include <cstring>

namespace bb::core::gfx {

Rect intersect(const Rect& a, const Rect& b) noexcept {
    if (a.empty() || b.empty()) {
        return {};
    }
    const int x0 = std::max(a.x, b.x);
    const int y0 = std::max(a.y, b.y);
    const int x1 = std::min(a.x + a.w, b.x + b.w);
    const int y1 = std::min(a.y + a.h, b.y + b.h);
    if (x1 <= x0 || y1 <= y0) {
        return {};
    }
    return {x0, y0, x1 - x0, y1 - y0};
}

Pixmap::Pixmap(int width, int height, Pixel fillColor)
    : width_(std::max(width, 0)),
      height_(std::max(height, 0)),
      pixels_(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_), fillColor) {}

Rect blit(Pixmap& dst, int x, int y, const Pixmap& src, const Rect* srcRect) {
    // Source rectangle clipped to the source.
    Rect s = src.bounds();
    int dx = x;
    int dy = y;
    if (srcRect != nullptr) {
        const Rect clipped = intersect(*srcRect, s);
        if (clipped.empty()) {
            return {};
        }
        dx += clipped.x - srcRect->x;
        dy += clipped.y - srcRect->y;
        s = clipped;
    }
    // Destination rectangle clipped to the destination.
    const Rect d0{dx, dy, s.w, s.h};
    const Rect d = intersect(d0, dst.bounds());
    if (d.empty()) {
        return {};
    }
    s.x += d.x - d0.x;
    s.y += d.y - d0.y;
    s.w = d.w;
    s.h = d.h;

    const auto width = static_cast<std::size_t>(d.w);
    const auto key = src.colorKey();
    if (!key) {
        // Plain copy; handles overlap like memmove, choosing the row order.
        const bool bottomUp = (&src == &dst) && (d.y > s.y);
        for (int i = 0; i < d.h; ++i) {
            const int r = bottomUp ? d.h - 1 - i : i;
            std::memmove(dst.row(d.y + r) + d.x, src.row(s.y + r) + s.x, width * sizeof(Pixel));
        }
        return d;
    }

    const Pixel k = *key & kRgbMask;
    for (int r = 0; r < d.h; ++r) {
        const Pixel* sp = src.row(s.y + r) + s.x;
        Pixel* dp = dst.row(d.y + r) + d.x;
        for (std::size_t i = 0; i < width; ++i) {
            if ((sp[i] & kRgbMask) != k) {
                dp[i] = sp[i];
            }
        }
    }
    return d;
}

void fill(Pixmap& dst, const Rect* rect, Pixel color) {
    const Rect r = rect != nullptr ? intersect(*rect, dst.bounds()) : dst.bounds();
    if (r.empty()) {
        return;
    }
    for (int y = r.y; y < r.y + r.h; ++y) {
        std::fill_n(dst.row(y) + r.x, static_cast<std::size_t>(r.w), color);
    }
}

void darken(Pixmap& dst, const Rect* rect) {
    const Rect r = rect != nullptr ? intersect(*rect, dst.bounds()) : dst.bounds();
    if (r.empty()) {
        return;
    }
    for (int y = r.y; y < r.y + r.h; ++y) {
        Pixel* p = dst.row(y) + r.x;
        for (int i = 0; i < r.w; ++i) {
            const Pixel v = p[i];
            p[i] = kAlphaMask | ((v & 0xFFu) >> 1) | (((v & 0xFF00u) >> 1) & 0xFF00u) |
                   (((v & 0xFF0000u) >> 1) & 0xFF0000u);
        }
    }
}

}  // namespace bb::core::gfx
