#pragma once

// The Care Bear world (bisous1 to bisous3), ported from the 2002 classes
// (EnnemiBisou*, TirArcCiel, TirEtoile, TirNuage, TirEpeeLaser,
// TirBouleChaman, TirColonneChaman, the bear gore pieces). As for the other
// worlds, arithmetic, random draws and states are the 2002 ones so that the
// replays keep their fingerprints.

#include "bb/assets/sounds/bisou.hpp"
#include "bb/assets/sounds/misc.hpp"
#include "bb/assets/sprites/bisouenn.hpp"
#include "bb/game/actor.hpp"

namespace bb::game::carebears {

// bisouenn.gfx (enemies) and bisou.sfx.
namespace img = bb::assets::sprites::bisouenn;
namespace snd = bb::assets::sounds::bisou;
namespace miscsnd = bb::assets::sounds::misc;

// 2002 character states beyond actor.hpp's (ETAT_MEURE_2..4).
constexpr int kDying2 = 11;
constexpr int kDying3 = 12;
constexpr int kDying4 = 13;

// What tells the sky and star bears apart: both are cut in half when they
// die (the legs fall as gore, the top falls, lands and is engraved).
struct HalvedBearImages {
    int walkRight, walkLeft;
    int dieRight, dieLeft;          // before the cut, 4 images
    int topFallRight, topFallLeft;  // the top falling, 4 images
    int topEndRight, topEndLeft;    // the top on the ground, 4 images
};

class HalvedBear : public Enemy {
public:
    HalvedBear(World& world, const HalvedBearImages& images) : Enemy(world), images_(images) {}

    void update() override;
    void hitBy(Shot& shot) override;

protected:
    virtual void walkAbout() = 0;
    virtual void afterShot() = 0;  // onTire(): the pause after a shot
    virtual void dropLegs() = 0;   // the gore piece of its legs
    virtual void beforeUpdate() {}

    void walkImage(int step);
    void dieWhole();  // onMeureEntier()
    void topFalls();  // onMeureHaut()
    void topLands();  // onMeureHautFin()
    void burnUp();    // onCarbonise()

    const HalvedBearImages& images_;
};

// The sky bear (200): walks, and pees a rainbow on a player close by.
class SkyBear : public HalvedBear {
public:
    explicit SkyBear(World& world);

private:
    void beforeUpdate() override;
    void walkAbout() override;
    void afterShot() override;
    void dropLegs() override;

    int peeWait_ = 0;
};

// The star bear (201): walks without leaving its platform and throws
// bouncing stars.
class StarBear : public HalvedBear {
public:
    explicit StarBear(World& world);

private:
    void beforeUpdate() override;
    void walkAbout() override;
    void afterShot() override;
    void dropLegs() override;

    int shotDelay_ = 0;
    int shotWait_ = 0;
};

// The boom bear (202): runs at the target player and blows up.
class BoomBear : public Enemy {
public:
    explicit BoomBear(World& world) : Enemy(world) { health.points = 200; }
    void update() override;
    void hitBy(Shot& shot) override;

private:
    void walkAbout();
    void explode();
};

// The cloud bear (203): sits on its cloud and releases storm clouds.
class CloudBear : public Enemy {
public:
    explicit CloudBear(World& world);
    void update() override;

private:
    void sit();
    void hitWait();   // onMeureWait()
    void fallDown();  // onMeureTombe()
    void finish();    // onMeureFin()

    int shotDelay_ = 0;
    int shotWait_ = 0;
};

// The zombie bear (204): shot below 500 HP it loses its legs and crawls on;
// killed by the shaman's death. Handles its hits itself.
class ZombieBear : public Enemy {
public:
    explicit ZombieBear(World& world) : Enemy(world) {
        health.points = 1000;
        treasure = 10;  // (unused: its kills skip the shared pipeline)
    }
    void update() override;
    void hitBy(Shot& shot) override;

private:
    static constexpr int kCrawling = 100;    // ETAT_RAMPE
    static constexpr int kCut = 101;         // ETAT_COUPE
    static constexpr int kTopBurning = 102;  // ETAT_CARBONISE_HAUT

    void walkAbout();
    void cut();       // onCoupe()
    void topFalls();  // onTombe()
    void crawl();     // onRampe()
    void die();
    void burnUp(int firstRight, int firstLeft, int lastStep);

    bool walkPhase_ = false;  // my_phase: walks every other tick
    int crawlStep_ = 0;       // etape2
};

// The shaman (205), boss of bisous3: casts fireballs or rolling columns at
// the target player and teleports between three spots after ten hits. His
// death (register 0 = 7) burns the zombies and ends the level.
class Shaman : public Enemy {
public:
    explicit Shaman(World& world);
    void update() override;
    void draw() override;
    void hitBy(Shot& shot) override;

private:
    static constexpr int kRaisingStaff = 30;   // ETAT_LEVEBATON
    static constexpr int kLoweringStaff = 31;  // ETAT_BAISSEBATON
    static constexpr int kTeleporting = 32;    // ETAT_TELE

    void stand();  // onAvance()
    void raiseStaff();
    void lowerStaff();
    void cast();      // onTire()
    void teleport();  // onTele()
    void burnUp();    // onCarbonise()

