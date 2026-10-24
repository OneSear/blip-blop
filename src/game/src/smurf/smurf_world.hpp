#pragma once

// The Smurf world (levels snuf1 and snuf2): its enemies and their shots,
// ported from the 2002 classes (EnnemiSmurf*, TirGateau, TirCadeau,
// TirHadoken, TirBisouMortel, TirSmurfSauvage). The arithmetic, the order
// of the random draws and the states are the 2002 ones, so that the
// replays give the same fingerprints. Image and sound numbers are indices
// in the level's enemy bank and sound bank.

#include "bb/assets/sounds/misc.hpp"
#include "bb/assets/sounds/smurf.hpp"
#include "bb/assets/sprites/snufenn.hpp"
#include "bb/game/actor.hpp"

namespace bb::game::smurf {

// The Smurf world's banks: snufenn.gfx (enemies), smurf.sfx (level sounds).
namespace img = bb::assets::sprites::snufenn;
namespace snd = bb::assets::sounds::smurf;
namespace miscsnd = bb::assets::sounds::misc;

// snuf2's script registers.
constexpr Flag kFightStage = Flag::User0;  // set: the strongmen fight; 3: the Smurfette flew off
constexpr Flag kBossKills = Flag::User1;   // strongmen and Big Smurf killed (6: all)
constexpr Flag kFightMusic = Flag::User2;  // the fight music started

// The steps of the Smurfs' walk cycles, from their first image.
constexpr int kWalkCycle[] = {0, 1, 2, 3, 4, 3, 2, 1};
constexpr int kWalkSteps = 8;

// The basic Smurf (enemy 0) and its five skins: walks between walls and
// the left of the screen at its birth, jumps now and then.
class Smurf : public Enemy {
public:
    enum class Skin {
        Plain,   // EnnemiSmurf
        Brainy,  // EnnemiSmurfALunettes: glasses, jumps further
        Vain,    // EnnemiSmurfCoquet: flower, jumps higher, speed 1 or 2
        Chilly,  // EnnemiSmurfFrileux: scarf and hat, 75 HP, speed 1 to 3
        Lazy,    // EnnemiSmurfParesseux: jumps lower
        Peon,    // EnnemiSmurfPeon: helmet, 125 HP
    };

    Smurf(World& world, Skin skin);

    void update() override;
    void draw() override;
    void hitBy(Shot& shot) override;

private:
    void walkAbout();  // onAvance()
    void jump();       // onSaute()
    void die();        // onMeure()

    Skin skin_;
    int jumpWait_ = 0;
    int jumpDelay_ = 0;
    int jumpSpeed_ = -5;
    int speed_ = 1;
};

// The greedy Smurf (1): stands still and lobs cakes at the target player.
class GreedySmurf : public Enemy {
public:
    explicit GreedySmurf(World& world);

    void update() override;
    void hitBy(Shot& shot) override;

private:
    static constexpr int kHolding = 15;  // after a throw (ETAT_HOLD)

    void stand();      // onNormal()
    void throwCake();  // onTire()
    void hold();
    void die();

    int shotWait_ = 0;
    int shotDelay_ = 0;
    int aimX_ = 0;
    int aimY_ = 0;
};

// The prankster Smurf (2): carries a gift, puts it down (it explodes),
// takes out another one. Shot while carrying one, it burns.
class PranksterSmurf : public Enemy {
public:
    explicit PranksterSmurf(World& world);

    void update() override;
    void draw() override;
    void hitBy(Shot& shot) override;

private:
    static constexpr int kSpeed = 2;
    static constexpr int kTakingOut = 11;  // ETAT_DEGAINE

    void walkAbout();    // onAvance()
    void putGiftDown();  // onPoseCadeau(), in state kWaiting
    void takeOutGift();  // onDegaine()
    void die();

    bool carrying_ = true;
    int newGiftDelay_ = 0;
    int newGiftWait_ = 0;
    int giftDelay_ = 0;
    int giftWait_ = 0;
    int giftHeight_ = 5;  // the gift goes down to the ground before it is put
};

// The wild Smurf (3): comes down on a rope at a random place of the screen,
// fires five shots at the target player, climbs back and leaves.
class WildSmurf : public Enemy {
public:
    explicit WildSmurf(World& world);

    void update() override;
    void draw() override;
    void hitBy(Shot& shot) override;

    // Aim directions, from left to right (shot speeds -4,4 / -1,5 / 0,6).
    enum Aim {
        DiagonalLeft = 0,   // DIAGONALE_GAUCHE
        SteepLeft = 1,      // DIAGONALE_BAS_GAUCHE
        DownLeft = 2,       // BAS_GAUCHE
        DownRight = 3,      // BAS_DROITE
        SteepRight = 4,     // DIAGONALE_BAS_DROITE
        DiagonalRight = 5,  // DIAGONALE_DROITE
    };

private:
    // The ticks hidden before coming down, this Smurf's own (D-78 fixed:
    // 2002 static members shared by every wild Smurf).
    int hideDelay_ = 0;
    int hideTicks_ = 0;
    static constexpr int kHidden = 100;    // ETAT_ATTENDS
    static constexpr int kClimbing = 102;  // ETAT_MONTE

    void comeDown();  // onTombe()
    void shoot();     // onTire()
    void die();       // onMeure()
    void burn();      // onCarbonise()

