// Smurfette (ennemi_smurfette.cpp), her kiss (tir_bisou_mortel.h) and the
// stork (ennemi_smurf_cigogne.cpp).

#include "bb/game/trig.hpp"
#include "smurf/smurf_world.hpp"

namespace bb::game::smurf {

Smurfette::Smurfette(World& world) : Enemy(world) {
    shotDelay_ = 100 + world.random() % 100;
    hitPoints = 200;
}

void Smurfette::update() {
    if (bleeding > 0) {
        bleeding -= 1;
    }
    shotWait_ += 1;

    switch (mode) {
        case kNormal:
        case kWalking:
            walkAbout();
            break;
        case kDying:
            die();
            break;
        case kShooting:
            blowKiss();
            break;
        case kCarbonised:
            burn(img::kSmurfBurnLeft, img::kSmurfBurnRight, 6);
            break;
        default:
            break;
    }

    if (x < world().camera.x - 20) {
        dead = true;
    }
    dieWhenOffscreen();
}

void Smurfette::walkAbout() {
    static const int kSpeed = 1;

    const int camera = world().camera.x;
    fall();

    // Turns at walls and at the edges of her platform.
    if (x - kSpeed < minX || solidWall(x - kSpeed, y) ||
        (platformAt(x - kSpeed, y) == 0 && platformAt(x - kSpeed, y + 5) == 0)) {
        direction = kFacingRight;
    } else if (x + kSpeed > camera + 640 || solidWall(x + kSpeed, y) ||
               (platformAt(x + kSpeed, y) == 0 && platformAt(x + kSpeed, y + 5) == 0)) {
        direction = kFacingLeft;
    }

    if (direction == kFacingRight) {
        walk(kSpeed);
        image = enemyImage(img::kSmurfetteWalkRight + animate(kWalkCycle, kWalkSteps, 6));
    } else {
        walk(-kSpeed);
        image = enemyImage(img::kSmurfetteWalkLeft + animate(kWalkCycle, kWalkSteps, 6));
    }

    if (shotWait_ >= shotDelay_ && x > camera + 10 && x < camera + 630) {
        frame = subFrame = 0;
        mode = kShooting;
        blowKiss();
        return;
    }
    hitboxFromImage();
}

void Smurfette::die() {
    subFrame += 1;
    subFrame %= 5;
    if (subFrame == 0) {
        frame += 1;
        if (frame >= 11) {
            dead = true;
            world().media.effects->engrave(x, y, image);
            return;
        }
    }
    image =
        enemyImage((direction == kFacingRight ? img::kSmurfetteDieRight : img::kSmurfetteDieLeft) + frame);
}

void Smurfette::blowKiss() {
    static const int kBlow[] = {0, 1, 2, 3, 4, 3, 2, 1, 0};

    World& w = world();

    subFrame += 1;
    subFrame %= 5;
    if (subFrame == 0) {
        frame += 1;

        if (frame == 4 && w.target != nullptr) {
            auto* kiss = new DeadlyKiss(w);
            kiss->x = x;
            kiss->y = y - 25;
            kiss->direction = direction;
            kiss->target = w.target;
            w.enemyShots.add(kiss);
        } else if (frame >= 9) {
            mode = kWalking;
            frame = subFrame = shotWait_ = 0;
            shotDelay_ = 200 + w.random() % 100;
            walkAbout();
            return;
        }
    }

    image = enemyImage((direction == kFacingRight ? img::kSmurfetteKissRight : img::kSmurfetteKissLeft) +
                       kBlow[frame]);
    hitboxFromImage();
}

void Smurfette::hitBy(Shot& shot) {
    static const int kBloodX[] = {0, 0, 4, 0, 0, 0, -4, 0};
    static const int kBloodY[] = {-15, -15, -15, -15, -15, -15, -15, -15};

    Enemy::hitBy(shot);
    sprayBlood(shot, kBloodX, kBloodY);

    if (mode == kDying) {
        world().media.levelSounds->play(snd::kSmurfetteDeath);
    }
}

//-----------------------------------------------------------------------------

void DeadlyKiss::update() {
    static const int kAcceleration = 1;

    subFrame += 1;
    subFrame %= 5;
    if (subFrame == 0) {
        frame += 1;
        frame %= 2;
    }

    direction = xSpeed > 0 ? kFacingRight : kFacingLeft;
    image = enemyImage((direction == kFacingRight ? img::kKissRight : img::kKissLeft) + frame);

    x += xSpeed / 13;
    y += dy / 13;

    lifetime -= 1;
    if (lifetime <= 0 || !target || target->dead) {
        dead = true;
        return;
    }

    // Steers towards the target's chest.
    const int targetX = target->x;
    const int targetY = target->y - 15;

    if (targetX > x && xSpeed < 36) {
        xSpeed += kAcceleration;
    } else if (xSpeed > -36) {
        xSpeed -= kAcceleration;
    }
    if (targetY > y && dy < 36) {
        dy += kAcceleration;
    } else if (dy > -36) {
        dy -= kAcceleration;
    }

    hitboxFromImage();
    dieWhenOffscreen();
}

//-----------------------------------------------------------------------------

Stork::Stork(World& world) : Enemy(world) {
    image = enemyImage(img::kStork);
    noHitbox();
}

void Stork::update() {
    World& w = world();

    if (firstUpdate_) {
        baseY_ = y;
        x -= 100;
        baseX_ = x;
        firstUpdate_ = false;
    }

    // Waits for the five strongmen and the Big Smurf.
    if (w.flags[kBossKills] < 6) {
        return;
    }

    x += 4;
    y = baseY_ + ((100 * sine1024(((x - baseX_) / 4) % 360)) >> kTrigShift);

    if (x > w.camera.x + 750 && w.flags[kFightStage] < 3) {
        w.flags[kFightStage] = 3;
    }
}

}  // namespace bb::game::smurf
