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

TEST_CASE("the SDL 2 display scales by whole factors when asked") {
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);

    std::string error;
    auto platform = bb::platform::sdl2::createPlatform({}, error);
    REQUIRE_MESSAGE(platform != nullptr, error);

    bb::platform::DisplayOptions options;
    options.vsync = false;
    auto display = platform->createDisplay(options, error);
    REQUIRE_MESSAGE(display != nullptr, error);

    // A window 1.5 frames wide and tall: fit scales by 1.5, integer by 1
    // with a border.
    // The display's window: the only one open (ids grow from test to test).
    SDL_Window* window = nullptr;
    for (Uint32 id = 1; id < 100 && window == nullptr; ++id) {
        window = SDL_GetWindowFromID(id);
    }
    REQUIRE(window != nullptr);
    SDL_SetWindowSize(window, 960, 720);
    SDL_PumpEvents();

    gfx::Pixmap frame(640, 480);
    gfx::Rect all{0, 0, 640, 480};
    gfx::fill(frame, &all, gfx::makePixel(255, 255, 255));
    SDL_Renderer* renderer = SDL_GetRenderer(window);
    REQUIRE(renderer != nullptr);

    // Where the frame goes, in window pixels (the read-back only covers
    // this viewport, so the border is checked through it).
    const auto placed = [&](SDL_Rect& at) {
        float scaleX = 0;
        float scaleY = 0;
        SDL_RenderGetViewport(renderer, &at);
        SDL_RenderGetScale(renderer, &scaleX, &scaleY);
        at.x = static_cast<int>(static_cast<float>(at.x) * scaleX);
        at.y = static_cast<int>(static_cast<float>(at.y) * scaleY);
        at.w = static_cast<int>(static_cast<float>(at.w) * scaleX);
        at.h = static_cast<int>(static_cast<float>(at.h) * scaleY);
    };

    SDL_Rect at{};
    gfx::Pixmap shown;
    display->setScaling(false, false);
    display->present(frame);
    placed(at);
    CHECK(at.x == 0);  // fills the window, scaled by 1.5
    CHECK(at.y == 0);
    CHECK(at.w == 960);
    CHECK(at.h == 720);
    CHECK(display->describe().find("scaling fit, filter sharp") != std::string::npos);

    display->setScaling(true, true);
    display->present(frame);
    placed(at);
    CHECK(at.x == 160);  // scale 1, centred: a 160-pixel border
    CHECK(at.y == 120);
    CHECK(at.w == 640);
    CHECK(at.h == 480);
    REQUIRE(bb::platform::sdl2::readBack(*display, shown));
    CHECK(gfx::red(shown.at(0, 0)) == 255);  // the frame's corner is in the viewport
    CHECK(display->describe().find("scaling integer, filter smooth") != std::string::npos);
}
