#pragma once

#include <sstream>
#include <string>

#include "bb/core/fixed_clock.hpp"
#include "bb/game/screen.hpp"
#include "bb/game/services.hpp"

namespace bb::game {

// Plays a cutscene script (.cin, docs/formats/cin.md; the 2002 CINEPlayer):
// up to 20 objects, sprites of the scene's bank or strings of the string
// table, that move and animate; "renderscene(n)" shows n updates of
// 11.8 ms. Escape (or closing the window) skips the scene.
class CutscenePlayer {
public:
    explicit CutscenePlayer(Services& services);

    // The scene's bank before the script loads one (loadPBK()).
    void loadSprites(const char* name);
    // Starts a scene; false if the script cannot be read.
    bool begin(const char* name);
    // One step: Frame::Present when a frame was drawn into the back buffer,
    // Frame::Continue once the scene is over.
    Frame step();
    bool skipped() const noexcept { return skipped_; }

private:
    static constexpr int kObjects = 20;
    static constexpr int kArguments = 20;
    static constexpr int kArgumentLength = 40;
    static constexpr int kLineLength = 200;
    static constexpr int kCommandLength = 20;

    struct Object {
        enum Kind { None = 0, Sprite = 1, Text = 2 };  // OBJ_SPRITE, OBJ_TEXTE
        int kind = None;
        bool shown = false;
        const Image* image = nullptr;
        int x = 0;
        int y = 0;
        int dx = 0;
        int dy = 0;
        int moveTicks = 0;
        int frames = 0;  // nb_anim: 0 when not animated
        int animation[20] = {};
        int frameDelay = 0;
        int frameWait = 0;
        bool loops = false;
        int frame = 0;
        bool xWarp = false;
        int xWarpFrom = 0;
        int xWarpTo = 0;
        bool yWarp = false;
        int yWarpFrom = 0;
        int yWarpTo = 0;
        int text = 0;
    };

    void reset();
    bool readCommand();  // getCommand()
    bool syntaxError();
    void error(const char* what);
    void runCommands();  // updateState(): up to the next renderscene
    void update();       // updateScene(): one 11.8 ms step
    void renderFrame();
    void draw();
    bool validObject(int n);
    void hide(Object& o);

    Services& services_;
    core::FixedClock clock_;
    std::istringstream script_;
    char line_[kLineLength + 1] = {};
    char cleaned_[kLineLength + 1] = {};
    char command_[kCommandLength] = {};
    int argumentCount_ = 0;
    int intArgument_[kArguments] = {};
    char stringArgument_[kArguments][kArgumentLength] = {};
    int lineNumber_ = 0;

    core::gfx::SpriteBank sprites_;
    MusicBank music_;
    Object objects_[kObjects];

    bool finished_ = false;
    int framesToDraw_ = 0;
    int clipLeft_ = 0;
    int clipTop_ = 0;
    int clipRight_ = 640;
    int clipBottom_ = 480;
    // The colours of the two layers, white or black (setbackcolor,
    // setclipcolor); the drawing uses layer 0's.
    bool whiteBack_[2] = {};
    bool whiteClip_[2] = {};

    bool skipped_ = false;
    bool rendering_ = false;  // inside the frames of a renderscene
};

// Shows a cutscene the player has begun; the player must outlive it.
class CutsceneScreen : public Screen {
public:
    explicit CutsceneScreen(CutscenePlayer& player) : player_(player) {}
    Frame update(ScreenStack& stack) override;
    const char* name() const override { return "cutscene"; }

private:
    CutscenePlayer& player_;
};

}  // namespace bb::game
