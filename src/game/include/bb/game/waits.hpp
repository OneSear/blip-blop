#pragma once

#include <memory>

#include "bb/game/screen.hpp"
#include "bb/game/services.hpp"

namespace bb::game {

// Waits until no key, button or pad direction is held (Input::waitClean());
// finishes at once, without a frame, when nothing is held.
class WaitReleaseScreen : public Screen {
public:
    explicit WaitReleaseScreen(Services& services) noexcept : services_(services) {}
    Frame update(ScreenStack& stack) override;
    const char* name() const override { return "wait for release"; }

private:
    Services& services_;
};

// Waits for a key and stores its code, 0 if the application is closing
// (Input::waitKey()).
class WaitKeyScreen : public Screen {
public:
    WaitKeyScreen(Services& services, unsigned int& key) noexcept : services_(services), key_(key) {}
    Frame update(ScreenStack& stack) override;
    const char* name() const override { return "wait for key"; }

private:
    Services& services_;
    unsigned int& key_;
};

// Lets the given time pass without drawing (Sleep()). In deterministic runs
// one idle frame per 1000/60 ms, rounded down, as the 2014 port's Sleep().
class DelayScreen : public Screen {
public:
    DelayScreen(Services& services, int milliseconds) noexcept
        : services_(services), milliseconds_(milliseconds) {}
    Frame update(ScreenStack& stack) override;
    const char* name() const override { return "delay"; }

private:
    Services& services_;
    int milliseconds_;
    bool started_ = false;
    int elapsed_ = 0;
    unsigned long long deadline_ = 0;
};

// What a menu waits for before it goes on.
enum class MenuWait {
    None,
    Release,  // until no key is held
    Key,      // for a key, stored in the menu
};

// Pushes the screen that waits for it; the owner calls the menu again when
// that screen has finished.
inline void pushMenuWait(Services& services, ScreenStack& stack, MenuWait wait, unsigned int& key) {
    if (wait == MenuWait::Key) {
        stack.push(std::make_unique<WaitKeyScreen>(services, key));
    } else {
        stack.push(std::make_unique<WaitReleaseScreen>(services));
    }
}

}  // namespace bb::game
