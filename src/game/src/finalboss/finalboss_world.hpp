#pragma once

// The final boss level (finalboss), ported from the 2002 classes
// (EnnemiMario, TirMarioFireball, TirMarioFireballVertical,
// FondBattlecruiser, TirCruiser, TirCruiserLaser, FondRocherHyperspace,
// FondRocher1, FondRocher2, FondMonolite). Arithmetic, random draws and
// states are the 2002 ones so that the replays keep their fingerprints.

#include "bb/assets/sprites/finalbossenn.hpp"
#include "bb/assets/sprites/finalbossfonda.hpp"
#include "bb/game/actor.hpp"
#include "bb/game/vehicle.hpp"

namespace bb::game::finalboss {

// finalbossenn.gfx (enemies), finalbossfonda.gfx (props). The level has no
// sound bank.
namespace img = bb::assets::sprites::finalbossenn;
namespace prop = bb::assets::sprites::finalbossfonda;

// Mario (700): walks; when the target player is ahead, rises and attacks
// with fireballs: 16 attacks, chosen at random in four sets as his HP drop
// (below 17500, 11500 and 8500). Killed, he sets register 0 = 7 (the
// victory) every tick and stays.
class Mario : public Enemy {
public:
    explicit Mario(World& world);
    void update() override;
    void draw() override;
    void hitBy(Shot& shot) override;

private:
    void walkAbout();  // onAvance()
    void jump();       // onSaute()
    void floatUp();    // tombe_mario()
    void startAttack();
    void endAttack();  // back to walking (speed 1)
    // The attacks, by number (attack_type).
    void throwAcross();        // 0 onTirehorizontale
    void throwUp();            // 1 onTireverticale
    void burstAcross();        // 2 onRafalehorizontale
    void burstUp();            // 3 onRafaleverticale
    void burstUpBack();        // 4 onRafaleverticaleinverser
    void burstAcrossTwice();   // 5 onRafalehorizontaledouble
    void burstUpTwice();       // 6 onRafaleverticaledouble
    void cross();              // 7 onTirecross
    void throwsAcross();       // 8 onRafalehorizontaleconstante
    void throwsUp();           // 9 onRafaleverticaleconstante
    void crosses();            // 10 onRafalecross
    void fireRain();           // 11 onPluiedefeu
    void throwsUpAimed();      // 12 onRafaleverticalechercheuse
    void throwsAcrossAimed();  // 13 onRafalehorizontalechercheuse
    void crossesAimed();       // 14 onRafalecrosschercheuse
    void berserk();            // 15 onRafaleberserker
    // To the edge of the screen (the first phase of 11 and 15).
    void runToEdge();
    // A fireball across at `speed` (boule_de_feu).
    void fireballAcross(int speed);
    // An upward fireball that falls on goalX.
    void fireballUp(int xSpeed, int goalX);

    int speed_ = 1;
    int speedStep_ = 0;
    bool fireball_ = false;  // a fireball in his hands (drawn)
    int attackDelay_;
    int attackWait_ = 0;
    int attackStep_ = 0;  // attack_etape
    int attackSub_ = 0;   // attack_ss_etape
    int attackType_ = 0;
    int attackPhase_ = 0;
    int volleys_ = 0;  // nb_pluie
    int goalX_ = 0;
    int goalY_ = 0;
};

// Mario's fireball across: speeds up by one every 8 ticks.
class Fireball : public Projectile {
public:
    Fireball(World& world, int speed) : Projectile(world), xSpeed_(speed) {}
    void update() override;

private:
    int xSpeed_;
    int speedStep_ = 0;
};

// Mario's upward fireball: rises off the screen, comes back down on its
// goal, falling faster and faster; shakes the screen when it passes the
// floor height of column 0.
class FireballUp : public Projectile {
public:
    FireballUp(World& world, int xSpeed, int goalX) : Projectile(world), xSpeed_(xSpeed), goalX_(goalX) {
        dy = -12;
    }
    void update() override;

private:
    void drop();  // fireball_tombe()

