#pragma once

#include <memory>
#include <string>

#include "bb/platform/platform.hpp"

namespace bb::platform::sdl2 {

struct Options {
    // Use SDL's dummy video and audio drivers: no window, no sound device.
    bool headless = false;
};

// Initialises SDL (video, audio, joysticks, events). Returns null and sets
// error on failure. SDL is shut down when the platform is destroyed.
std::unique_ptr<Platform> createPlatform(const Options& options, std::string& error);

// Reads back what the display shows (tests). The display must come from
// this backend and must not be headless. Returns false on failure.
bool readBack(Display& display, core::gfx::Pixmap& out);

}  // namespace bb::platform::sdl2