    int shotDelay_ = 0;
    int shotWait_ = 0;
    int glows_ = 0;  // nb_lueurs
    int hits_ = 0;   // nb_touch
    int spot_ = 1;   // n_pos
};

// The Jedi bear (207): walks and throws a homing lightsaber that comes back.
class Jedi : public Enemy {
public:
    explicit Jedi(World& world);
    void update() override;
    void draw() override;
    void hitBy(Shot& shot) override;

    bool holdsSword = true;  // a_epee

protected:
    void walkAbout();

private:
    void throwSword(bool targetKnown);
    void die();
    void burnUp();

    int shotDelay_ = 0;
    int shotWait_ = 0;
};

// The Jedi hero (208), bisous1's last enemy: walks in, sets register 1 to 1
// (the dialogue) and turns the generators on; killed, register 1 = 2.
class JediHero : public Jedi {
public:
    explicit JediHero(World& world) : Jedi(world) { health.points = 6000; }
    void update() override;
    void hitBy(Shot& shot) override;

private:
    bool entering_ = true;
};

// The Snork in Black cameo (209): waits, then runs off to the right; cannot
// be killed (300 hits make it run).
class SnorkCameo : public Enemy {
public:
    explicit SnorkCameo(World& world);
    void update() override;
    void hitBy(Shot& shot) override;
    // Shots go through it and it never dies: not in the kill percentage
    // (D-55 fixed).
    bool countsForKills() override { return false; }

private:
    static constexpr int kWaitingHurt = 100;  // ETAT_ATTENDS_MAL
    static constexpr int kWaiting = 101;      // ETAT_ATTENDS
    static constexpr int kRunning = 102;      // ETAT_COURS_FOREST
    static constexpr int kGoingIn = 103;      // ETAT_RENTRE_MAL

    int hits_ = 0;
};

//-----------------------------------------------------------------------------
// Shots
//-----------------------------------------------------------------------------

// The sky bear's rainbow: a short spray in front of it.
class Rainbow : public Projectile {
public:
    explicit Rainbow(World& world) : Projectile(world) {}
    void update() override;
};

// The star bear's star: bounces along for 500 ticks.
class BouncingStar : public Projectile {
public:
    explicit BouncingStar(World& world) : Projectile(world) {}
    void update() override;

private:
    int lifetime_ = 500;
};

// The cloud bear's storm cloud: drifts, and strikes lightning down on the
// target player below it.
class StormCloud : public Projectile {
public:
    explicit StormCloud(World& world);
    void update() override;
    void draw() override;

private:
    void appear();
    void vanish();
    void drift();
    void strike();

    int phase_ = 0;  // 0 appearing, 1 drifting, 2 vanishing, 3 striking
    int strikeStep_ = 0;
    int boltSubFrame_ = 0;
    int bolt_ = 0;
    int lifetime_ = 0;
};

// The Jedi's lightsaber: homes on the target player, then flies back to
// the Jedi (who can throw it again); lost when the Jedi dies.
class Lightsaber : public Projectile {
public:
    explicit Lightsaber(World& world);
    ~Lightsaber() override;
    void update() override;

    Handle<Jedi> thrower;
    Handle<> target;
    bool lost = false;  // isdead
    int xSpeed = 0;     // in 1/10 pixel per tick, as dy

private:
    bool returning_ = false;
    int lifetime_ = 0;
    int acceleration_ = 1;
};

// The shaman's fireball: grows, flies straight, bursts on a wall.
class ShamanBall : public Projectile {
public:
    explicit ShamanBall(World& world) : Projectile(world) {}
    void update() override;

    int xSpeed = 0;  // its own dx; dy is the actor's

private:
    bool flying_ = false;    // ok
    bool bursting_ = false;  // boom
};

// The shaman's column: rises, rolls along the ground, sinks.
class ShamanColumn : public Projectile {
public:
    explicit ShamanColumn(World& world);
    void update() override;

private:
    void roll();  // deplace()

    int lifetime_ = 0;
    int phase_ = 0;  // 0 rising, 1 rolling, 2 sinking
};

//-----------------------------------------------------------------------------
// Gore
//-----------------------------------------------------------------------------

// Legs left behind by a halved bear or a zombie: they walk back a few
// steps, fall over and are engraved.
class WalkingLegs : public Actor {
public:
    WalkingLegs(World& world, int firstRight, int firstLeft)
        : Actor(world), firstRight_(firstRight), firstLeft_(firstLeft) {}
    void update() override;

private:
    int firstRight_;
    int firstLeft_;
};

// The star bear's legs: the same, then they dance for 100 ticks.
class DancingLegs : public Actor {
public:
    explicit DancingLegs(World& world) : Actor(world) {}
    void update() override;

private:
    bool dancing_ = false;
    int ticks_ = 0;
};

// A piece of a boom bear: flies off, lands and is engraved with a splat.
class BoomPiece : public Actor {
public:
    BoomPiece(World& world, int firstRight, int firstLeft, int speedRight)
        : Actor(world), firstRight_(firstRight), firstLeft_(firstLeft), speedRight_(speedRight) {}
    void update() override;

private:
    int firstRight_;
    int firstLeft_;
    int speedRight_;  // horizontal speed when facing right (sign included)
};

}  // namespace bb::game::carebears
