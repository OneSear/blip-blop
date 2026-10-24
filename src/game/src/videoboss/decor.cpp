// The props of videoboss (fond_lcd.cpp, fond_worldmap.cpp,
// fond_electronics.cpp, fond_ecraneige.h, fond_ecrancamera.cpp,
// fond_assembleur.cpp, fond_windows.h, fond_dos.h, fond_bossventilo.cpp).

#include "videoboss/videoboss_world.hpp"

namespace bb::game::videoboss {

void TableProp::update() {
    image = levelImage(first_ + animate(table_, steps_, delay_));
}

void Electronics::update() {
    subFrame += 1;
    subFrame %= 2;
    if (subFrame == 0) {
        frame += 1;
        frame %= 15;
    }
    image = levelImage(prop::kElectronics + frame);
}

void Electronics::draw() {
    Actor::draw();
    if (overlay_ >= 0) {
        drawAt(x, y, levelImage(overlay_));
    }
}

void ScreenLoop::update() {
    subFrame += 1;
    subFrame %= delay_;
    if (subFrame == 0) {
        frame += 1;
        frame %= steps_;
    }
    image = levelImage(first_ + frame);
    dieWhenOffscreen();
}

void CameraScreen::update() {
    subFrame += 1;
    subFrame %= 87;
    if (subFrame == 0) {
        frame += 1;
        frame %= 3;
    }
    image = levelImage(prop::kCameraScreen + frame);
}

void CameraScreen::draw() {
    Actor::draw();
    drawAt(x, y + subFrame, levelImage(prop::kCameraScreenBar));
}

void Assembler::update() {
    frame += 1;
    frame %= 432;
}

void Assembler::draw() {
    // An 87-pixel window on the listing, clipped at the right of the
    // screen.
    const int camera = world().camera.x;
    const int width = x + 75 > camera + 640 ? camera + 640 - x : 75;
    world().media.canvas->drawPart(levelImage(prop::kAssembler), 0, frame, width, 87 + frame, x - camera, y);
}

void WindowsScreen::update() {
    subFrame += 1;
    subFrame %= 180;
    if (subFrame == 0) {
        frame += 1;
        frame %= 4;
    }
    // Two steps on the second image (the bank's 55 is unused).
    int picture = prop::kWindows1;
    if (frame == 1 || frame == 2) {
        picture = prop::kWindows2;
    } else if (frame == 3) {
        picture = prop::kWindows3;
    }
    image = levelImage(picture);
    dieWhenOffscreen();
}

}  // namespace bb::game::videoboss
