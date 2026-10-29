#include "bb/game/screen.hpp"

#include <utility>

namespace bb::game {

ScreenStack::~ScreenStack() {
    // Top first, as if each screen had finished.
    while (!screens_.empty()) {
        screens_.pop_back();
    }
}

void ScreenStack::push(std::unique_ptr<Screen> screen) {
    screens_.push_back(std::move(screen));
    ++changes_;
}

void ScreenStack::finish() {
    if (screens_.empty()) {
        return;
    }
    finished_.push_back(std::move(screens_.back()));
    screens_.pop_back();
    ++changes_;
}

void ScreenStack::replace(std::unique_ptr<Screen> screen) {
    finish();
    push(std::move(screen));
}

Frame ScreenStack::step() {
    if (screens_.empty()) {
        return Frame::Wait;
    }
    const std::size_t before = changes_;
    Frame frame = screens_.back()->update(*this);
    // In the order they finished (vector::clear leaves the order open).
    for (auto& screen : finished_) {
        screen.reset();
    }
    finished_.clear();
    if (frame == Frame::Continue && changes_ == before) {
        ++stalled_;
        frame = Frame::Wait;
    }
    return frame;
}

}  // namespace bb::game
