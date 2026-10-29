#pragma once

// The video-game boss level (videoboss), ported from the 2002 classes
// (EnnemiLara, EnnemiPic, EnnemiPoid, EnnemiTeteRayman, EnnemiMariotapette,
// EnnemiLuigi, EnnemiSonic, EnnemiVoiturePacman, TirEpine as Lara uses it,
// TirDebouchechiote, TirSoniceclair, MorceauSonicRing and the Fond* props
// of videoboss). Arithmetic, random draws and states are the 2002 ones so
// that the replays keep their fingerprints. The level is a script: register
// 0 is its stage (Lara: 1 head thrown, 2 button, 3 draws, 4 fights, 5 to 7
// crushed; then 0 again for the plumbers and Sonic, 7 the end).

#include "bb/assets/sounds/boss.hpp"
#include "bb/assets/sprites/videobossenn.hpp"
#include "bb/assets/sprites/videofondaboss.hpp"
#include "bb/game/actor.hpp"

namespace bb::game::videoboss {

// videobossenn.gfx (enemies), videofondaboss.gfx (props), boss.sfx.
namespace img = bb::assets::sprites::videobossenn;
namespace prop = bb::assets::sprites::videofondaboss;
namespace snd = bb::assets::sounds::boss;

constexpr Flag kStage = Flag::User0;

//-----------------------------------------------------------------------------
// Lara's room
//-----------------------------------------------------------------------------

// Lara (603): never hit (no hitbox); lays out 18 spikes, throws Rayman's
// head at the target player (stage 1), presses a button and draws (2, 3),
// then fires spines at the target player from her bust in 5 angles (4).
// The weight crushes her (stage 7).
class Lara : public Enemy {
public:
    explicit Lara(World& world);
    void update() override;
    void draw() override;

private:
    void stand();  // onAvance()
    void aim();    // the stage 4 part of onAvance()
    void shoot();  // onTire(): the shot's animation
    // A spine from her gun (x - gunX, y - gunY) at the target player
    // (aimed `extraX` pixels right of it).
    void fire(int gunX, int gunY, int extraX);

    int bust_ = img::kLaraBustDraw;  // numero_image_buste
    int attackWait_ = 0;
    bool spikesLaid_ = false;
    bool headThrown_ = false;
};

// A spike (604) of the floor of Lara's room: rises at stages 2 and 3,
// sinks from stage 5, then opens the wall at column 108. Hurts by 5; not
// hit by shots, not counted.
class Spikes : public Enemy {
public:
    explicit Spikes(World& world);
    void update() override;
    void draw() override;
    void hitBy(Shot&) override {}
    int contactDamage() override { return 5; }
    bool countsForKills() override { return false; }

private:
    void spikeHitbox();  // the image's, 25 pixels to the right

    int risen_ = 0;  // y_origine: how far it rose (negative)
};

// The weight (605) hanging above Lara: from stage 3 it drops to y 100;
// shots going right push it 3 pixels each; past x 950 it falls on her,
// throwing gore, sets stage 6 then 7, shakes the screen and is engraved.
class Weight : public Enemy {
public:
    explicit Weight(World& world);
    void update() override;
    void hitBy(Shot& shot) override;
    int contactDamage() override { return 0; }
    bool countsForKills() override { return false; }

private:
    void crush();  // onMeure()
    void splash(int count);

    int pushes_ = 0;  // nb_colision
};

// Rayman's head (thrown by Lara): rolls; landing or shot, it sets stage 2.
class RaymanHead : public Enemy {
public:
    explicit RaymanHead(World& world);
    void update() override;
    void hitBy(Shot& shot) override;
    int contactDamage() override { return 0; }
    bool countsForKills() override { return false; }

private:
    void roll();  // onAvance()
    void die();   // onMeure()
};

// Lara's spine: straight, through walls (traverse).
class LaraSpine : public Projectile {
public:
    LaraSpine(World& world, int speedX, int speedY) : Projectile(world), xSpeed_(speedX) {
        image = enemyImage(img::kLaraSpine);
        dy = speedY;
    }
    void update() override;

private:
    int xSpeed_;
};

//-----------------------------------------------------------------------------
// The plumbers
//-----------------------------------------------------------------------------

// What small Mario and Luigi share: shots push them back ("encaissement",
// the damage of the shots from the left minus those from the right), and
// every hit throws 4 gore drops the way the shot goes.
class Plumber : public Enemy {
public:
    explicit Plumber(World& world) : Enemy(world) {}
    void hitBy(Shot& shot) override;

protected:
    // After a step on the ground: the push changes the speed (towards
    // `topSpeed`, never below 0).
    void groundPush(int topSpeed);
    // The push moves it (20 damage per pixel), not past rightLimit or
    // minX.
    void groundRecoil(int rightLimit);
    // In the air: 15 damage per pixel, and the speed changes (up to twice
    // `topSpeed`).
    void airPush(int topSpeed);
    // Falls with the low gravity of a jump going up (tombe_mario,
    // tombe_luigi: one more pixel per tick every 12 ticks).
    void floatUp();
    // The jump at a target player above and ahead: true if it jumped.
    bool jumpAt(int targetX, int targetY, int delayBase, int delayRange);

    int speed_ = 1;
    int speedStep_ = 0;
    int attackDelay_ = 0;
    int attackWait_ = 0;
    int push_ = 0;  // encaissement
};

// Small Mario (602): walks, charges a player on his platform layer, jumps
// at one above. Killed, he shrinks for 300 ticks and stays as a harmless
// small Mario; Luigi comes (register 3 = 1) and the players hold fire
// until stage 3.
class SmallMario : public Plumber {
public:
    explicit SmallMario(World& world);
    void update() override;
    // He never dies (Luigi takes over): not in the kill percentage (D-55
    // fixed).
    bool countsForKills() override { return false; }

private:
    void walkAbout();  // onAvance()
    void die();        // onMeure()
    void charge();     // onCharge()
    void jump();       // onSaute()
    void endCharge();

