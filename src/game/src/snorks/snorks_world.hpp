#pragma once

// The Snork world (snorkniv, snorkniv2), ported from the 2002 classes
// (EnnemiSnorky*, EnnemiSnorkInBlack*, TirSnorky*, TirEpine for these
// levels, Morceau*). Arithmetic, random draws and states are the 2002 ones
// so that the replays keep their fingerprints.

#include "bb/assets/sounds/snork.hpp"
#include "bb/assets/sprites/snorkenn2.hpp"
#include "bb/game/actor.hpp"

namespace bb::game::snorks {

// snorkenn2.gfx (enemies) and snork.sfx.
namespace img = bb::assets::sprites::snorkenn2;
namespace snd = bb::assets::sounds::snork;

// A walking Snork trooper: walks, falls off platforms, shoots now and
// then (the 2002 grunt, gunner, governor and Snork in Black shared this
// loop by copy).
class Trooper : public Enemy {
public:
    explicit Trooper(World& world) : Enemy(world) {}
    void update() override;

protected:
    virtual void walkAbout() = 0;  // onAvance()
    virtual void shoot() = 0;      // onTire()
    virtual void die() = 0;        // onMeure()
    virtual void burnUp() = 0;     // onCarbonise()
    virtual bool bleeds() const { return true; }

    // Starts falling when there is no ground (true then).
    bool startFalling();
    void fallDown();  // onTombe()
    // Walks `speed` pixels and turns at walls, the left limit and the
    // right edge of the screen, after the step (2002 order).
    void stepAndTurn(int speed, int imageRight, int imageLeft, const int* cycle, int steps);

    int shotWait_ = 0;
    int shotDelay_ = 0;
};

// The grunt (300): walks and fires an oscillating, accelerating shot.
// Killed, it loses its head.
class Grunt : public Trooper {
public:
    explicit Grunt(World& world);
    void hitBy(Shot& shot) override;

private:
    void walkAbout() override;
    void shoot() override;
    void die() override;
    void burnUp() override;
};

// The gunner (301): fires aimed shots at the target player.
class Gunner : public Trooper {
public:
    explicit Gunner(World& world);
    void hitBy(Shot& shot) override;

private:
    void walkAbout() override;
    void shoot() override;
    void die() override;
    void burnUp() override;

    int aimX_ = 0;
    int aimY_ = 0;
};

// The governor (302): fires five homing shots in a row.
class Governor : public Trooper {
public:
    explicit Governor(World& world);
    void hitBy(Shot& shot) override;

private:
    void walkAbout() override;
    void shoot() override;
    void die() override;
    void burnUp() override;

    int shots_ = 0;
};

// The Snork in Black (305): fires bursts of spines when level with the
// target player. Its blood timer never counts down (it bleeds once).
class SnorkInBlack : public Trooper {
public:
    explicit SnorkInBlack(World& world);
    void hitBy(Shot& shot) override;

private:
    void walkAbout() override;
    void shoot() override;
    void die() override;
    void burnUp() override;
    bool bleeds() const override { return false; }

    int shots_ = 0;
    int aimX_ = 0;
    int aimY_ = 0;
};

// The mage (303): floats, bobbing, and casts a diagonal shot down at the
// target player.
class Mage : public Enemy {
public:
    explicit Mage(World& world);
    void update() override;
    void hitBy(Shot& shot) override;

private:
    void floatAbout();  // onAvance()
    void fallDown();    // onTombe()
    void die();
    void cast();  // onTire()

    int shotWait_ = 0;
    int shotDelay_ = 0;
    int bobStep_ = 0;
    int focus_ = 0;  // concentration
};

// The seahorse rider (304): swims, charges; killed, the rider falls off.
class SeahorseRider : public Enemy {
public:
    explicit SeahorseRider(World& world);
    void update() override;
    void draw() override;
    void hitBy(Shot& shot) override;

private:
    void swim();  // onAvance()
    void charge();
    void die();
    void burnAway();  // onCarbonise()

    int chargeWait_ = 0;
    int chargeDelay_ = 0;
    bool fin_ = true;  // nageoire: the fin is drawn
    int attackStep_ = 0;
};

// The Snork in Black boss (307), end of snorkniv2: an intro (register 0 =
// 1), then jumps between four perches and fires spines; killed, he goes
// back to the first perch (register 0 = 3) and a shark eats him (4).
class SnorkBoss : public Enemy {
public:
    explicit SnorkBoss(World& world);
    void update() override;
    void draw() override;
    void hitBy(Shot& shot) override;

private:
    void stand();  // onAvance()
    void jump();   // onTombe()
    void die();    // onMeure()
    void fire();   // onTire()
    // The jump from the current perch to `destination` (dir, dx, dy).
    void aimJump(int destination);
    bool landJump();  // the shared end of jump() and die()

    int perch_ = 1;
    int attackType_ = 0;
    int xSpeed_ = 0;
    bool attacking_ = false;
    int sharkX_ = 0;  // set from the level ("snorks.sharkPath")
    int sharkY_ = 100;
    bool shark_ = false;
    bool jumping_ = false;
};

//-----------------------------------------------------------------------------
// Shots
//-----------------------------------------------------------------------------

// The grunt's shot: accelerates and bobs.
class GruntShot : public Projectile {
public:
    GruntShot(World& world, int speed);
    void update() override;

private:
    int xSpeed_;
    int bobStep_ = 0;
    int speedStep_ = 0;
};

// The gunner's shot: straight.
class GunnerShot : public Projectile {
public:
    GunnerShot(World& world, int speedX, int speedY);
    void update() override;

private:
    int xSpeed_;
};

// The governor's shot: homes on its target for 100 ticks.
class GovernorShot : public Projectile {
public:
    GovernorShot(World& world, Entity* target, int speedX, int speedY);
    void update() override;

private:
    Handle<> target_;
    int xSpeed_;
    int inertiaX_ = 0;
    int inertiaY_ = 0;
    int age_ = 0;
};

// The mage's shot: diagonal down; shows its impact before walls.
class MageShot : public Projectile {
public:
    explicit MageShot(World& world) : Projectile(world) { dy = 6; }
    void update() override;

private:
    int xSpeed_ = 0;
};

// A spine (the 2002 TirEpine, as used by the Snorks): straight, stops at
// walls.
class Spine : public Projectile {
public:
    Spine(World& world, int image, int speedX, int speedY);
    void update() override;

private:
    int xSpeed_;
};

//-----------------------------------------------------------------------------
// Debris (in the blood-spray list, as in 2002)
//-----------------------------------------------------------------------------

// A grunt's head: rolls, throws its trunk and its back, is engraved.
class GruntHead : public Actor {
public:
    explicit GruntHead(World& world) : Actor(world) { dy = 0; }
    void update() override;
};

class GruntHeadBack : public Actor {
public:
    GruntHeadBack(World& world, int speedX, int speedY) : Actor(world), xSpeed_(speedX) { dy = speedY; }
    void update() override;

private:
    int xSpeed_;
};

class GruntTrunk : public Actor {
public:
    GruntTrunk(World& world, int speedX, int speedY) : Actor(world), xSpeed_(speedX) { dy = speedY; }
    void update() override;

private:
    int xSpeed_;
};

// The seahorse rider falling off.
class RiderBody : public Actor {
public:
    RiderBody(World& world, int speedX) : Actor(world), xSpeed_(speedX) { dy = -1; }
    void update() override;

private:
    int xSpeed_;
};

}  // namespace bb::game::snorks
