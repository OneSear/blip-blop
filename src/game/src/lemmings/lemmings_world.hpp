#pragma once

// The Lemmings bonus level (lem), ported from the 2002 classes
// (EnnemiLemmings, EnnemiLemmingsVolant, FondVentilo, FondLumiere,
// FondLumiere2, FondEau*). Arithmetic, random draws and states are the
// 2002 ones so that the replays keep their fingerprints.

#include "bb/assets/sounds/lem.hpp"
#include "bb/assets/sounds/misc.hpp"
#include "bb/assets/sprites/lemenn.hpp"
#include "bb/assets/sprites/lemfond.hpp"
#include "bb/game/actor.hpp"

namespace bb::game::lemmings {

namespace img = bb::assets::sprites::lemenn;
namespace prop = bb::assets::sprites::lemfond;
namespace snd = bb::assets::sounds::lem;
namespace miscsnd = bb::assets::sounds::misc;

// A lemming (400; the flying lemming, 401, is the same: its 2002
// constructor only set y, which the spawner overwrites). Walks and turns
// at walls; after 200 kills every lemming blows up, and the last one
// drops a cow bomb pickup and sets register 0 to 1 (the level's dialogue).
class Lemming : public Enemy {
public:
    explicit Lemming(World& world) : Enemy(world) { health.points = 100; }
    void update() override;
    void hitBy(Shot& shot) override;

private:
    void walkAbout();  // onAvance()
    void fallDown();   // onTombe()
    void die();        // onMeure()

    bool nuked_ = false;  // suicide
};

// The fan (400).
class Fan : public Actor {
public:
    explicit Fan(World& world) : Actor(world) {}
    void update() override;
};

// A flickering light (401 from image `first` 0, 403 from 17): lit for a
// random while, then flickers.
class FlickeringLight : public Actor {
public:
    FlickeringLight(World& world, int first);
    void update() override;

private:
    int first_;
    bool lit_ = false;
    int litWait_ = 0;
    int litDelay_ = 0;
};

// The water (402): lays water tiles along the whole level, and animates
// them through two shared counters.
class Water : public Actor {
public:
    explicit Water(World& world);
    void update() override;
};

// One water tile; `second` tiles use the second counter.
class WaterTile : public Actor {
public:
    WaterTile(World& world, bool second) : Actor(world), second_(second) {}
    void update() override;

private:
    bool second_;
};

}  // namespace bb::game::lemmings
