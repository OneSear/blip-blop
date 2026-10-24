// Presents a frame through the SDL 2 display and reads it back: checks the
// texture format (channel order) and that nothing is blended or shifted.
// Runs without a screen: SDL's dummy video driver with the software renderer.

#include <doctest.h>

#include <SDL.h>

#include <string>

#include "bb/core/gfx/pixmap.hpp"
#include "bb/platform/sdl2/sdl2_platform.hpp"

namespace gfx = bb::core::gfx;

TEST_CASE("the SDL 2 display shows frames unchanged") {
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);

    std::string error;
    auto platform = bb::platform::sdl2::createPlatform({}, error);
    REQUIRE_MESSAGE(platform != nullptr, error);

    bb::platform::DisplayOptions options;
    options.vsync = false;
    auto display = platform->createDisplay(options, error);
    REQUIRE_MESSAGE(display != nullptr, error);

    gfx::Pixmap frame(640, 480);
    for (int y = 0; y < 480; ++y) {
        for (int x = 0; x < 640; ++x) {
            frame.at(x, y) = gfx::makePixel(static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y),
                                            static_cast<std::uint8_t>((x * 7 + y * 3) & 0xFF));
        }
    }
    gfx::Rect red{10, 10, 20, 20};
    gfx::fill(frame, &red, gfx::makePixel(255, 0, 0));

    display->present(frame);

    gfx::Pixmap shown;
    REQUIRE(bb::platform::sdl2::readBack(*display, shown));
    REQUIRE(shown.width() == 640);
    REQUIRE(shown.height() == 480);

    int mismatches = 0;
    for (int y = 0; y < 480; ++y) {
        for (int x = 0; x < 640; ++x) {
            if ((shown.at(x, y) & gfx::kRgbMask) != (frame.at(x, y) & gfx::kRgbMask)) {
                ++mismatches;
            }
        }
    }
    CHECK(mismatches == 0);
    CHECK(gfx::red(shown.at(15, 15)) == 255);
    CHECK(gfx::blue(shown.at(15, 15)) == 0);
}