    int chargeStep_ = 0;
    int shrinkWait_ = 0;  // nabo_delay
    bool small_ = false;  // nabo
    bool luigiCalled_ = false;
};

// Luigi (600): walks, jumps at a player above, fires volleys of plungers
// (stage 2 when he arrives, attacks from stage 3). Killed, he walks to the
// pit (register 1 = 1), shakes, walks off the screen (2), bleeds for 90
// ticks (3), then the Pac-Man car and Sonic come.
class Luigi : public Plumber {
public:
    explicit Luigi(World& world);
    void update() override;
    void hitBy(Shot& shot) override;

private:
    void walkAbout();  // onAvance()
    void die();        // onMeure()
    void volley();     // onAttack()
    void jump();       // onSaute()

    int volleyStep_ = 0;   // ss_etape_attack
    int volleyFrame_ = 0;  // etape_attack
    int volleyTicks_ = 0;  // nb_tir (the 2002 attack_type 0, a charge, is commented out)
};

// A plunger.
class Plunger : public Projectile {
public:
    Plunger(World& world, int picture, int speedX, int speedY) : Projectile(world), xSpeed_(speedX) {
        image = enemyImage(picture);
        dy = speedY;
    }
    void update() override;

private:
    int xSpeed_;
};

//-----------------------------------------------------------------------------
// Sonic and the car
//-----------------------------------------------------------------------------

// Sonic (601): waits 400 ticks (stage 4), runs; below 9000 HP throws his
// rings three times, then attacks: a horizontal or vertical ball attack or
// lightning, more often as his HP drop. Killed, he runs to x 1600–1630
// (register 2 = 1), turns super saiyan and dies (2).
class Sonic : public Enemy {
public:
    explicit Sonic(World& world);
    void update() override;
    void hitBy(Shot& shot) override;

private:
    void run();         // onAvance()
    void die();         // onMeure()
    void jump();        // onSaute()
    void horizontal();  // onAttackhorizontal()
    void vertical();    // onAttackvertical()
    void lightning();   // onAttackeclair()
    void throwRings();  // onBalancering()
    void runStep();     // the shared run animation and step
    void backToRun();
    // The unused spine counter (ss_etape_attack, etape_attack): it still
    // advances as in 2002, below the given HP.
    void tickSpines(int belowHp);

    int speed_ = 1;
    int speedStep_ = 0;
    int attackDelay_;
    int attackWait_ = 0;
    int attackPhase_ = 0;
    int attackType_ = 0;
    int goalX_ = 0;
    int goalY_ = 0;
    int spineStep_ = 0;
    int spineFrame_ = 0;
    int shots_ = 0;
    int rings_ = 90;
    int wait_ = 0;  // wait_sang_tombe
};

// Sonic's lightning: shakes the screen, strikes after 30 steps.
class SonicLightning : public Projectile {
public:
    explicit SonicLightning(World& world) : Projectile(world) {}
    void update() override;
    void draw() override;
};

// A ring Sonic loses (in the blood-spray list): bounces.
class SonicRing : public Actor {
public:
    SonicRing(World& world, int speedX, int speedY) : Actor(world), xSpeed_(speedX) { dy = speedY; }
    void update() override;

private:
    int xSpeed_;
};

// The Pac-Man car: waits off the screen; once Sonic dies (register 2) it
// drives in and lands (3); 250 ticks later, stage 6. Harmless, never hit,
// never dies (D-55).
class PacManCar : public Enemy {
public:
    explicit PacManCar(World& world);
    void update() override;
    void draw() override;
    void hitBy(Shot&) override {}
    int contactDamage() override { return 0; }

private:
    void drop();  // tombeVoiture(): heavier and faster than the others

    int endWait_ = 0;
};

//-----------------------------------------------------------------------------
// Props
//-----------------------------------------------------------------------------

// The LCD screen (600) and the world map (601): animation tables.
class TableProp : public Actor {
public:
    TableProp(World& world, int first, const int* table, int steps, int delay)
        : Actor(world), first_(first), table_(table), steps_(steps), delay_(delay) {}
    void update() override;

private:
    int first_;
    const int* table_;
    int steps_;
    int delay_;
};

// The electronics (602 to 605): 15 images every 2 ticks, under an overlay
// (none for the fourth).
class Electronics : public Actor {
public:
    Electronics(World& world, int overlay) : Actor(world), overlay_(overlay) {}
    void update() override;
    void draw() override;

private:
    int overlay_;  // < 0: none
};

// The snow screen (606), the DOS screen (610), the fan (611): loops that
// go off screen.
class ScreenLoop : public Actor {
public:
    ScreenLoop(World& world, int first, int steps, int delay)
        : Actor(world), first_(first), steps_(steps), delay_(delay) {}
    void update() override;

private:
    int first_;
    int steps_;
    int delay_;
};

// The camera screen (607): 3 images; a bar scrolls down over it.
class CameraScreen : public Actor {
public:
    explicit CameraScreen(World& world) : Actor(world) {}
    void update() override;
    void draw() override;
};

// The assembler listing (608): scrolls in a 75 by 87 window.
class Assembler : public Actor {
public:
    explicit Assembler(World& world) : Actor(world) {}
    void update() override;
    void draw() override;
};

// The Windows screen (609).
class WindowsScreen : public Actor {
public:
    explicit WindowsScreen(World& world) : Actor(world) {}
    void update() override;
};

}  // namespace bb::game::videoboss
