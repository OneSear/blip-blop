#pragma once

#include <memory>
#include <string>

#include "bb/platform/audio.hpp"
#include "bb/platform/clock.hpp"
#include "bb/platform/display.hpp"
#include "bb/platform/input_source.hpp"
#include "bb/platform/system.hpp"

namespace bb::platform {

// Entry point to the platform services. A backend (platform/sdl2, later
// others for consoles) implements it; only the application links a
// backend (migration.md, section 3.2).
class Platform {
public:
    virtual ~Platform() = default;

    // Opens the game window (or a headless stand-in). Returns null and sets
    // error on failure.
    virtual std::unique_ptr<Display> createDisplay(const DisplayOptions& options, std::string& error) = 0;

    virtual Clock& clock() = 0;
    virtual InputSource& input() = 0;
    virtual Audio& audio() = 0;
    virtual System& system() = 0;
};

}  // namespace bb::platform
