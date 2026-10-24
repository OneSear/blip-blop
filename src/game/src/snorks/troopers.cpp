// The Snork troopers (ennemi_sorky_base_1.cpp, ennemi_snorky_base_2.cpp,
// ennemi_snorky_gouverneur.cpp, ennemi_snork_in_black.cpp), the mage
// (ennemi_snorky_mage.cpp) and the seahorse rider
// (ennemi_snorky_hyporider.cpp).

#include "snorks/snorks_world.hpp"

namespace bb::game::snorks {

namespace {

const int kWalkCycle[] = {0, 1, 2, 3, 4, 3, 2, 1};
const int kBloodX[] = {0, 0, 4, 0, 0, 0, -4, 0};
const int kBloodYLow[] = {-15, -15, -15, -15, -25, -15, -15, -15};
const int kBloodYHigh[] = {-15, -15, -15, -15, -35, -15, -15, -15};

}  // namespace

//-----------------------------------------------------------------------------
// Trooper
//-----------------------------------------------------------------------------

void Trooper::update() {
    if (bleeds() && bleeding > 0) {
        bleeding -= 1;
    }
    switch (mode) {
        case kWalking:
        case kNormal:
            walkAbout();
            break;
        case kFalling:
        case kJumping:
            fallDown();
            break;
        case kDying:
            die();
            break;
        case kCarbonised:
            burnUp();
            break;
        case kShooting:
            shoot();
            break;
        default:
            break;
    }
    dieWhenOffscreen();
}

bool Trooper::startFalling() {
    if (platformAt(x, y) != 0) {
        return false;
    }
    mode = kFalling;
    dy = 0;
    gravityDelay = 0;
    fallDown();
    return true;
}

void Trooper::fallDown() {
    fall();
    if (platformAt(x, y) != 0) {
        mode = kWalking;
    }
    hitboxFromImage();
}

void Trooper::stepAndTurn(int speed, int imageRight, int imageLeft, const int* cycle, int steps) {
    if (direction == kFacingLeft) {
        walk(-speed);
        image = enemyImage(imageLeft + animate(cycle, steps, 4));
        if (solidWall(x - speed, y) || x - speed < minX) {
            direction = kFacingRight;
        }
    } else {
        walk(speed);
        image = enemyImage(imageRight + animate(cycle, steps, 4));
        if (solidWall(x + speed, y) || x + speed > world().camera.x + 640) {
            direction = kFacingLeft;
        }
    }
    hitboxFromImage();
}

//-----------------------------------------------------------------------------
// The grunt
//-----------------------------------------------------------------------------

Grunt::Grunt(World& world) : Trooper(world) {
    shotDelay_ = 50 + world.random() % 200;
    hitPoints = 200;
    image = enemyImage(img::kGruntWalkRight);
    dy = 0;
}

void Grunt::walkAbout() {
    if (startFalling()) {
        return;
    }
    const int camera = world().camera.x;
    shotWait_ += 1;
    if (shotWait_ > shotDelay_ &&
        ((direction == kFacingRight && x > camera + 40) || (direction == kFacingLeft && x < camera + 600))) {
        shotWait_ = 0;
        shotDelay_ = 50 + world().random() % 200;
        frame = 0;
        subFrame = 0;
        mode = kShooting;
        shoot();
        return;
    }
    stepAndTurn(2, img::kGruntWalkRight, img::kGruntWalkLeft, kWalkCycle, 8);
}

void Grunt::die() {
    World& w = world();
    fall();

    subFrame += 1;
    subFrame %= 5;
    if (subFrame == 0) {
        frame += 1;
    }

    if (frame == 4 && subFrame == 1) {
        auto* head = new GruntHead(w);
        head->direction = direction;
        head->y = y - 22;
        head->x = x;
        w.blood.add(head);
        dy = 0;
    }

    if (frame == 14) {
        if (platformAt(x, y) != 0) {
            w.media.effects->engrave(x, y, image);
        }
        dead = true;
    } else if (direction == kFacingLeft) {
        walk(-2);
        image = enemyImage(img::kGruntDieLeft + frame);
    } else {
        walk(2);
        image = enemyImage(img::kGruntDieRight + frame);
    }
}

void Grunt::shoot() {
    World& w = world();

    subFrame += 1;
    subFrame %= 6;
    if (subFrame == 0) {
        frame += 1;
    }

    if (frame == 12) {
        frame = 0;
        subFrame = 0;
        mode = kWalking;
        walkAbout();
        return;
    }

    if (frame == 10 && subFrame == 0) {
        const bool right = direction == kFacingRight;
        auto* shot = new GruntShot(w, right ? 1 : -1);
        shot->direction = direction;
        shot->x = right ? x + 35 : x - 35;
        shot->y = y - 42;
        w.enemyShots.add(shot);
        w.media.levelSounds->play(snd::kGruntShoot);
    }

    if (frame < 11) {
        image =
            enemyImage((direction == kFacingRight ? img::kGruntShootRight : img::kGruntShootLeft) + frame);
    }
}

void Grunt::burnUp() {
    burn(img::kGruntBurnLeft, img::kGruntBurnRight, 9);
}

void Grunt::hitBy(Shot& shot) {
    Enemy::hitBy(shot);
    sprayBlood(shot, kBloodX, kBloodYLow);
    if (mode == kDying) {
        world().media.levelSounds->play(snd::kGruntDeath + world().random() % 3);  // one of 3
    }
}

//-----------------------------------------------------------------------------
// The gunner
//-----------------------------------------------------------------------------

Gunner::Gunner(World& world) : Trooper(world) {
    shotDelay_ = 50 + world.random() % 200;
    hitPoints = 150;
    image = enemyImage(img::kGruntWalkRight);
    dy = 0;
}

void Gunner::walkAbout() {
    if (startFalling()) {
        return;
    }
    World& w = world();
    const int camera = w.camera.x;
    shotWait_ += 1;
    if (w.target != nullptr && shotWait_ > shotDelay_ &&
        ((direction == kFacingRight && x > camera + 80) || (direction == kFacingLeft && x < camera + 560))) {
        shotWait_ = 0;
        shotDelay_ = 50 + w.random() % 200;
        frame = 0;
        subFrame = 0;
        mode = kShooting;
        aimX_ = w.target->x;
        aimY_ = w.target->y;
        shoot();
        return;
    }
    stepAndTurn(2, img::kGunnerWalkRight, img::kGunnerWalkLeft, kWalkCycle, 8);
}

void Gunner::die() {
    World& w = world();
    fall();

    subFrame += 1;
    subFrame %= 5;
    if (subFrame == 0) {
        frame += 1;
    }

    const int side = direction == kFacingRight ? 1 : -1;
    if (frame == 5 && subFrame == 0) {
        dy = 0;
        y -= 17;
        x += 10 * side;
    }
    if (frame >= 5) {
        fall();
        x += side;
    }
    if (frame == 10) {
        if (platformAt(x, y) != 0) {
            w.media.effects->engrave(x, y, image);
        }
        dead = true;
    }
    image = enemyImage((direction == kFacingLeft ? img::kGunnerDieLeft : img::kGunnerDieRight) + frame);
}

void Gunner::shoot() {
    World& w = world();

    subFrame += 1;
    subFrame %= 6;
    if (subFrame == 0) {
        frame += 1;
    }

    if (frame == 12) {
        frame = 0;
        subFrame = 0;
        mode = kWalking;
        walkAbout();
        return;
    }

    if (frame == 10 && subFrame == 0) {
        // Aimed at where the target was, at most 3 pixels per tick down or up.
        const auto aim = [this]() {
            int vy = ((aimY_ - y + 50) * 6) / (aimX_ - x);
            if (vy > 3) {
                vy = 3;
            }
            if (vy < -3) {
                vy = -3;
            }
            return vy;
        };
        GunnerShot* shot = nullptr;
        if (direction == kFacingRight) {
            shot = aimX_ > x ? new GunnerShot(w, 6, aim()) : new GunnerShot(w, 6, 1);
            shot->x = x + 35;
            image = enemyImage(img::kGunnerShootRight);
        } else {
            shot = aimX_ < x ? new GunnerShot(w, -6, -aim()) : new GunnerShot(w, -6, 1);
            shot->x = x - 35;
            image = enemyImage(img::kGunnerShootLeft);
        }
        shot->direction = direction;
        shot->y = y - 42;
        w.enemyShots.add(shot);
    }

    if (frame < 10) {
        image =
            enemyImage((direction == kFacingRight ? img::kGunnerShootRight : img::kGunnerShootLeft) + frame);
    }
    hitboxFromImage();
}

void Gunner::burnUp() {
    burn(img::kGunnerBurnLeft, img::kGunnerBurnRight, 9);
}

void Gunner::hitBy(Shot& shot) {
    Enemy::hitBy(shot);
    sprayBlood(shot, kBloodX, kBloodYHigh);
    if (mode == kDying) {
        world().media.levelSounds->play(snd::kGunnerDeath);
    }
}

//-----------------------------------------------------------------------------
// The governor
//-----------------------------------------------------------------------------

Governor::Governor(World& world) : Trooper(world) {
    shotDelay_ = 50 + world.random() % 200;
    hitPoints = 150;
    image = enemyImage(img::kGruntWalkRight);
    dy = 0;
}

void Governor::walkAbout() {
    static const int kCycle[] = {0, 1, 2, 3, 2, 1};
    if (startFalling()) {
        return;
    }
    const int camera = world().camera.x;
    shotWait_ += 1;
    if (shotWait_ > shotDelay_ &&
        ((direction == kFacingRight && x > camera + 40) || (direction == kFacingLeft && x < camera + 600))) {
        shotWait_ = 0;
        shotDelay_ = 50 + world().random() % 200;
        frame = 0;
        subFrame = 0;
        shots_ = 0;
        mode = kShooting;
        shoot();
        return;
    }
    stepAndTurn(1, img::kGovernorWalkRight, img::kGovernorWalkLeft, kCycle, 6);
}

void Governor::die() {
    World& w = world();
    fall();

    subFrame += 1;
    subFrame %= 5;
    if (subFrame == 0) {
        frame += 1;
    }
    if (frame == 5) {
        if (platformAt(x, y) != 0) {
            w.media.effects->engrave(x, y, image);
        }
        dead = true;
    }
    image = enemyImage((direction == kFacingLeft ? img::kGovernorDieLeft : img::kGovernorDieRight) + frame);
}

void Governor::shoot() {
    World& w = world();

    subFrame += 1;
    subFrame %= 6;
    if (subFrame == 0) {
        frame += 1;
    }

    if (frame == 10) {
        frame = 0;
        subFrame = 0;
        mode = kWalking;
        walkAbout();
        return;
    }

    if (frame == 6 && subFrame == 0) {
        if (w.target != nullptr) {
            const int speedX = w.random() % 6 - 2;
            const int speedY = -5 - w.random() % 5;
            auto* shot = new GovernorShot(w, w.target, speedX, speedY);
            shot->direction = direction;
            shot->x = x;
            shot->y = y - 71;
            w.enemyShots.add(shot);
            shots_ += 1;
        }
        if (shots_ == 0) {
            w.media.levelSounds->play(snd::kGovernorShoot);
        }
    }

    // Five shots: back to the start of the throw after each one.
    if (frame == 7 && shots_ < 5 && subFrame == 0) {
        frame -= 4;
    }

    if (frame < 7) {
        image = enemyImage((direction == kFacingRight ? img::kGovernorShootRight : img::kGovernorShootLeft) +
                           frame);
    }
}

void Governor::burnUp() {
    burn(img::kGovernorBurnLeft, img::kGovernorBurnRight, 8);
}

void Governor::hitBy(Shot& shot) {
    Enemy::hitBy(shot);
    sprayBlood(shot, kBloodX, kBloodYHigh);
    if (mode == kDying) {
        world().media.levelSounds->play(snd::kGovernorDeath + world().random() % 2);  // one of 2
    }
}

//-----------------------------------------------------------------------------
// The Snork in Black
//-----------------------------------------------------------------------------

SnorkInBlack::SnorkInBlack(World& world) : Trooper(world) {
    shotDelay_ = 50 + world.random() % 100;
    hitPoints = 250;
    dy = 0;
}

void SnorkInBlack::walkAbout() {
    static const int kCycle[] = {0, 1, 2, 1};
    if (startFalling()) {
        return;
    }
    World& w = world();
    const int camera = w.camera.x;
    shotWait_ += 1;
    if (w.target != nullptr && shotWait_ > shotDelay_ &&
        ((direction == kFacingRight && x > camera + 80) || (direction == kFacingLeft && x < camera + 560))) {
        aimX_ = w.target->x;
        aimY_ = w.target->y;
        if (aimX_ != x) {
            // Only when nearly level with the target.
            const int vy = ((aimY_ - y + 14) * 6) / (aimX_ - x);
            if ((direction == kFacingRight && x < aimX_ && vy < 2 && vy > -2) ||
                (direction == kFacingLeft && x > aimX_ && vy < 2 && vy > -2)) {
                shotWait_ = 0;
                shotDelay_ = 50 + w.random() % 100;
                frame = 0;
                subFrame = 0;
                shots_ = w.random() % 3;
                mode = kShooting;
                shoot();
                return;
            }
        }
    }
    stepAndTurn(2, img::kBlackWalkRight, img::kBlackWalkLeft, kCycle, 4);
}

void SnorkInBlack::die() {
    World& w = world();
    fall();

    subFrame += 1;
    subFrame %= 8;
    if (subFrame == 0) {
        frame += 1;
    }
    if (frame == 5) {
        if (platformAt(x, y) != 0) {
            w.media.effects->engrave(x, y, image);
        }
        dead = true;
    } else {
        image = enemyImage((direction == kFacingRight ? img::kBlackDieRight : img::kBlackDieLeft) + frame);
    }
}

void SnorkInBlack::shoot() {
    static const int kMaxShots = 6;
    World& w = world();

    subFrame += 1;
    subFrame %= frame == 0 ? 20 : 4;
    if (subFrame == 0) {
        frame += 1;
    }

    if (frame == 1 && subFrame == 0) {
        Spine* spine = nullptr;
        if (direction == kFacingRight) {
            const int vx = 5 + w.random() % 2;
            const int vy = w.random() % 5 - 2;
            spine = new Spine(w, img::kBlackSpineRight, vx, vy);
            spine->x = x + 49;
        } else {
            const int vx = -5 - w.random() % 2;
            const int vy = w.random() % 5 - 2;
            spine = new Spine(w, img::kBlackSpineLeft, vx, vy);
            spine->x = x - 49;
        }
        spine->direction = direction;
        spine->y = y - 15;
        w.enemyShots.add(spine);
        w.media.levelSounds->play(snd::kBlackShoot);
    }

    if (frame == 3) {
        shots_ += 1;
        frame = 0;
        if (shots_ >= kMaxShots) {
            subFrame = 0;
            frame = 0;
            mode = kWalking;
            walkAbout();
            return;
        }
    }

    if (direction == kFacingRight) {
        image = enemyImage(frame == 0 ? img::kBlackWalkRight : img::kBlackShootRight + frame - 1);
    } else {
        image = enemyImage(frame == 0 ? img::kBlackWalkLeft : img::kBlackShootLeft + frame - 1);
    }
    hitboxFromImage();
}

void SnorkInBlack::burnUp() {
    burn(img::kBlackBurnLeft, img::kBlackBurnRight, 7);
}

void SnorkInBlack::hitBy(Shot& shot) {
    Enemy::hitBy(shot);
    sprayBlood(shot, kBloodX, kBloodYLow);
    if (mode == kDying) {
        world().media.levelSounds->play(snd::kBlackDeath + world().random() % 3);  // one of 3
    }
}

//-----------------------------------------------------------------------------
// The mage
//-----------------------------------------------------------------------------

namespace {

const int kBob[] = {0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 2, 2, 2, 1, 1, 1, 0, 0, 0};

}  // namespace

Mage::Mage(World& world) : Enemy(world) {
    shotDelay_ = 50 + world.random() % 200;
    hitPoints = 200;
    image = enemyImage(img::kGruntWalkRight);
    dy = 0;
}

void Mage::update() {
    switch (mode) {
        case kWalking:
        case kNormal:
            floatAbout();
            break;
        case kFalling:
        case kJumping:
            fallDown();
            break;
        case kDying:
            die();
            break;
        case kCarbonised:
            burn(img::kMageBurn, img::kMageBurn, 7);
            break;
        case kShooting:
            cast();
            break;
        default:
            break;
    }
    dieWhenOffscreen();
}

void Mage::floatAbout() {
    World& w = world();
    const int camera = w.camera.x;

    // Bobs up and down over 43 ticks.
    bobStep_ += 1;
    bobStep_ %= 43;
    if (bobStep_ >= 22) {
        y -= kBob[bobStep_ - 22];
    } else {
        y += kBob[bobStep_];
    }

    shotWait_ += 1;
    if (w.target != nullptr && shotWait_ > shotDelay_ &&
        ((direction == kFacingRight && x > camera + 30) || (direction == kFacingLeft && x < camera + 610))) {
        // Only when the target is on its diagonal, below it.
        const int below = w.target->y - y;
        const int ahead = w.target->x - x;
        if ((direction == kFacingRight && below > 0 && ahead > 0 && below - ahead >= -30 &&
             below - ahead <= 30) ||
            (direction == kFacingLeft && below > 0 && ahead < 0 && below + ahead >= -30 &&
             below + ahead <= 30)) {
            frame = 0;
            shotWait_ = 0;
            shotDelay_ = 50 + w.random() % 200;
            frame = 0;
            subFrame = 0;
            mode = kShooting;
            focus_ = 0;
            cast();
            return;
        }
    }

    subFrame += 1;
    subFrame %= 5;
    if (subFrame == 0) {
        frame += 1;
        frame %= 6;
    }
    image = enemyImage(img::kMageSwim + frame);

    if (direction == kFacingLeft) {
        x -= 1;
        if (solidWall(x - 1, y) || x - 1 < minX) {
            direction = kFacingRight;
        }
    } else {
        x += 1;
        if (solidWall(x + 1, y) || x + 1 > camera + 640) {
            direction = kFacingLeft;
        }
    }
    hitboxFromImage();
}

void Mage::fallDown() {
    fall();
    if (platformAt(x, y) != 0) {
        mode = kWalking;
    }
    hitboxFromImage();
}

void Mage::die() {
    World& w = world();

    if (frame > 2) {
        fall();
    }
    subFrame += 1;
    subFrame %= frame < 2 ? 17 : 5;
    if (subFrame == 0) {
        frame += 1;
    }
    if (subFrame == 0 && frame == 3) {
        w.media.levelSounds->play(snd::kMageFall);
    }
    // Falls (images 4 to 6 in a loop) until it lands (image 8 on).
    if (platformAt(x, y) != 0 && frame < 7) {
        frame = 8;
    }
    if (frame == 7) {
        frame = 4;
    }
    if (frame == 11) {
        if (platformAt(x, y) != 0) {
            w.media.effects->engrave(x, y, image);
        }
        dead = true;
    } else {
        image = enemyImage(img::kMageDie + frame);
    }
}

void Mage::cast() {
    World& w = world();

    if (focus_ < 3) {
        // Concentrates three times.
        subFrame += 1;
        subFrame %= 4;
        if (subFrame == 0) {
            frame += 1;
        }
        if (frame == 6) {
            focus_ += 1;
            frame = 0;
            if (focus_ == 3) {
                frame = 0;
                subFrame = 0;
            }
        }
        image = enemyImage(img::kMageConcentrate + frame);
        return;
    }

    subFrame += 1;
    subFrame %= 4;
    if (subFrame == 0) {
        frame += 1;
    }
    if (frame == 2 && subFrame == 0) {
        auto* shot = new MageShot(w);
        shot->direction = direction;
        shot->x = direction == kFacingRight ? x + 35 : x - 35;
        shot->y = y - 42;
        w.enemyShots.add(shot);
        w.media.levelSounds->play(snd::kMageCast);
    }
    if (frame == 5) {
        frame = 0;
        subFrame = 0;
        mode = kWalking;
        floatAbout();
        return;
    }
    if (frame < 3) {
        image = enemyImage((direction == kFacingRight ? img::kMageCastRight : img::kMageCastLeft) + frame);
    }
}

void Mage::hitBy(Shot& shot) {
    static const int kMageBloodY[] = {-35, -45, -45, -45, -55, -45, -45, -45};
    Enemy::hitBy(shot);
    sprayBlood(shot, kBloodX, kMageBloodY);
    if (mode == kDying) {
        world().media.levelSounds->play(snd::kMageDeath);
    }
}

//-----------------------------------------------------------------------------
// The seahorse rider
//-----------------------------------------------------------------------------

SeahorseRider::SeahorseRider(World& world) : Enemy(world) {
    chargeDelay_ = 50 + world.random() % 200;
    hitPoints = 350;
    dy = 0;
}

void SeahorseRider::update() {
    switch (mode) {
        case kFalling:
        case kJumping:
        case kWalking:
        case kNormal:
            swim();
            break;
        case kDying:
            die();
            break;
        case kCarbonised:
            burnAway();
            break;
        case kShooting:
            charge();
            break;
        default:
            break;
    }
    dieWhenOffscreen();
}

void SeahorseRider::swim() {
    World& w = world();
    const int camera = w.camera.x;

    chargeWait_ += 1;
    if (chargeWait_ > chargeDelay_ &&
        ((direction == kFacingRight && x > camera + 80) || (direction == kFacingLeft && x < camera + 560))) {
        chargeWait_ = 0;
        chargeDelay_ = 50 + w.random() % 200;
        frame = 0;
        subFrame = 0;
        mode = kShooting;
        attackStep_ = 0;
        charge();
        return;
    }

    subFrame += 1;
    subFrame %= 6;
    if (subFrame == 0) {
        frame += 1;
        frame %= 4;
    }

    if (direction == kFacingLeft) {
        x -= 1;
        image = enemyImage(img::kRiderSwimLeft);
        if (solidWall(x - 1, y) || x - 1 < minX) {
            direction = kFacingRight;
        }
    } else {
        x += 1;
        image = enemyImage(img::kRiderSwimRight);
        if (solidWall(x + 1, y) || x + 1 > camera + 640) {
            direction = kFacingLeft;
        }
    }
    hitboxFromImage();
}

void SeahorseRider::die() {
    World& w = world();

    subFrame += 1;
    subFrame %= frame < 7 ? 3 : 5;
    if (subFrame == 0) {
        frame += 1;
    }

    if (frame == 7 && subFrame == 0) {
        auto* body = new RiderBody(w, direction == kFacingRight ? -1 : 1);
        body->direction = direction;
        body->y = y;
        body->x = x;
        w.blood.add(body);
        w.media.levelSounds->play(snd::kRiderBody);
        dy = 0;
    }

    if (frame >= 7) {
        // The seahorse swims off alone.
        fin_ = false;
        if (direction == kFacingLeft) {
            x -= 4;
            image = enemyImage(img::kRiderSeahorseLeft);
        } else {
            x += 4;
            image = enemyImage(img::kRiderSeahorseRight);
        }
    } else if (direction == kFacingLeft) {
        x -= 1;
        image = enemyImage(img::kRiderDieLeft + frame);
    } else {
        x += 1;
        image = enemyImage(img::kRiderDieRight + frame);
    }
}

void SeahorseRider::charge() {
    static const int kChargeSpeed = 8;
    World& w = world();
    const int camera = w.camera.x;

    subFrame += 1;
    subFrame %= 2;
    if (subFrame == 0) {
        frame += 1;
        frame %= 4;
    }

    attackStep_ += 1;
    if (attackStep_ > 40) {
        fin_ = false;
        // (2002 checks the next swimming step, not the charge step, on the
        // right.)
        if (direction == kFacingRight) {
            if (solidWall(x + 1, y) || x + 1 > camera + 640) {
                frame = 0;
                subFrame = 0;
                mode = kWalking;
                fin_ = true;
                swim();
                return;
            }
            x += kChargeSpeed;
            image = enemyImage(img::kRiderChargeRight);
        } else {
            if (solidWall(x - kChargeSpeed, y) || x - kChargeSpeed < minX) {
                frame = 0;
                subFrame = 0;
                mode = kWalking;
                fin_ = true;
                swim();
                return;
            }
            x -= kChargeSpeed;
            image = enemyImage(img::kRiderChargeLeft);
        }
    } else {
        image =
            enemyImage(direction == kFacingRight ? img::kRiderChargeWaitRight : img::kRiderChargeWaitLeft);
    }
    hitboxFromImage();
}

void SeahorseRider::burnAway() {
    fin_ = false;

    subFrame += 1;
    subFrame %= 4;
    if (subFrame == 0) {
        frame += 1;
    }
    if (direction == kFacingRight) {
        x += 6;
        image = enemyImage(img::kRiderSeahorseRight);
    } else {
        x -= 6;
        image = enemyImage(img::kRiderSeahorseLeft);
    }
    if (x - 6 < minX || x + 6 > world().camera.x + 640) {
        dead = true;
    }
}

void SeahorseRider::draw() {
    Actor::draw();
    if (mode == kCarbonised && frame < 9) {
        drawAt(x, y,
               enemyImage((direction == kFacingRight ? img::kRiderBurnRight : img::kRiderBurnLeft) + frame));
    }
    if (fin_) {
        if (direction == kFacingRight) {
            drawAt(x + 15, y + 20, enemyImage(img::kRiderFinRight + frame % 4));
        } else {
            drawAt(x - 15, y + 20, enemyImage(img::kRiderFinLeft + frame % 4));
        }
    }
}

void SeahorseRider::hitBy(Shot& shot) {
    static const int kRiderBloodX[] = {0, 0, 7, 3, 0, -3, -7, 0};
    static const int kRiderBloodY[] = {-15, -15, -15, -25, -25, -25, -15, -15};
    Enemy::hitBy(shot);
    sprayBlood(shot, kRiderBloodX, kRiderBloodY);
    if (mode == kDying) {
        world().media.levelSounds->play(snd::kRiderDeath);
    }
}

}  // namespace bb::game::snorks
