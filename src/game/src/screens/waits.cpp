// The waiting screens (screens/screen_runner.cpp).

#include "bb/game/waits.hpp"

namespace bb::game {

Frame WaitReleaseScreen::update(ScreenStack& stack) {
    services_.input->update();
    if (!services_.input->anyHeld() || services_.session->quitRequested()) {
        stack.finish();
        return Frame::Continue;
    }
    return Frame::Wait;
}

Frame WaitKeyScreen::update(ScreenStack& stack) {
    services_.input->update();
    if (services_.session->quitRequested()) {
        key_ = 0;
        stack.finish();
        return Frame::Continue;
    }
    if (services_.input->firstHeld(key_)) {
        stack.finish();
        return Frame::Continue;
    }
    return Frame::Wait;
}

Frame DelayScreen::update(ScreenStack& stack) {
    if (services_.session->deterministic()) {
        if (elapsed_ >= milliseconds_) {
            stack.finish();
            return Frame::Continue;
        }
        elapsed_ += 1000 / 60;
        return Frame::Wait;
    }

    const unsigned long long now = services_.session->nowMicroseconds();
    if (!started_) {
        started_ = true;
        deadline_ = now + static_cast<unsigned long long>(milliseconds_) * 1000u;
    }
    if (now >= deadline_) {
        stack.finish();
        return Frame::Continue;
    }
    return Frame::Wait;
}

}  // namespace bb::game