    int aim_ = DownLeft;
    int ropeEnd_ = 0;  // the height it comes down to (hauteur)
    int slope_ = 0;    // towards the target, times 10 (pente)
    int firstImage_ = 0;
    int shotStep_ = 0;
    int shots_ = 0;
    bool firstUpdate_ = true;
    bool shooting_ = false;
    Handle<> target_;
};

// The farmer Smurf (4): charges with his scythe, turning at walls. Killed,
// his head flies off and the body runs on.
class FarmerSmurf : public Enemy {
public:
    explicit FarmerSmurf(World& world);

    void update() override;
    void draw() override;
    void hitBy(Shot& shot) override;

private:
    static constexpr int kSpeed = 4;

    void run();       // onAvance()
    void loseHead();  // onMeure()

    int headSubFrame_ = 0;
    int headFrame_ = 0;
    int headX_ = 0;
    int headY_ = 0;
};

// The strongman Smurf (5), mini-boss of snuf2: walks, then throws a
// fireball, jumps with an uppercut or spins at the target player. Level
// script registers: 0 starts his fight (music, 40 s timer, flag 2); each
// strongman killed adds one to register 1.
class StrongmanSmurf : public Enemy {
public:
    explicit StrongmanSmurf(World& world);

    void update() override;
    void hitBy(Shot& shot) override;

private:
    static constexpr int kFireball = 1000;  // ETAT_TIRE_HADOKEN
    static constexpr int kUppercut = 1001;  // ETAT_TIRE_DPUNCH
    static constexpr int kSpinKick = 1002;  // ETAT_TIRE_HKICK

    void walkAbout();  // onAvance()
    void attack();     // onTire()
    void fireball();
    void uppercut();
    void spinKick();
    void die();

    int attackWait_ = 0;
    int attackDelay_ = 0;
    int xSpeed_ = 0;
    int uppercutSpeed_ = 0;
};

// Smurfette (6): walks without leaving her platform and blows homing kisses.
class Smurfette : public Enemy {
public:
    explicit Smurfette(World& world);

    void update() override;
    void hitBy(Shot& shot) override;

private:
    void walkAbout();
    void blowKiss();
    void die();

    int shotWait_ = 0;
    int shotDelay_ = 0;
};

// The stork (7) of snuf2's ending: flies away along a sine once level
// register 1 reaches 6 (5 strongmen and the Big Smurf; 5 in 2002), and sets
// register 0 to 3 when it leaves the screen.
// Cannot be hit; counts as an enemy created.
class Stork : public Enemy {
public:
    explicit Stork(World& world);

    void update() override;
    // It cannot be hit and never dies: not an enemy of the kill percentage
    // (D-77 fixed: 100 % was out of reach in snuf2).
    bool countsForKills() override { return false; }

private:
    int baseX_ = 0;
    int baseY_ = 0;
    bool firstUpdate_ = true;
};

// The Big Smurf (8), Papa Smurf, boss of snuf2's ending (finished in 2026:
// the 2002 class was a skeleton without art, death or second attack). He
// waits off screen until the five strongmen are dead (register 1 = 5),
// walks in, then alternates two attacks: he runs at the target player and
// hits in a flurry, or keeps his distance and lobs exploding gifts. Killed,
// he adds one to register 1, which sends the stork off.
class BigSmurf : public Enemy {
public:
    explicit BigSmurf(World& world);

    void update() override;
    void hitBy(Shot& shot) override;
    int contactDamage() override { return mode == kFlurry ? 2 : 1; }

private:
    static constexpr int kWaitingOffscreen = 100;
    static constexpr int kFlurry = 101;    // ATT_ATATA
    static constexpr int kThrowing = 102;  // ATT_BOOM, finished

    enum class Plan { Flurry, Gifts };

    void updateTarget();  // updateCible()
    void choose();        // updateChoice()
    void walkAbout();     // onAvance()
    void flurry();        // onAtata()
    void throwGift();
    void die();
    void moveBy(int step);
    void scriptKill();

    Handle<> target_;
    int targetWait_ = 0;
    int xSpeed_ = 0;
    Plan plan_ = Plan::Gifts;
    int giftsThrown_ = 0;
    int throwWait_ = 0;
    bool counted_ = false;
};

// The Big Smurf's gift: lobbed, it explodes where it lands.
class LobbedGift : public Projectile {
public:
    explicit LobbedGift(World& world);
    void update() override;

    int xSpeed = 0;

private:
    bool exploding_ = false;
};

// The greedy Smurf's cake, lobbed.
class Cake : public Projectile {
public:
    explicit Cake(World& world);
    void update() override;

    int xSpeed = 0;
};

// The prankster's gift: falls, waits, explodes.
class Gift : public Projectile {
public:
    explicit Gift(World& world);
    void update() override;
};

// The strongman's fireball: flies straight, blinking.
class Fireball : public Projectile {
public:
    explicit Fireball(World& world) : Projectile(world) {}
    void update() override;
};

// Smurfette's kiss: homes on a player for 235 ticks.
class DeadlyKiss : public Projectile {
public:
    explicit DeadlyKiss(World& world) : Projectile(world) {}
    void update() override;

    Handle<> target;
    int xSpeed = 0;  // in 1/13 pixel per tick, as dy
    int lifetime = 235;
};

// The wild Smurf's shot. Its speed is its own: the 2002 class declared dx
// and dy members that hid Sprite::dy, so the fingerprinted dy stays 0.
class WildSmurfShot : public Projectile {
public:
    explicit WildSmurfShot(World& world);
    void update() override;

    int xSpeed = 0;
    int ySpeed = 0;
};

}  // namespace bb::game::smurf
