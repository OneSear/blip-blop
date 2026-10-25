// The farmer Smurf (ennemi_smurf_paysan.cpp).

#include "smurf/smurf_world.hpp"

namespace bb::game::smurf {

namespace {

const int kCharge[] = {0, 1, 2, 3};
const int kHeadlessRight[] = {0, 1, 2, 3};
const int kHeadlessLeft[] = {0, 1, 2, 2};  // (2002 table: the last image repeats)

}  // namespace

FarmerSmurf::FarmerSmurf(World& world) : Enemy(world) {
    health.points = 200;
    mode = kWalking;
}

void FarmerSmurf::draw() {
    Actor::draw();
    if (mode == kDying && headFrame_ < 5) {
        drawAt(headX_, headY_,
               enemyImage((direction == kFacingLeft ? img::kFarmerHeadLeft : img::kFarmerHeadRight) +
                          headFrame_));
    }
}

void FarmerSmurf::update() {
    if (bleeding != 0) {
        bleeding -= 1;
    }

    switch (mode) {
        case kDying:
            loseHead();
            run();  // the body runs on
            break;
        case kNormal:
        case kWalking:
            run();
            break;
        case kCarbonised:
            burn(img::kSmurfBurnLeft, img::kSmurfBurnRight, 6);
            break;
        default:
            break;
    }
    dieWhenOffscreen();
}

void FarmerSmurf::loseHead() {
    if (headFrame_ == 0) {
        headX_ = x;
        headY_ = y - 25;
    }
    if (headFrame_ < 5) {
        headSubFrame_ += 1;
        headSubFrame_ %= 4;
        if (headSubFrame_ == 0) {
            headFrame_ += 1;
        }
    }
}

void FarmerSmurf::run() {
    fall();

    // Turns at walls only.
    if (solidWall(x - kSpeed, y)) {
        direction = kFacingRight;
    } else if (solidWall(x + kSpeed, y)) {
        direction = kFacingLeft;
    }

    if (mode == kDying) {
        if (direction == kFacingRight) {
            walk(kSpeed);
            image = enemyImage(img::kFarmerHeadlessRight + animation.loop(kHeadlessRight, 4, 3));
        } else {
            walk(-kSpeed);
            image = enemyImage(img::kFarmerHeadlessLeft + animation.loop(kHeadlessLeft, 4, 3));
        }
    } else {
        if (direction == kFacingRight) {
            walk(kSpeed);
            image = enemyImage(img::kFarmerChargeRight + animation.loop(kCharge, 4, 3));
        } else {
            walk(-kSpeed);
            image = enemyImage(img::kFarmerChargeLeft + animation.loop(kCharge, 4, 3));
        }
        hitboxFromImage();
    }
}

void FarmerSmurf::hitBy(Shot& shot) {
    static const int kBloodX[] = {0, 0, 4, 0, 0, 0, -4, 0};
    static const int kBloodY[] = {-15, -15, -15, -15, -15, -15, -15, -15};

    Enemy::hitBy(shot);
    sprayBlood(shot, kBloodX, kBloodY);

    if (mode == kDying) {
        world().media.miscSounds->play(miscsnd::kDeathCry + world().random() % 3);  // one of 3
    }
}

}  // namespace bb::game::smurf
