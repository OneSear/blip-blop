#pragma once

#include <cstddef>
#include <memory>
#include <vector>

namespace bb::game {

// What a screen did with the current frame.
enum class Frame {
    // A complete picture is in the back buffer: present it.
    Present,
    // Nothing new to show (waiting for a key, a delay): let one frame of
    // time pass without presenting.
    Wait,
    // The screen changed the stack (pushed a screen or finished) without
    // using the frame: the new top screen runs in the same frame.
    Continue,
};

class ScreenStack;

// One state of the application: a menu, a level, a cutscene, an overlay.
// The application loop calls update() of the top screen once per step;
// nothing ever blocks inside a screen (migration.md, section 3.1).
class Screen {
public:
    Screen() = default;
    Screen(const Screen&) = delete;
    Screen& operator=(const Screen&) = delete;
    virtual ~Screen() = default;

    // Advances the screen by one step. A screen that pushes another one or
    // finishes (ScreenStack::finish) returns Frame::Continue unless it also
    // drew a frame.
    virtual Frame update(ScreenStack& stack) = 0;

    // Short English name for logs and replay milestones.
    virtual const char* name() const = 0;
};

class ScreenStack {
public:
    ScreenStack() = default;
    ScreenStack(const ScreenStack&) = delete;
    ScreenStack& operator=(const ScreenStack&) = delete;
    ~ScreenStack();

    // Puts a screen on top; it runs from the next step on.
    void push(std::unique_ptr<Screen> screen);

    // Removes the top screen. Called by that screen from its update(): the
    // screen is destroyed once its update() has returned (screens finished
    // in the same step are destroyed in the order they finished).
    void finish();

    // finish() then push(): the top screen hands over to another one, and
    // the screen below resumes when that one finishes.
    void replace(std::unique_ptr<Screen> screen);

    bool empty() const noexcept { return screens_.empty(); }
    std::size_t size() const noexcept { return screens_.size(); }
    Screen& top() const { return *screens_.back(); }

    // Runs update() of the top screen once. A screen that returns
    // Frame::Continue without changing the stack would make the
    // application spin: the step counts as Frame::Wait and
    // stalledSteps() increases (a bug to fix, reported by the caller).
    Frame step();

    std::size_t stalledSteps() const noexcept { return stalled_; }

private:
    std::vector<std::unique_ptr<Screen>> screens_;
    std::vector<std::unique_ptr<Screen>> finished_;
    std::size_t changes_ = 0;
    std::size_t stalled_ = 0;
};

}  // namespace bb::game
