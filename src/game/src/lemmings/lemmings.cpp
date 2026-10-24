// The Lemmings level (ennemi_lemmings.cpp, ennemi_lemmings_volant.h,
// fond_ventilo.h, fond_lumiere.cpp, fond_lampe_2.cpp, fond_eau.h).

#include "bb/game/lemmings_world.hpp"
#include "lemmings/lemmings_world.hpp"

namespace bb::game {

namespace lemmings {

namespace {

// 2002 file statics, kept in World::shared() (D-50): every 20th lemming
// killed plays the rare cry (the others draw a random cry), and the two
// water counters; reset at each level start (D-50).
struct LemmingCounters {
    int deathCount = 0;
    int waterStep = 0;
    int waterStep2 = 5;
};

constexpr int kNukeKills = 200;
constexpr Flag kNuked = Flag::User0;     // the 200th kill blew them all up
constexpr Flag kSoundCue = Flag::User3;  // set by a dialogue: play the cue
constexpr int kCowBombPickup = 6;        // EventBonus BCOW

}  // namespace

//-----------------------------------------------------------------------------
// Lemming
//-----------------------------------------------------------------------------

void Lemming::update() {
    World& w = world();

    if (bleeding != 0) {
        bleeding -= 1;
    }

    // 200 lemmings killed: they all blow up and the generators stop.
    if (w.flags[Flag::Kills] >= kNukeKills && mode != kDying && mode != kCarbonised) {
        nuked_ = true;
        collides = false;
        subFrame = 0;
        frame = 0;
        mode = kDying;
        w.flags[Flag::GeneratorsOff] = 1;
    }

    // Set by the level's dialogue: a sound cue.
    if (w.flags[kSoundCue] == 1) {
        w.media.levelSounds->play(snd::kScriptCue);
        w.flags[kSoundCue] = 0;
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
            burn(img::kLemmingBurnLeft, img::kLemmingBurnRight, 14);
            break;
        default:
            break;
    }
    dieWhenOffscreen();
}

void Lemming::walkAbout() {
    subFrame += 1;
    subFrame %= 5;
    if (subFrame == 0) {
        frame += 1;
        frame %= 14;
    }

    if (direction == kFacingLeft) {
        x -= 1;
        fall();
        image = enemyImage(img::kLemmingWalkLeft + frame);
        if (solidWall(x, y) || x < 5) {
            direction = kFacingRight;
        }
    } else {
        x += 1;
        fall();
        image = enemyImage(img::kLemmingWalkRight + frame);
        if (solidWall(x, y)) {
            direction = kFacingLeft;
        }
    }
    hitboxFromImage();
}

void Lemming::fallDown() {
    fall();
    if (platformAt(x, y) != 0) {
        mode = kWalking;
    }
    hitboxFromImage();
}

void Lemming::die() {
    World& w = world();
    fall();

    subFrame += 1;
    subFrame %= 5;
    if (subFrame == 0) {
        frame += 1;
    }

    // The explosion is cut short at a platform edge.
    if (frame >= 6 && (platformAt(x - 80, y) == 0 || platformAt(x + 80, y) == 0)) {
        dead = true;
    }

    if (frame == 10) {
        if (platformAt(x, y) != 0 && platformAt(x - 80, y) != 0 && platformAt(x + 80, y) != 0) {
            w.media.effects->engrave(x, y, image);
        }
        dead = true;
        if (nuked_ && w.flags[kNuked] == 0) {
            w.registry->spawn(w, EntityCategory::Pickup, kCowBombPickup,
                              SpawnPoint{w.marker("lemmings.cowBombX"), -50, 0});
            w.flags[kNuked] = 1;
        }
    } else {
        image = enemyImage((direction == kFacingLeft ? img::kLemmingDieLeft : img::kLemmingDieRight) + frame);
    }
}

void Lemming::hitBy(Shot& shot) {
    static const int kBloodX[] = {0, 0, 4, 0, 0, 0, -4, 0};
    static const int kBloodY[] = {-15, -15, -15, -15, -15, -15, -15, -15};
    World& w = world();

    Enemy::hitBy(shot);
    sprayBlood(shot, kBloodX, kBloodY);

    if (mode == kDying) {
        int& deathCount = w.shared<LemmingCounters>().deathCount;
        deathCount += 1;
        deathCount %= 20;
        if (deathCount == 0) {
            w.media.levelSounds->play(snd::kLemmingDeathRare);
        } else {
            w.media.miscSounds->play(miscsnd::kLemmingDeath + w.random() % 3);  // one of 3
        }
    }
}

//-----------------------------------------------------------------------------
// Props
//-----------------------------------------------------------------------------

void Fan::update() {
    subFrame += 1;
    subFrame %= 3;
    if (subFrame == 0) {
        frame += 1;
        frame %= 4;
    }
    image = levelImage(prop::kFan + frame);
    if (x < world().camera.x - 100) {
        dead = true;
    }
}

namespace {

// 2 lit, 1 dim, 0 off.
const int kFlicker[] = {2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 1, 2,
                        2, 2, 1, 2, 2, 1, 2, 1, 2, 1, 2, 1, 1, 2, 1, 1, 1, 2, 1, 1,
                        1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 0, 0};

}  // namespace

FlickeringLight::FlickeringLight(World& world, int first) : Actor(world), first_(first) {
    litDelay_ = 50 + world.random() % 200;
    image = levelImage(first);
}

void FlickeringLight::update() {
    litWait_ += 1;
    if (lit_) {
        if (litWait_ >= litDelay_) {
            lit_ = false;
            subFrame = 0;
            frame = 0;
        }
    } else {
        image = levelImage(first_ + animate(kFlicker, 58, 3));
        if (frame == 57) {
            lit_ = true;
            litWait_ = 0;
            litDelay_ = 50 + world().random() % 200;
        }
    }
    if (x < world().camera.x - 300) {
        dead = true;
    }
}

Water::Water(World& world) : Actor(world) {
    // The tiles, from x = 0 (2002: the constructor ran before the event set
    // the water's position), alternating, every 376 pixels.
    bool firstCounter = false;
    const int tiles = world.collision.width / levelImage(prop::kWater)->width() + 1;
    for (int i = 0; i < tiles; ++i) {
        auto* tile = new WaterTile(world, !firstCounter);
        tile->x = x;
        tile->y = 454;
        x += 376;
        firstCounter = !firstCounter;
        world.props.add(tile);
    }
}

void Water::update() {
    subFrame += 1;
    subFrame %= 7;
    if (subFrame == 0) {
        LemmingCounters& counters = world().shared<LemmingCounters>();
        counters.waterStep += 1;
        counters.waterStep %= 10;
        counters.waterStep2 += 1;
        counters.waterStep2 %= 10;
    }
}

void WaterTile::update() {
    const LemmingCounters& counters = world().shared<LemmingCounters>();
    image = levelImage(prop::kWater + (second_ ? counters.waterStep2 : counters.waterStep));
    if (x < world().camera.x - 400) {
        dead = true;
    }
}

}  // namespace lemmings

//-----------------------------------------------------------------------------
// Registration
//-----------------------------------------------------------------------------

namespace {

using namespace lemmings;

Entity* makeLemming(World& world, const SpawnPoint& at) {
    auto* lemming = new Lemming(world);
    lemming->x = at.x;
    lemming->y = at.y;
    lemming->direction = at.direction;
    return lemming;
}

template <class T>
Entity* makeProp(World& world, const SpawnPoint& at) {
    auto* p = new T(world);
    p->x = at.x;
    p->y = at.y;
    return p;
}

template <int First>
Entity* makeLight(World& world, const SpawnPoint& at) {
    auto* light = new FlickeringLight(world, First);
    light->x = at.x;
    light->y = at.y;
    return light;
}

}  // namespace

void registerLemmingsWorld(EntityRegistry& r) {
    r.add({EntityCategory::Enemy, 400, "lemmings.lemming", Placement::Enemies, makeLemming});
    r.add({EntityCategory::Enemy, 401, "lemmings.flying_lemming", Placement::Enemies, makeLemming});
    r.add({EntityCategory::Prop, 400, "lemmings.fan", Placement::Props, makeProp<Fan>});
    r.add({EntityCategory::Prop, 401, "lemmings.light", Placement::Props, makeLight<prop::kLight>});
    r.add({EntityCategory::Prop, 402, "lemmings.water", Placement::Props, makeProp<Water>});
    r.add({EntityCategory::Prop, 403, "lemmings.light_2", Placement::Props, makeLight<prop::kLight2>});
}

}  // namespace bb::game
