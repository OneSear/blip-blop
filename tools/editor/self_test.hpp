#pragma once

// bbedit --self-test: drives the real interface with SDL input events (keys,
// clicks, drags) and checks the level after each gesture. Run by ctest
// (test editor.ui) with SDL's dummy video driver, so it needs no screen.

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

struct SDL_Window;

namespace bb::editor {

class EditorApp;
class LevelDocument;

class SelfTest {
public:
    SelfTest(EditorApp& app, SDL_Window* window);

    // Called before each frame: queues this frame's input. False once the
    // test is over.
    bool beforeFrame();
    int failures() const noexcept { return failures_; }

private:
    void check(bool ok, const std::string& what);
    void key(int sym, int scancode, int mod);
    void moveTo(float wx, float wy);
    void button(bool down, bool right = false);
    const LevelDocument& doc() const;

    EditorApp& app_;
    unsigned int windowId_;
    std::vector<std::function<void()>> steps_;
    std::size_t next_ = 0;
    int failures_ = 0;
    float mouseX_ = 0;
    float mouseY_ = 0;
    // Between the steps.
    std::size_t eventsBefore_ = 0;
    std::vector<std::int32_t> layerBefore_;
    int enemy_ = -1;
    std::int32_t enemyX_ = 0;
    std::int32_t enemyY_ = 0;
};

}  // namespace bb::editor
