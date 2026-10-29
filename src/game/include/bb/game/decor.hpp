#pragma once

#include <vector>

#include "bb/core/gfx/pixmap.hpp"
#include "bb/game/media.hpp"

namespace bb::game {

struct World;

// The level's scrolling background (scroll.cpp and grave() of the 2002
// code): the decor screens are copied 2 columns at a time into an
// 840-pixel ring ahead of the camera, and the screen is cut out of the
// ring. Decals (corpses, carcasses, blood on walls) are engraved into the
// ring, so they last until the ring wraps over them.
class Decor : public LevelEffects {
public:
    static constexpr int kRingWidth = 840;  // WANTED_VBUFFER_WIDE

    explicit Decor(World& world);

    // A new level: its decor bank and the decor image of each screen.
    void setLevel(const ImageBank* screens, std::vector<int> screenImages);
    // Back to the first column (level start).
    void rewind() noexcept;

    // Keeps the camera inside the level (from 0 to its width less a
    // screen), as the 2002 drawScrolling() did before drawing.
    void clampCamera() noexcept;
    // Fills the ring up to the camera, then copies the screen into dst.
    // Clamps the camera to the level first.
    void draw(core::gfx::Pixmap& dst);

    // LevelEffects: engraves image into the ring at level position (x, y),
    // unless it is over a prop or a moving platform or outside the part
    // of the level the ring holds.
    bool engrave(int x, int y, const Image* image) override;

private:
    // A clipped blit of image to (x, y) of the ring, inside [left, right]
    // (the 2002 clipedBlit(), whose bounds are inclusive).
    void blitClipped(const Image& image, int x, int y, int left, int right);

    World& world_;
    core::gfx::Pixmap ring_;
    const ImageBank* screens_ = nullptr;
    std::vector<int> screenImages_;  // num_decor

    int ringX_ = 0;      // next_x: the next column of the ring to fill
    int screenX_ = 0;    // xTex: the next column of the decor screen
    int screen_ = 0;     // n_img: the decor screen being copied
    int ringTurns_ = 0;  // n_cache: how many times the ring was filled
};

}  // namespace bb::game