    int xSpeed_;
    int goalX_;
};

// The stork (vehicle 0): flies right at 8 pixels per tick; a player who
// walks into it rides it, unable to fire or turn, and is let off at
// x 1480 (80 pixels up for the first stork, 40 for the second).
class Stork : public Vehicle {
public:
    explicit Stork(World& world);

private:
    bool riderCanFire() const override { return false; }
    bool riderCanTurn() const override { return false; }
    void updateRidden() override;
    void updateEmpty() override;
};

//-----------------------------------------------------------------------------
// Props
//-----------------------------------------------------------------------------

// The battle cruiser (700) of the opening, fighting an enemy cruiser that
// comes the other way; their shots and lasers go into the prop list.
class BattleCruiser : public Actor {
public:
    explicit BattleCruiser(World& world) : Actor(world) {
        x = -20;
        y = 80;
    }
    void update() override;
    void draw() override;

private:
    int enemyX_ = 700;
    int enemyY_ = 40;
    int attackStep_ = -2;
    int flameStep_ = 0;
    int flameSub_ = 0;
};

// A cruiser's shot: flies to the other cruiser's height and bursts.
class CruiserShot : public Actor {
public:
    CruiserShot(World& world, int picture, int speedX, int speedY) : Actor(world), xSpeed_(speedX) {
        image = levelImage(picture);
        dy = speedY;
    }
    void update() override;

private:
    int xSpeed_;
};

// A cruiser's laser flash: 4 steps; at the first it jumps to the end of
// its beam, by its image (D-87 fixed: the 2002 class read the image number
// from a member never set, so the flash never moved).
class CruiserLaser : public Actor {
public:
    CruiserLaser(World& world, int picture, int side) : Actor(world), picture_(picture), toRight_(side > 0) {
        image = levelImage(picture);
        animation.subFrame = 2;
    }
    void update() override;

private:
    int picture_;
    bool toRight_;
};

// A rock of hyperspace (701): placed at random in one of three planes by
// its starting x (far, middle, near: slower and smaller when far), drifts
// right, bobbing, and comes back from the left of the screen.
class HyperspaceRock : public Actor {
public:
    explicit HyperspaceRock(World& world);
    void update() override;
    void draw() override;

private:
    // Its speed, image and bobbing speed for its plane.
    void pickLook();

    bool placing_ = true;  // placer
    int plane_ = 0;        // 0 near, 1 middle, 2 far
    int angle_;            // delta, degrees
    int xSpeed_ = 0;       // x_delay
    int bobSpeed_ = 1;     // sinusoide_delay
    int baseY_ = 0;
};

// The floating monolith (702) and rocks (703, 704): moving platforms that
// bob up and down and carry the players (the monolith carries the enemies
// too).
struct FloaterSpec {
    int still;  // the image before the first update
    int first;  // animation images from here
    const int* animation;
    int animationSteps;
    int animationDelay;
    const int* bob;                   // pixels per step, down then up
    int bobHalf;                      // steps down
    int bobSteps;                     // steps in a period
    int bobDelay;                     // ticks per step
    int top;                          // the platform is this far above y
    int carryLeft, carryRight;        // carried between x - left and x + width - right
    int platformLeft, platformRight;  // the platform from x - left to x + width - right
    bool carriesEnemies;
};

class Floater : public MovingPlatform {
public:
    Floater(World& world, const FloaterSpec& spec) : MovingPlatform(world), spec_(spec) {
        image = levelImage(spec.still);
    }
    void update() override;

    static const FloaterSpec kMonolith;
    static const FloaterSpec kRock1;
    static const FloaterSpec kRock2;

private:
    void carry(EntityList<Entity>& list, int width, int step);

    const FloaterSpec& spec_;
    int bobFrame_ = 0;  // etape_levitation
    int bobSub_ = 0;    // ss_etape_levitation
};

}  // namespace bb::game::finalboss
