// Checks that bb::core::gfx reproduces SDL's blits and fills exactly, so the
// legacy game can move off SDL surfaces without changing a single pixel.
// Links SDL; the core library itself does not.

#include <doctest.h>

#include <SDL.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "bb/core/gfx/pixmap.hpp"
#include "bb/core/rng.hpp"

namespace gfx = bb::core::gfx;

namespace {

// Legacy surface layout: R in the low byte, alpha mask in the top byte.
SDL_Surface* makeSurface(int w, int h) {
    SDL_Surface* s = SDL_CreateRGBSurface(0, w, h, 32, 0x000000FFu, 0x0000FF00u, 0x00FF0000u, 0xFF000000u);
    REQUIRE(s != nullptr);
    SDL_SetSurfaceBlendMode(s, SDL_BLENDMODE_NONE);
    return s;
}

std::uint32_t* surfaceRow(SDL_Surface* s, int y) {
    return reinterpret_cast<std::uint32_t*>(static_cast<std::uint8_t*>(s->pixels) +
                                            static_cast<std::ptrdiff_t>(y) * s->pitch);
}

void randomize(SDL_Surface* s, gfx::Pixmap& p, bb::core::Rng& rng, gfx::Pixel key) {
    for (int y = 0; y < p.height(); ++y) {
        for (int x = 0; x < p.width(); ++x) {
            // A third of the pixels carry the key colour (with random alpha
            // bits, which the comparison must ignore).
            const gfx::Pixel v =
                rng.next() % 3 == 0
                    ? (key & gfx::kRgbMask) | (static_cast<gfx::Pixel>(rng.next() & 0xFF) << 24)
                    : (static_cast<gfx::Pixel>(rng.next()) << 17) ^ static_cast<gfx::Pixel>(rng.next());
            p.at(x, y) = v;
            surfaceRow(s, y)[x] = v;
        }
    }
}

bool same(SDL_Surface* s, const gfx::Pixmap& p) {
    for (int y = 0; y < p.height(); ++y) {
        if (std::memcmp(surfaceRow(s, y), p.row(y), p.pitch()) != 0) {
            return false;
        }
    }
    return true;
}

int range(bb::core::Rng& rng, int lo, int hi) {
    return lo + rng.next() % (hi - lo + 1);
}

}  // namespace

TEST_CASE("blit and fill match SDL") {
    bb::core::Rng rng(2024);
    const gfx::Pixel key = gfx::makePixel(246, 205, 148, 0);

    for (int iteration = 0; iteration < 3000; ++iteration) {
        const int sw = range(rng, 1, 40);
        const int sh = range(rng, 1, 40);
        const int dw = range(rng, 1, 40);
        const int dh = range(rng, 1, 40);
        const bool selfBlit = iteration % 5 == 0;
        const bool keyed = iteration % 3 == 0 && !selfBlit;

        SDL_Surface* dstS = makeSurface(dw, dh);
        gfx::Pixmap dstP(dw, dh);
        randomize(dstS, dstP, rng, key);

        SDL_Surface* srcS = dstS;
        gfx::Pixmap srcP;
        if (!selfBlit) {
            srcS = makeSurface(sw, sh);
            srcP = gfx::Pixmap(sw, sh);
            randomize(srcS, srcP, rng, key);
            if (keyed) {
                SDL_SetColorKey(srcS, SDL_TRUE, key);
                srcP.setColorKey(key);
            }
        }

        const bool withRect = rng.next() % 4 != 0;
        SDL_Rect sr{range(rng, -10, 45), range(rng, -10, 45), range(rng, -5, 50), range(rng, -5, 50)};
        gfx::Rect pr{sr.x, sr.y, sr.w, sr.h};
        const int px = range(rng, -50, 50);
        const int py = range(rng, -50, 50);
        SDL_Rect pos{px, py, 0, 0};  // SDL overwrites it with the clipped rectangle

        SDL_BlitSurface(srcS, withRect ? &sr : nullptr, dstS, &pos);
        gfx::blit(dstP, px, py, selfBlit ? dstP : srcP, withRect ? &pr : nullptr);
        INFO("iteration " << iteration << (selfBlit ? " (self)" : keyed ? " (keyed)" : ""));
        CHECK(same(dstS, dstP));

        // Fill a random rectangle as well.
        SDL_Rect fr{range(rng, -10, 45), range(rng, -10, 45), range(rng, -5, 50), range(rng, -5, 50)};
        gfx::Rect fp{fr.x, fr.y, fr.w, fr.h};
        const gfx::Pixel color = static_cast<gfx::Pixel>(rng.next()) * 2654435761u;
        SDL_FillRect(dstS, &fr, color);
        gfx::fill(dstP, &fp, color);
        CHECK(same(dstS, dstP));

        if (srcS != dstS) {
            SDL_FreeSurface(srcS);
        }
        SDL_FreeSurface(dstS);
    }
}
