// The scripted props of dork: lifts (fond_poke_ascenceur.cpp,
// fond_poke_ascenseur_2.cpp), the switch (fond_poke_interupteur.cpp), the
// crate and its platforms (fond_poke_caisse*.cpp) and the bomb
// (fond_bombe.cpp).

#include "bb/game/trig.hpp"
#include "pokemon/pokemon_world.hpp"

namespace bb::game::pokemon {

namespace {

// The level registers the props use.
constexpr Flag kBombsDefused = Flag::User0;
constexpr Flag kSwitchesOn = Flag::User2;

}  // namespace

//-----------------------------------------------------------------------------
// Lifts
//-----------------------------------------------------------------------------

Lift::Lift(World& world, int switchesNeeded) : MovingPlatform(world), switchesNeeded_(switchesNeeded) {
    image = levelImage(prop::kLift);
    dy = 1;
}

void Lift::update() {
    World& w = world();

    if (w.flags[kSwitchesOn] <= switchesNeeded_) {
        if (switchesNeeded_ > 0) {
            y = -60;  // lift 2 waits above the screen
        }
        return;
    }

    // Up and down between 96 and 387.
    if (y <= 96) {
        dy = 1;
    } else if (y >= 387) {
        dy = -1;
    }

    const int width = image->width();
    carryPlayers(width, y + 20, dy);

    y += dy;
    setPlatform(4, x, x + width, y + 20);
    hitboxFromImage();

    if (x < w.camera.x - 400) {
        dead = true;
    }
}

//-----------------------------------------------------------------------------
// Switch
//-----------------------------------------------------------------------------

void Switch::update() {
    static const int kOn[] = {0, 1, 2, 0};
    static const int kOff[] = {0, 1, 2, 1};

    World& w = world();

    animation.cycle(8, 4);

    if (on_) {
        image = levelImage(prop::kSwitchOn + kOn[animation.frame]);
    } else {
        Entity* target = w.target;
        if (target != nullptr && target->x > x - 10 && target->x < x + 33 && target->y > y - 10 &&
            target->y < y + 51) {
            w.flags[kSwitchesOn] += 1;
            on_ = true;
            w.media.levelSounds->play(snd::kSwitch);
        }
        image = levelImage(prop::kSwitchOff + kOff[animation.frame]);
    }

    if (x < w.camera.x - 100) {
        dead = true;
    }

    // Before the first bomb, the generators run only once the "use the
    // switch" hint was shown or while no switch is on (flag 7 = 1: off).
    if (w.flags[kBombsDefused] == 0) {
        w.flags[Flag::GeneratorsOff] = (waitShown_ || w.flags[kSwitchesOn] == 0) ? 1 : 0;
    }

    // The instructions (strings 104 to 107).
    if (!waitShown_ && w.flags[kSwitchesOn] == 0 && w.flags[Flag::Timer] <= 80) {
        waitShown_ = true;
        showBanner(w, 104);  // USE THE SWITCH ON THE RIGHT!
    }
    if (!liftShown_ && waitShown_ && w.flags[kSwitchesOn] == 1) {
        liftShown_ = true;
        showBanner(w, 105);  // GET ON THE LIFT AND
        showBanner(w, 106);  // DEFUSE THE BOMB!
    }
    if (!wellDoneShown_ && liftShown_ && w.flags[kBombsDefused] == 1) {
        wellDoneShown_ = true;
        showBanner(w, 107);  // WELL DONE!
    }
}

//-----------------------------------------------------------------------------
// The crate
//-----------------------------------------------------------------------------

namespace {

constexpr int kUnlockDelay = 32;  // UNLOCK_DELAY
constexpr int kCrateImage = prop::kCrate;

}  // namespace

LockedCrate::LockedCrate(World& world) : MovingPlatform(world) {
    image = levelImage(kCrateImage);
}

void LockedCrate::update() {
    World& w = world();

    if (w.flags[kSwitchesOn] <= 2) {
        y = -160;  // waits above the screen
        return;
    }

    // Comes down, then unlocks.
    if (y < 100 && animation.frame <= kUnlockDelay) {
        y += 2;
        return;
    }

    animation.tick(5);

    // Two platforms come down on the left (frame 0 is reached on the first
    // tick after the descent).
    if (animation.frame == 0 && animation.subFrame == 1) {
        auto* platform = new CratePlatform(w, 180);
        platform->x = x - 250;
        platform->y = -150;
        w.movingPlatforms.add(platform);
        platform = new CratePlatform(w, 180);
        platform->x = x - 470;
        platform->y = -150;
        w.movingPlatforms.add(platform);
    }

    if (animation.frame > kUnlockDelay + 4) {
        // Open: goes back up, drops its contents and a swinging platform.
        y -= 3;
        if (y < -160) {
            dead = true;
        }
        if (animation.frame == kUnlockDelay + 5 && animation.subFrame == 0) {
            image = levelImage(prop::kCrateEmpty);
            auto* contents = new FallingCrate(w);
            contents->x = x;
            contents->y = y + 150;
            w.props.add(contents);
            auto* swinging = new SwingingPlatform(w);
            swinging->x = x;
            swinging->y = -150;
            w.movingPlatforms.add(swinging);
        }
    }
}

void LockedCrate::draw() {
    static const int kLock[] = {0, 1, 2, 3, 4, 5, 4, 3, 2, 1};

    Actor::draw();
    if (animation.frame > kUnlockDelay + 4) {
        drawAt(x, y, levelImage(prop::kCrateOpenTop));
        drawAt(x, y + 63, levelImage(prop::kCrateOpenBottom));
    } else if (animation.frame > kUnlockDelay - 1) {
        drawAt(x, y + 63, levelImage(prop::kCrateUnlock + animation.frame - kUnlockDelay));
    } else if (animation.frame > 0) {
        drawAt(x, y, levelImage(prop::kCrateLock + kLock[animation.frame % 10]));
        drawAt(x, y + 63, levelImage(prop::kCrateCounter + animation.frame % 16));
    }
}

CratePlatform::CratePlatform(World& world, int targetY) : MovingPlatform(world), targetY_(targetY) {
    image = levelImage(kCrateImage);
}

void CratePlatform::update() {
    // Slows down as it gets close to its height.
    if (y < targetY_ - 50) {
        y += 5;
    } else if (y < targetY_ - 40) {
        y += 4;
    } else if (y < targetY_ - 30) {
        y += 3;
    } else if (y < targetY_ - 20) {
        y += 2;
    } else if (y < targetY_ - 10) {
        y += 1;
    }
    // A height of 0 means "no platform": 1 pixel lower then.
    setPlatform(4, x - 90, x + 140, y + 28 != 0 ? y + 28 : y + 29);
}

SwingingPlatform::SwingingPlatform(World& world) : MovingPlatform(world) {
    image = levelImage(kCrateImage);
}

void SwingingPlatform::update() {
    if (y < 20) {
        y += 3;  // comes down first
        return;
    }
    animation.frame += 2;
    animation.frame %= 360;
    y = 80 - ((60 * cosine1024(animation.frame)) >> kTrigShift);
    setPlatform(3, x - 90, x + 140, y + 28 != 0 ? y + 28 : y + 29);
}

FallingCrate::FallingCrate(World& world) : Actor(world) {
    image = levelImage(prop::kCrateContents);
}

void FallingCrate::update() {
    fall();

    // On the ground: engraved, and its top becomes a step.
    const int ground = dy > 0 ? platformAt(x, y + dy) : 0;
    if (ground != 0) {
        y = ground;
        world().media.effects->engrave(x, y - 10, image);
        world().collision.setPlatform(4, x - 90, x + 140, y - 125);
        hitboxFromImage();
        dead = true;
    }
}

//-----------------------------------------------------------------------------
// The bomb
//-----------------------------------------------------------------------------

namespace {

// Animations, from their first image.
const int kWarning[] = {0, 0, 0, 0, 0, 1, 1, 2, 1, 1};
const int kNuke[] = {0, 0, 0, 1, 1, 1, 2, 2, 3, 4, 5, 6, 6, 7, 7, 7, 8, 8,
                     8, 8, 7, 7, 7, 6, 6, 5, 4, 3, 2, 2, 1, 1, 1, 0, 0};
constexpr int kDigits = prop::kBombDigit;  // images of 0..9
constexpr int kNine = kDigits + 9;         // the hundredths count down from 9

// Pickups given for the bombs (EventBonus ids BFUSIL, BPM, BCOW).
constexpr int kShotgunPickup = 1;
constexpr int kSmgPickup = 0;
constexpr int kCowBombPickup = 6;

}  // namespace

Bomb::Bomb(World& world) : Actor(world) {
    image = levelImage(prop::kBomb);
}

void Bomb::update() {
    World& w = world();

    if (!armed_) {
        // Once the third is defused, the timer stops. In 2002 every defused
        // bomb restored the same saved value each tick; one is enough.
        if (holdsTimer_ && w.flags[kBombsDefused] == 3) {
            w.flags[Flag::Timer] = heldTimer_;
        }
        return;
    }

    Entity* target = w.target;
    if (target != nullptr && target->x > x - 50 && target->x < x + 50 && target->y > y - 10 &&
        target->y < y + 51) {
        w.flags[kBombsDefused] += 1;
        armed_ = false;
        int pickup = -1;
        switch (w.flags[kBombsDefused]) {
            case 1:
                pickup = kShotgunPickup;
                break;
            case 2:
                pickup = kSmgPickup;
                break;
            case 3:
                pickup = kCowBombPickup;
                holdsTimer_ = true;
                heldTimer_ = w.flags[Flag::Timer];
                w.flags[Flag::GeneratorsOff] = 1;
                break;
            default:
                break;  // more than three bombs: no pickup (D-42)
        }
        if (pickup >= 0) {
            w.registry->spawn(w, EntityCategory::Pickup, pickup, SpawnPoint{x, 0, 0});
        }
        w.media.levelSounds->play(snd::kBombDefused);
    }

    animation.cycle(4, 9);

    nukeSubFrame_ += 1;
    nukeSubFrame_ %= 3;
    if (nukeSubFrame_ == 0) {
        nukeFrame_ += 1;
        nukeFrame_ %= 35;
    }

    counterStep_ += 2;
    if (counterStep_ >= 100) {
        counterStep_ = 0;
    }

    // The warning light blinks faster as time runs out.
    const int timer = w.flags[Flag::Timer];
    if (timer < 40) {
        warningSubFrame_ += 1;
        if (timer / 3 > 0) {
            warningSubFrame_ %= timer / 3;
        } else {
            warningSubFrame_ = 0;
        }
        if (warningSubFrame_ == 0) {
            warningFrame_ += 1;
            warningFrame_ %= 10;
        }
    }

    seconds_ = timer;
    hundredths_ = counterStep_;
}

void Bomb::draw() {
    Actor::draw();

    const int timer = world().flags[Flag::Timer];
    const auto digits = [this](int sec, int hundredths) {
        drawAt(x + 6, y - 27, levelImage(kDigits + sec / 10));
        drawAt(x + 11, y - 27, levelImage(kDigits + sec % 10));
        drawAt(x + 16, y - 27, levelImage(kNine - hundredths / 10));
        drawAt(x + 21, y - 27, levelImage(kNine - hundredths % 10));
    };

    if (!armed_) {
        // Stopped: shows the time it was defused at.
        drawAt(x, y, levelImage(prop::kBombNuke + kNuke[nukeFrame_]));
        digits(seconds_, hundredths_);
        return;
    }

    drawAt(x, y, levelImage(prop::kBombRing + animation.frame));
    drawAt(x, y, levelImage(prop::kBombNuke + kNuke[nukeFrame_]));
    if (timer >= 100) {
        for (int dx = 6; dx <= 21; dx += 5) {
            drawAt(x + dx, y - 27, levelImage(kNine));  // "9999"
        }
    } else {
        digits(timer, counterStep_);
    }
    if (timer < 40) {
        drawAt(x, y - 35, levelImage(prop::kBombWarning + kWarning[warningFrame_]));
    } else if (timer < 60) {
        drawAt(x, y - 35, levelImage(prop::kBombWarning));
    }
}

}  // namespace bb::game::pokemon
