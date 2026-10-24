// The prankster Smurf (ennemi_smurf_farceur.cpp) and his gift
// (tir_cadeau.cpp).

#include "smurf/smurf_world.hpp"

namespace bb::game::smurf {

PranksterSmurf::PranksterSmurf(World& world) : Enemy(world) {
    hitPoints = 150;
    giftDelay_ = 100 + world.random() % 400;
    newGiftDelay_ = 50 + world.random() % 200;
}

void PranksterSmurf::draw() {
    if (carrying_) {
        drawAt(direction == kFacingRight ? x + 26 : x - 26, y - giftHeight_, enemyImage(img::kGift));
    }
    Actor::draw();
}

void PranksterSmurf::update() {
    if (bleeding != 0) {
        bleeding -= 1;
    }

    switch (mode) {
        case kNormal:
        case kWalking:
            walkAbout();
            break;
        case kDying:
            die();
            break;
        case kWaiting:
            putGiftDown();
            break;
        case kTakingOut:
            takeOutGift();
            break;
        case kCarbonised:
            burn(img::kPranksterBurnLeft, img::kPranksterBurnRight, 11);
            break;
        default:
            break;
    }
    dieWhenOffscreen();
}

void PranksterSmurf::takeOutGift() {
    subFrame += 1;
    subFrame %= 8;
    if (subFrame == 0) {
        frame += 1;
    }

    image = enemyImage(
        (direction == kFacingRight ? img::kPranksterTakeOutRight : img::kPranksterTakeOutLeft) + frame);

    if (frame > 4) {
        mode = kNormal;
        carrying_ = true;
        frame = 0;
        subFrame = 0;
        walkAbout();
    }
}

void PranksterSmurf::putGiftDown() {
    // The gift goes gently down to the ground...
    subFrame += 1;
    subFrame %= 5;
    if (subFrame == 0) {
        giftHeight_ -= 1;
    }

    // ...and is left there.
    if (giftHeight_ < 0) {
        mode = kNormal;
        carrying_ = false;
        giftHeight_ = 5;
        newGiftWait_ = 0;
        newGiftDelay_ = 50 + world().random() % 200;

        auto* gift = new Gift(world());
        gift->x = direction == kFacingRight ? x + 26 : x - 26;
        gift->y = y;
        world().enemyShots.add(gift);

        frame = 0;
        subFrame = 0;
    }
}

void PranksterSmurf::walkAbout() {
    World& w = world();
    fall();

    if (carrying_) {
        // Puts the gift down after a while.
        giftWait_ += 1;
        if (giftWait_ > giftDelay_ && platformAt(x, y) != 0) {
            mode = kWaiting;
            giftWait_ = 0;
            giftDelay_ = 50 + w.random() % 250;
            frame = 0;
            subFrame = 0;
        }
    } else if (platformAt(x, y) != 0) {
        // Takes out another one after a while.
        newGiftWait_ += 1;
        if (newGiftWait_ > newGiftDelay_) {
            newGiftWait_ = 0;
            newGiftDelay_ = 50 + w.random() % 200;
            mode = kTakingOut;
            frame = 0;
            subFrame = 0;
        }
    }

    // Turns at obstacles.
    if (x - kSpeed < minX || solidWall(x - kSpeed, y)) {
        direction = kFacingRight;
    } else if (x + kSpeed > w.camera.x + 640 || solidWall(x + kSpeed, y)) {
        direction = kFacingLeft;
    }

    if (direction == kFacingRight) {
        walk(kSpeed);
        image = enemyImage((carrying_ ? img::kPranksterWalkRight : img::kSmurfWalkRight) +
                           animate(kWalkCycle, kWalkSteps, 4));
    } else {
        walk(-kSpeed);
        image = enemyImage((carrying_ ? img::kPranksterWalkLeft : img::kSmurfWalkLeft) +
                           animate(kWalkCycle, kWalkSteps, 4));
    }
    hitboxFromImage();
}

void PranksterSmurf::die() {
    fall();

    if (carrying_) {
        // The gift goes off in his hands.
        mode = kCarbonised;
        carrying_ = false;
        auto* gift = new Gift(world());
        gift->frame = 20;
        gift->y = y - 5;
        gift->x = direction == kFacingRight ? x + 26 : x - 26;
        world().enemyShots.add(gift);
        burn(img::kPranksterBurnLeft, img::kPranksterBurnRight, 11);
        return;
    }

    subFrame += 1;
    subFrame %= 4;
    if (subFrame == 0) {
        frame += 1;
    }
    if (frame > 10) {
        dead = true;
        return;
    }

    if (direction == kFacingLeft) {
        if (!solidWall(x - kSpeed, y) && platformAt(x, y) == 0) {
            x -= kSpeed;
        }
        image = enemyImage(img::kSmurfDieLeft + frame);
    } else {
        if (!solidWall(x + kSpeed, y) && platformAt(x, y) == 0) {
            x += kSpeed;
        }
        image = enemyImage(img::kSmurfDieRight + frame);
    }
}

void PranksterSmurf::hitBy(Shot& shot) {
    static const int kBloodX[] = {0, 0, 6, 0, 0, 0, -6, 0};
    static const int kBloodY[] = {-15, -15, -15, -15, -15, -15, -15, -15};

    Enemy::hitBy(shot);
    sprayBlood(shot, kBloodX, kBloodY);

    if (mode == kDying) {
        if (carrying_) {
            world().media.miscSounds->play(miscsnd::kExplosion);
        } else {
            world().media.levelSounds->play(snd::kSmurfDeath + world().random() % 10);
        }
    }
}

//-----------------------------------------------------------------------------

Gift::Gift(World& world) : Projectile(world) {
    image = enemyImage(img::kGift);
    noHitbox();
}

void Gift::update() {
    if (!solidWall(x, y)) {
        fall();
    }

    subFrame += 1;
    subFrame %= 6;
    if (subFrame == 0) {
        frame += 1;
        if (frame == 20) {
            world().media.miscSounds->play(miscsnd::kExplosion);
        }
    }

    // Explodes from step 20; only the first steps of the blast hurt.
    if (frame >= 20) {
        collides = true;
        image = enemyImage(img::kGiftExplosion + frame - 20);
    }
    if (frame > 35) {
        dead = true;
    }
    hitboxFromImage();
    if (frame >= 30) {
        noHitbox();
    }
}

}  // namespace bb::game::smurf
