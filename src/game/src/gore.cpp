// The generic gore drop (gore_giclure.cpp).

#include "bb/game/gore.hpp"

#include "bb/assets/sprites/misc.hpp"

namespace bb::game {

namespace img = bb::assets::sprites::misc;

GoreDrop::GoreDrop(World& world, int speedX, int speedY) : Actor(world), xSpeed_(speedX) {
    dy = speedY;
    // At least 2 pixels sideways.
    if (xSpeed_ > -2 && xSpeed_ < 0) {
        xSpeed_ = -2;
    } else if (xSpeed_ < 2 && xSpeed_ >= 0) {
        xSpeed_ = 2;
    }
    chooseImage();
}

void GoreDrop::chooseImage() {
    // 7 images by vertical speed, from rising fast to falling fast.
    const int first = xSpeed_ < 0 ? img::kGoreDropLeft : img::kGoreDropRight;
    int step = 0;
    if (dy >= 6) {
        step = 6;
    } else if (dy >= 4) {
        step = 5;
    } else if (dy >= 1) {
        step = 4;
    } else if (dy >= 0) {
        step = 3;
    } else if (dy >= -2) {
        step = 2;
    } else if (dy >= -4) {
        step = 1;
    } else {
        step = 0;
    }
    image = world().media.misc->image(first + step);
}

void GoreDrop::update() {
    chooseImage();
    x += xSpeed_;
    fall();

    if (dy > 0 && platformAt(x, y) != 0) {
        dead = true;
    }
    dieWhenOffscreen();
}

}  // namespace bb::game
