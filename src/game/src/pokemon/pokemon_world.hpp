#pragma once

// The Pokémon bonus level (dork): its enemies and props, ported from the
// 2002 classes (EnnemiPikachu, EnnemiBulbizarre, EnnemiFlameche,
// EnnemiCOM, EnnemiCOMVolant, FondPoke*, FondBombe). As for the Smurf
// world, the arithmetic, the random draws and the states are the 2002
// ones so that the replays keep their fingerprints. Image numbers are
// indices in the level's enemy bank (enemies) or level bank (props).

#include "bb/assets/sounds/dork.hpp"
#include "bb/assets/sounds/misc.hpp"
#include "bb/assets/sprites/dorkfonda.hpp"
#include "bb/assets/sprites/pokeenn.hpp"
#include "bb/game/actor.hpp"

namespace bb::game::pokemon {

// dork's banks: pokeenn.gfx (enemies), dorkfonda.gfx (props), dork.sfx.
namespace img = bb::assets::sprites::pokeenn;
namespace prop = bb::assets::sprites::dorkfonda;
namespace snd = bb::assets::sounds::dork;
namespace miscsnd = bb::assets::sounds::misc;

// What tells the three walking Pokémon apart (2002: three copies of one
// class).
struct WalkerSpec {
    int turnSpeed;  // how far ahead it looks for a wall or the screen edge
    int walkSpeed;
    int walkRight;  // first of the two walking images
    int walkLeft;
    int dieDelay;  // ticks per death image
    int dieSteps;
    int dieRight;
    int dieLeft;
    bool engraves;  // the corpse becomes part of the decor
    bool burns;     // its own burnt animation (otherwise the death one)
    int burnRight;
    int burnLeft;
    bool bleedsAgain;  // the blood timer counts down (Charmander's never does)
    bool cries;        // a death sound
    const int* bloodX;
    const int* bloodY;
};

extern const WalkerSpec kPikachu;     // 100
extern const WalkerSpec kBulbasaur;   // 101
extern const WalkerSpec kCharmander;  // 102

// Walks between walls and screen edges, falls off platforms.
class Walker : public Enemy {
public:
    Walker(World& world, const WalkerSpec& spec) : Enemy(world), spec_(spec) { health.points = 100; }

    void update() override;
    void hitBy(Shot& shot) override;

private:
    void walkAbout();
    void die();

    const WalkerSpec& spec_;
};

// The COM (103): flies sideways between walls and screen edges.
class Com : public Enemy {
public:
    explicit Com(World& world) : Enemy(world) { health.points = 100; }
    void update() override;

protected:
    void die();  // shared with the flying COM
};

// The flying COM (104): swoops down to the target player's height, then
// climbs back and swoops again at each screen edge.
class FlyingCom : public Com {
public:
    explicit FlyingCom(World& world);
    void update() override;

private:
    void aimAtTarget();
    void fly();

    int speed_ = 0;
    bool attacking_ = true;
    int targetY_ = 0;
};

// The blinking alarm light (props 100, 101, 102): 14 images from `first`.
class Alarm : public Actor {
public:
    Alarm(World& world, int first) : Actor(world), first_(first) {}
    void update() override;

private:
    int first_;
};

// The "GO" arrow (103).
class Arrow : public Actor {
public:
    explicit Arrow(World& world) : Actor(world) {}
    void update() override;
};

// The window with a scrolling sky (104).
class SkyWindow : public Actor {
public:
    explicit SkyWindow(World& world);
    void update() override;
    void draw() override;
};

// The flickering "LEVEL 2" / "LEVEL 3" neon signs (110, 111): each letter
// flickers on its own period.
class NeonSign : public Actor {
public:
    NeonSign(World& world, bool levelTwo);
    void update() override;
    void draw() override;

private:
    bool levelTwo_;
    int letterL1_ = 0;
    int letterE1_ = 0;
    int letterV_ = 0;
    int letterE2_ = 0;
    int letterL2_ = 0;
};

// A lift (105, 108): goes up and down once level register 2 exceeds
// `switchesNeeded`, carrying the players on it. Lift 2 waits off screen.
class Lift : public MovingPlatform {
public:
    Lift(World& world, int switchesNeeded);
    void update() override;

private:
    int switchesNeeded_;
};

// A wall switch (106): touched by the target player, it adds one to level
// register 2; the switches also show the level's instructions.
class Switch : public Actor {
public:
    explicit Switch(World& world) : Actor(world) {}
    void update() override;

private:
    bool on_ = false;
    bool waitShown_ = false;
    bool liftShown_ = false;
    bool wellDoneShown_ = false;
};

// The locked crate (107): comes down once register 2 exceeds 2, unlocks,
// drops its contents and lowers three platforms.
class LockedCrate : public MovingPlatform {
public:
    explicit LockedCrate(World& world);
    void update() override;
    void draw() override;
};

// A crate platform that comes down to a height and stays.
class CratePlatform : public MovingPlatform {
public:
    CratePlatform(World& world, int targetY);
    void update() override;

private:
    int targetY_;
};

// The crate platform that swings up and down for ever.
class SwingingPlatform : public MovingPlatform {
public:
    explicit SwingingPlatform(World& world);
    void update() override;
};

// The crate's contents: falls and becomes a step in the decor.
class FallingCrate : public Actor {
public:
    explicit FallingCrate(World& world);
    void update() override;
};

// A bomb (109; 900 is the same bomb as a plain prop): the target player
// next to it defuses it, adds one to register 0 and gets a pickup; the
// third one stops the generators and the timer.
class Bomb : public Actor {
public:
    explicit Bomb(World& world);
    void update() override;
    void draw() override;

private:
    int counterStep_ = 0;
    int seconds_ = 0;
    int hundredths_ = 0;
    bool armed_ = true;
    int warningFrame_ = 0;
    int warningSubFrame_ = 0;
    int nukeFrame_ = 0;
    int nukeSubFrame_ = 0;
    // Set on the bomb defused last: it holds the timer from then on (the
    // 2002 file global save_time, D-50).
    bool holdsTimer_ = false;
    int heldTimer_ = 0;
};

}  // namespace bb::game::pokemon
