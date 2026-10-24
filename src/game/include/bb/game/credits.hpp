#pragma once

#include <cstdint>

#include "bb/core/fixed_clock.hpp"
#include "bb/core/gfx/sprite.hpp"
#include "bb/game/screen.hpp"
#include "bb/game/services.hpp"

namespace bb::game {

// The credits scrolling up over the title picture; a key ends them. The
// long version after the last level (theEnd) scrolls on the right over the
// pictures of credits.gfx, changed every 12.5 seconds, and a key does not
// interrupt it.
class CreditsScreen : public Screen {
public:
    CreditsScreen(Services& services, bool theEnd) noexcept : services_(services), theEnd_(theEnd) {}
    Frame update(ScreenStack& stack) override;
    const char* name() const override { return "credits"; }

private:
    Services& services_;
    bool theEnd_;
    bool started_ = false;
    core::gfx::SpriteBank pages_;
    core::FixedClock clock_;
    int page_ = 0;
    std::uint32_t pageStart_ = 0;
    int y_ = 480;
    int subStep_ = 0;
    int lastY_ = 100;
    int textX_ = 320;
};

}  // namespace bb::game
