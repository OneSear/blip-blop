// The props of video (fond_cube.cpp, fond_piece.cpp, fond_ring.cpp,
// fond_tournesol_1.h to fond_tournesol_4.h, fond_drapeau.h).

#include "video/video_world.hpp"

namespace bb::game::video {

void SyncedProp::update() {
    World& w = world();

    if (clock_.phase != w.blink) {
        clock_.phase = w.blink;
        clock_.subFrame += 1;
        clock_.subFrame %= delay_;
        if (clock_.subFrame == 0) {
            clock_.frame += 1;
            clock_.frame %= steps_;
        }
    }
    image = levelImage(first_ + clock_.frame);

    if (x < w.camera.x - 100) {
        dead = true;
    }
}

void LoopProp::update() {
    animation.cycle(delay_, steps_);
    image = levelImage(first_ + animation.frame);
}

}  // namespace bb::game::video
