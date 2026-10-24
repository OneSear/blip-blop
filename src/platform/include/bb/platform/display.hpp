#pragma once

#include <string>

#include "bb/core/gfx/pixmap.hpp"

namespace bb::platform {

struct DisplayOptions {
    std::string title = "Blip & Blop";
    int width = 640;  // logical size of the frames presented
    int height = 480;
    bool fullscreen = false;
    bool vsync = true;
    bool headless = false;  // never opens a window, present() does nothing
};

// Shows finished frames. Frames are software-rendered pixmaps
// (migration.md DEC-01); the display only uploads and scales them.
class Display {
public:
    virtual ~Display() = default;

    // Shows the frame, scaled to the window with its aspect ratio kept.
    virtual void present(const core::gfx::Pixmap& frame) = 0;

    virtual void setFullscreen(bool fullscreen) = 0;
    virtual bool fullscreen() const = 0;

    // Refresh rate of the screen in Hz, 0 when unknown.
    virtual int refreshRate() const = 0;

    // For the log: the window's size, the size it is drawn at in pixels
    // and the renderer ("" for a headless display).
    virtual std::string describe() const = 0;

    // Whether present() waits for the screen's refresh. setVsync() asks for
    // it on or off and returns whether the display now does as asked (a
    // headless display or a renderer without vsync never syncs).
    virtual bool vsync() const = 0;
    virtual bool setVsync(bool on) = 0;
};

}  // namespace bb::platform
