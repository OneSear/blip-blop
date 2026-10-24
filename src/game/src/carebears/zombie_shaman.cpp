// The zombie bear (ennemi_bisou_zombi.cpp) and the shaman
// (ennemi_bisou_chaman.cpp).

#include <algorithm>

#include "bb/assets/music/bisou.hpp"
#include "bb/game/trig.hpp"
#include "carebears/carebears_world.hpp"

namespace bb::game::carebears {

namespace {

const int kBloodX[] = {0, 7, 10, 7, 0, -7, -10, -7};
const int kBloodY[] = {-15, -25, -25, -25, -35, -25, -25, -25};
const int kBloodYCrawling[] = {0, -10, -10, -10, -20, -10, -10, -10};

constexpr Flag kShamanState = Flag::User0;  // 1 fight starts, 2 fighting, 7 dead
constexpr Flag kShamanHere = Flag::User2;   // set by the shaman every tick

// The zombies' moans: one counter for all of them, advanced once per tick
// (on the blink phase) until the shaman appears. 2002 file statics, kept
// in World::shared(): they only choose when a moan sound plays, which is
// not fingerprinted; reset at each level start (D-50).
struct Moans {
    int step = 199;
    int sound = 0;
    bool phase = false;
};

}  // namespace

//-----------------------------------------------------------------------------
// The zombie bear
//-----------------------------------------------------------------------------

void ZombieBear::update() {
    World& w = world();

    if (bleeding > 0) {
        bleeding -= 1;
    }

    // The shaman is dead: every zombie burns.
    if (w.flags[kShamanState] == 7 && mode != kCarbonised && mode != kTopBurning) {
        frame = subFrame = 0;
        mode = mode == kCut ? kTopBurning : kCarbonised;
    }

    Moans& moans = w.shared<Moans>();
    if (moans.phase != w.blink && w.flags[kShamanHere] == 0) {
        moans.step += 1;
        moans.step %= 200;
        if (moans.step == 0) {
            moans.sound += 1;
            moans.sound %= 3;
            w.media.levelSounds->play(snd::kZombieMoan + moans.sound);
        }
        moans.phase = w.blink;
    }

    switch (mode) {
        case kNormal:
        case kWalking:
            walkAbout();
            break;
        case kCut:
            cut();
            break;
        case kFalling:
            topFalls();
            break;
        case kCrawling:
            crawl();
            break;
        case kDying:
            die();
            break;
        case kCarbonised:
            burnUp(img::kZombieBurnRight, img::kZombieBurnLeft, 6);
            break;
        case kTopBurning:
            burnUp(img::kZombieTopBurnRight, img::kZombieTopBurnLeft, 7);
            break;
        default:
            break;
    }
    dieWhenOffscreen();
}

void ZombieBear::walkAbout() {
    static const int kSpeed = 1;

    fall();

    subFrame += 1;
    subFrame %= 15;
    if (subFrame == 0) {
        frame += 1;
        frame %= 8;
    }

    // Walks every other tick.
    walkPhase_ = !walkPhase_;
    if (walkPhase_) {
        if (x - kSpeed < minX || solidWall(x - kSpeed, y)) {
            direction = kFacingRight;
        } else if (x + kSpeed > world().camera.x + 640 || solidWall(x + kSpeed, y)) {
            direction = kFacingLeft;
        }
        if (direction == kFacingRight) {
            walk(kSpeed);
            image = enemyImage(img::kZombieWalkRight + frame);
        } else {
            walk(-kSpeed);
            image = enemyImage(img::kZombieWalkLeft + frame);
        }
    }
    hitboxFromImage();
}

void ZombieBear::hitBy(Shot& shot) {
    World& w = world();
    const int wound = shot.damage();

    if (wound < hitPoints) {
        shot.onHit(wound);
        hitPoints -= wound;
        // Below 500 HP a non-burning shot cuts it in two.
        if (hitPoints < 500 && (mode == kNormal || mode == kWalking) && shot.burning() == 0) {
            frame = subFrame = 0;
            collides = false;
            mode = kCut;
        }
    } else {
        shot.onHit(hitPoints);
        shot.onKill();
        hitPoints = 0;
        collides = false;
        w.flags[Flag::Kills] += 1;  // its own pipeline: no wall blood, no pickup

        frame = subFrame = 0;
        if (mode == kCrawling) {
            mode = shot.burning() == 2 ? kTopBurning : kDying;
        } else if (shot.burning() != 0) {
            mode = kCarbonised;
        } else if (mode != kCut) {
            mode = kCut;
        }
    }

    sprayBlood(shot, kBloodX, mode == kCrawling ? kBloodYCrawling : kBloodY);
}

void ZombieBear::topFalls() {
    subFrame += 1;
    subFrame %= 6;
    if (subFrame == 0 && frame < 3) {
        frame += 1;
    }

    if (direction == kFacingRight) {
        image = enemyImage(img::kZombieFallRight + frame);
        x += 1;
    } else {
        image = enemyImage(img::kZombieFallLeft + frame);
        x -= 1;
    }

    fall();
    if (platformAt(x, y) != 0 && frame >= 3) {
        frame = subFrame = 0;
        mode = kCrawling;
        collides = true;
        crawl();
    }
}

void ZombieBear::die() {
    fall();

    subFrame += 1;
    subFrame %= 6;
    if (subFrame == 0 && frame < 3) {
        frame += 1;
    }

    image = enemyImage((direction == kFacingRight ? img::kZombieDieRight : img::kZombieDieLeft) + frame);

    if (frame >= 3) {
        world().media.effects->engrave(x, y, image);
        dead = true;
    }
}

void ZombieBear::cut() {
    fall();

    subFrame += 1;
    subFrame %= 5;
    if (subFrame == 0) {
        frame += 1;
    }

    if (frame == 4) {
        auto* legs = new WalkingLegs(world(), img::kZombieLegsRight, img::kZombieLegsLeft);
        legs->x = x;
        legs->direction = direction;
        legs->y = y;
        world().gore.add(legs);
        y -= 19;
        dy = 0;
        frame = 0;
        subFrame = 0;
        mode = kFalling;
        topFalls();
    } else {
        image = enemyImage((direction == kFacingRight ? img::kZombieCutRight : img::kZombieCutLeft) + frame);
    }
}

void ZombieBear::crawl() {
    static const int kCrawl[] = {0, 1, 2, 1};
    static const int kSpeed = 1;

    if (hitPoints <= 0) {
        frame = subFrame = 0;
        mode = kDying;
        collides = false;
        die();
        return;
    }

    fall();

    // Crawls every fourth tick.
    crawlStep_ = (crawlStep_ + 1) % 4;
    if (crawlStep_ == 0) {
        if (x - kSpeed < minX || solidWall(x - kSpeed, y)) {
            direction = kFacingRight;
        } else if (x + kSpeed > world().camera.x + 640 || solidWall(x + kSpeed, y)) {
            direction = kFacingLeft;
        }
        walk(direction == kFacingRight ? kSpeed : -kSpeed);
        hitboxFromImage();
    }

    image = enemyImage((direction == kFacingRight ? img::kZombieCrawlRight : img::kZombieCrawlLeft) +
                       animate(kCrawl, 4, 16));
}

void ZombieBear::burnUp(int firstRight, int firstLeft, int lastStep) {
    subFrame += 1;
    subFrame %= 4;
    if (subFrame == 0) {
        frame += 1;
        if (frame > lastStep) {
            dead = true;
            return;
        }
    }
    image = enemyImage((direction == kFacingRight ? firstRight : firstLeft) + frame);
}

//-----------------------------------------------------------------------------
// The shaman
//-----------------------------------------------------------------------------

Shaman::Shaman(World& world) : Enemy(world) {
    hitPoints = 11000;
    shotDelay_ = 250 + world.random() % 50;
    treasure = 0;
}

void Shaman::update() {
    World& w = world();

    w.flags[kShamanHere] = 1;
    if (w.flags[kShamanState] == 1) {
        w.media.levelMusic->play(bb::assets::music::bisou::kBoss);
        w.flags[kShamanState] = 2;
        w.flags[Flag::Timer] = 50;
    }

    if (bleeding > 0) {
        bleeding -= 1;
    }
    shotWait_ += 1;

    switch (mode) {
        case kNormal:
        case kWalking:
            stand();
            break;
        case kRaisingStaff:
            raiseStaff();
            break;
        case kTeleporting:
            teleport();
            break;
        case kLoweringStaff:
            lowerStaff();
            break;
        case kShooting:
            cast();
            break;
        case kDying:
        case kCarbonised:
            burnUp();
            break;
        default:
            break;
    }
    dieWhenOffscreen();
}

void Shaman::stand() {
    World& w = world();
    fall();

    subFrame += 1;
    subFrame %= 6;
    if (subFrame == 0) {
        frame += 1;
        frame %= 8;
    }

    // Faces the middle of the screen.
    if (x < w.camera.x + 320) {
        image = enemyImage(img::kShamanTeleportRight);
        direction = kFacingRight;
    } else {
        image = enemyImage(img::kShamanTeleportLeft);
        direction = kFacingLeft;
    }

    if (shotWait_ > shotDelay_ && frame == 0) {
        mode = kRaisingStaff;
        subFrame = 0;
        raiseStaff();
    }

    if (hits_ >= 10) {
        frame = subFrame = 0;
        mode = kTeleporting;
        w.media.levelSounds->play(snd::kShamanTeleport);
    }
    hitboxFromImage();
}

void Shaman::teleport() {
    World& w = world();
    const int camera = w.camera.x;

    subFrame += 1;
    subFrame %= 4;
    if (subFrame == 0) {
        frame += 1;
        if (frame == 8) {
            // Away from the target player: left spot when the target is on
            // the right half, else the high spot unless the target is high.
            int spot = 0;
            if (w.target->x > camera + 320) {
                spot = 2;
            } else if (w.target->y < 280) {
                spot = 0;
            } else {
                spot = 1;
            }
            if (spot != spot_) {
                spot_ = spot;
            } else {
                spot_ += w.random() % 2 + 1;
                spot_ %= 3;
            }
            // His three spots, (x, y) in the level's marker: high right,
            // right, left.
            x = w.marker("carebears.shamanSpots", 2 * static_cast<std::size_t>(spot_));
            y = w.marker("carebears.shamanSpots", 2 * static_cast<std::size_t>(spot_) + 1);
        } else if (frame == 19) {
            mode = kNormal;
            hits_ = 0;
            return;
        }
    }

    // Vanishes (8 images), blinks (3 steps), comes back (8 images).
    const bool onLeft = x < camera + 320;
    direction = onLeft ? kFacingRight : kFacingLeft;
    const int first = onLeft ? img::kShamanTeleportRight : img::kShamanTeleportLeft;
    if (frame <= 7) {
        image = enemyImage(first + frame);
    } else if (frame <= 10) {
        image = subFrame % 2 == 0 ? enemyImage(first + 7) : nullptr;
    } else {
        image = enemyImage(first + 7 - (frame - 11));
    }
    noHitbox();
}

void Shaman::raiseStaff() {
    subFrame += 1;
    subFrame %= 5;
    if (subFrame == 0) {
        frame += 1;
        if (frame > 2) {
            frame = subFrame = 0;
            mode = kShooting;
            glows_ = 0;
            cast();
            return;
        }
    }
    image = enemyImage((direction == kFacingRight ? img::kShamanStaffRight : img::kShamanStaffLeft) + frame);
    hitboxFromImage();
}

void Shaman::lowerStaff() {
    subFrame += 1;
    subFrame %= 5;
    if (subFrame == 0) {
        frame += 1;
        if (frame > 2) {
            frame = subFrame = 0;
            mode = kWalking;
            shotDelay_ = 100 + world().random() % 50;
            shotWait_ = 0;
            return;
        }
    }
    image =
        enemyImage((direction == kFacingRight ? img::kShamanStaffRight : img::kShamanStaffLeft) + 2 - frame);
    hitboxFromImage();
}

void Shaman::cast() {
    World& w = world();

    subFrame += 1;
    subFrame %= 4;
    if (subFrame == 0) {
        frame += 1;
        frame %= 11;
        if (frame == 0) {
            glows_ += 1;
            if (glows_ >= 3) {
                // A fireball when the target is more than 45 degrees off
                // the vertical, else a column along the ground.
                bool fireball = false;
                int dx = 0;
                int dyTarget = 0;
                if (w.target != nullptr) {
                    dx = w.target->x - x;
                    dyTarget = w.target->y - (y - 72);
                    // At least 1 (D-43: 2002 divided by zero at distance 0).
                    const int distance = std::max(1, squareRoot(dx * dx + dyTarget * dyTarget));
                    dx = (dx * 6) / distance;
                    dyTarget = (dyTarget * 6) / distance;
                    const int slope = dyTarget == 0 ? 10 : dx / dyTarget;
                    if (slope < -1 || slope > 1) {
                        fireball = true;
                    }
                }
                const int startX = direction == kFacingRight ? x + 18 : x - 18;
                if (fireball) {
                    auto* ball = new ShamanBall(w);
                    ball->y = y - 72;
                    ball->x = startX;
                    ball->xSpeed = (dx * 4) / 3;
                    ball->dy = (dyTarget * 4) / 3;
                    w.enemyShots.add(ball);
                } else {
                    auto* column = new ShamanColumn(w);
                    column->y = y - 72;
                    column->x = startX;
                    column->direction = direction;
                    w.enemyShots.add(column);
                }
                frame = subFrame = 0;
                mode = kLoweringStaff;
            }
        }
    }
    hitboxFromImage();
}

void Shaman::draw() {
    Actor::draw();
    if (mode == kShooting) {
        if (direction == kFacingRight) {
            drawAt(x + 18, y - 70, enemyImage(img::kShamanGlowRight + frame));
        } else {
            drawAt(x - 18, y - 70, enemyImage(img::kShamanGlowLeft + frame));
        }
    }
}

void Shaman::burnUp() {
    subFrame += 1;
    subFrame %= 4;
    if (subFrame == 0) {
        frame += 1;
        if (frame > 7) {
            dead = true;
            return;
        }
    }
    image = enemyImage((direction == kFacingRight ? img::kShamanBurnRight : img::kShamanBurnLeft) + frame);
}

void Shaman::hitBy(Shot& shot) {
    static const int kShamanBloodX[] = {0, 7, 10, 7, 0, -7, -10, -7};

    World& w = world();
    Enemy::hitBy(shot);
    sprayBlood(shot, kShamanBloodX, kBloodY);

    hits_ += 1;

    if (mode == kDying || mode == kCarbonised) {
        w.flags[kShamanState] = 7;
        w.flags[Flag::GeneratorsOff] = 1;
        w.media.levelSounds->play(snd::kShamanDeath);
    }
}

}  // namespace bb::game::carebears
