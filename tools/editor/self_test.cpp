#include "self_test.hpp"

#include <SDL.h>

#include <cstdio>
#include <cstdlib>

#include "editor_app.hpp"

namespace bb::editor {

namespace {

// The first event of a type at or after x (-1: none).
int findEvent(const LevelDocument& doc, LevelEventType type, std::int32_t x) {
    const auto& events = doc.data().events;
    int best = -1;
    for (std::size_t i = 0; i < events.size(); ++i) {
        if (events[i].type == static_cast<std::int32_t>(type) && events[i].x >= x &&
            (best < 0 || events[i].x < events[static_cast<std::size_t>(best)].x)) {
            best = static_cast<int>(i);
        }
    }
    return best;
}

}  // namespace

SelfTest::SelfTest(EditorApp& app, SDL_Window* window) : app_(app), windowId_(SDL_GetWindowID(window)) {

    steps_ = {
        // A few frames first: the layout (the menu bar) settles.
        [] {},
        [] {},
        [] {},
        // Place: a basic Smurf (the palette's default) at (1500, 300).
        [this] {
            check(app_.document() != nullptr, "snuf1 is open");
            eventsBefore_ = doc().data().events.size();
            key(SDLK_2, SDL_SCANCODE_2, KMOD_NONE);
        },
        [this] { moveTo(1500, 300); },
        [this] { button(true); },
        [this] { button(false); },
        [this] {
            check(doc().data().events.size() == eventsBefore_ + 1, "a click with the place tool adds an event");
            const int placed = findEvent(doc(), LevelEventType::Enemy, 1500);
            if (placed >= 0) {
                std::printf("  (placed at %d, %d)\n", doc().data().events[static_cast<std::size_t>(placed)].x,
                            doc().data().events[static_cast<std::size_t>(placed)].y);
            }
            check(placed >= 0 && doc().data().events[static_cast<std::size_t>(placed)].x == 1500 &&
                      doc().data().events[static_cast<std::size_t>(placed)].y == 300,
                  "the enemy is at the clicked point (1500, 300)");
            check(placed >= 0 && doc().data().events[static_cast<std::size_t>(placed)].activationX == 750,
                  "its activation x is x - 750 (facing left)");
            check(doc().dirty(), "the level is changed");
            key(SDLK_z, SDL_SCANCODE_Z, KMOD_LCTRL);
        },
        [this] {
            check(doc().data().events.size() == eventsBefore_, "Ctrl+Z removes it");
            check(!doc().dirty(), "the level is as saved again");
            // Platforms: a stroke on layer 0 from x 2000 to 2100 at height 200.
            layerBefore_ = doc().data().platforms[0];
            key(SDLK_3, SDL_SCANCODE_3, KMOD_NONE);
        },
        [this] { moveTo(2000, 200); },
        [this] { button(true); },
        [this] { moveTo(2050, 200); },
        [this] { moveTo(2100, 200); },
        [this] { button(false); },
        [this] {
            const auto& layer = doc().data().platforms[0];
            bool painted = true;
            for (int x = 1997; x <= 2103; ++x) {  // brush width 4: 3 columns each side
                painted = painted && layer[static_cast<std::size_t>(x)] == 200;
            }
            check(painted, "the stroke sets every column from 1997 to 2103 to 200");
            check(layer[1996] == layerBefore_[1996] && layer[2104] == layerBefore_[2104], "and no other column");
            key(SDLK_z, SDL_SCANCODE_Z, KMOD_LCTRL);
        },
        [this] {
            check(doc().data().platforms[0] == layerBefore_, "one Ctrl+Z undoes the whole stroke");
            // Select and drag the first enemy at or after x 1400 by 40 pixels.
            enemy_ = findEvent(doc(), LevelEventType::Enemy, 1400);
            check(enemy_ >= 0, "snuf1 has an enemy after x 1400");
            if (enemy_ >= 0) {
                enemyX_ = doc().data().events[static_cast<std::size_t>(enemy_)].x;
                enemyY_ = doc().data().events[static_cast<std::size_t>(enemy_)].y;
            }
            key(SDLK_1, SDL_SCANCODE_1, KMOD_NONE);
        },
        [this] { moveTo(static_cast<float>(enemyX_), static_cast<float>(enemyY_ - 10)); },
        [this] { button(true); },
        [this] { moveTo(static_cast<float>(enemyX_ + 20), static_cast<float>(enemyY_ - 10)); },
        [this] { moveTo(static_cast<float>(enemyX_ + 40), static_cast<float>(enemyY_ - 10)); },
        [this] { button(false); },
        [this] {
            const int moved = findEvent(doc(), LevelEventType::Enemy, enemyX_ + 40);
            check(moved >= 0 && doc().data().events[static_cast<std::size_t>(moved)].x == enemyX_ + 40 &&
                      doc().data().events[static_cast<std::size_t>(moved)].y == enemyY_,
                  "dragging the enemy moves it by 40 pixels");
            check(doc().data().events.size() == eventsBefore_, "moving adds no event");
            key(SDLK_z, SDL_SCANCODE_Z, KMOD_LCTRL);
        },
        [this] {
            check(!doc().dirty(), "one Ctrl+Z undoes the whole drag");
            // Delete: select the enemy again, press Delete.
        },
        [this] { moveTo(static_cast<float>(enemyX_), static_cast<float>(enemyY_ - 10)); },
        [this] { button(true); },
        [this] { button(false); },
        [this] { key(SDLK_DELETE, SDL_SCANCODE_DELETE, KMOD_NONE); },
        [this] {
            std::printf("  (%zu events, %zu before)\n", doc().data().events.size(), eventsBefore_);
            check(doc().data().events.size() == eventsBefore_ - 1, "Delete removes the selected enemy");
            key(SDLK_z, SDL_SCANCODE_Z, KMOD_LCTRL);
        },
        [this] { check(!doc().dirty() && doc().data().events.size() == eventsBefore_, "and Ctrl+Z brings it back"); },
        [] {},
    };
}

const LevelDocument& SelfTest::doc() const {
    return *app_.document();
}

bool SelfTest::beforeFrame() {
    if (next_ >= steps_.size()) {
        std::printf("bbedit self-test: %s (%d failure%s)\n", failures_ == 0 ? "passed" : "FAILED", failures_,
                    failures_ == 1 ? "" : "s");
        return false;
    }
    steps_[next_++]();
    return true;
}

void SelfTest::check(bool ok, const std::string& what) {
    std::printf("%s %s\n", ok ? "  ok  " : "  FAIL", what.c_str());
    if (!ok) {
        ++failures_;
    }
}

void SelfTest::key(int sym, int scancode, int mod) {
    for (const Uint32 type : {SDL_KEYDOWN, SDL_KEYUP}) {
        SDL_Event e{};
        e.type = type;
        e.key.windowID = windowId_;
        e.key.state = type == SDL_KEYDOWN ? SDL_PRESSED : SDL_RELEASED;
        e.key.keysym.sym = sym;
        e.key.keysym.scancode = static_cast<SDL_Scancode>(scancode);
        // The modifiers are released with the key, as on a keyboard (a held
        // Ctrl turns a click into a right click on macOS).
        e.key.keysym.mod = static_cast<Uint16>(type == SDL_KEYDOWN ? mod : KMOD_NONE);
        SDL_PushEvent(&e);
    }
}

void SelfTest::moveTo(float wx, float wy) {
    mouseX_ = app_.screenX(wx + 0.5f);
    mouseY_ = app_.screenY(wy + 0.5f);
    SDL_Event e{};
    e.type = SDL_MOUSEMOTION;
    e.motion.windowID = windowId_;
    e.motion.x = static_cast<Sint32>(mouseX_);
    e.motion.y = static_cast<Sint32>(mouseY_);
    SDL_PushEvent(&e);
}

void SelfTest::button(bool down, bool right) {
    SDL_Event e{};
    e.type = down ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
    e.button.windowID = windowId_;
    e.button.button = static_cast<Uint8>(right ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT);
    e.button.state = down ? SDL_PRESSED : SDL_RELEASED;
    e.button.clicks = 1;
    e.button.x = static_cast<Sint32>(mouseX_);
    e.button.y = static_cast<Sint32>(mouseY_);
    SDL_PushEvent(&e);
}

}  // namespace bb::editor
