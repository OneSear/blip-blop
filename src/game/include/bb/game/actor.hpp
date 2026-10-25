#pragma once

#include "bb/game/entity.hpp"
#include "bb/game/world.hpp"

namespace bb::game {

// Facing, as stored by the 2002 code (SENS_GAUCHE, SENS_DROITE).
constexpr int kFacingLeft = 0;
constexpr int kFacingRight = 1;

// Gravity: the fall speed grows by one every kGravityDelay ticks, up to
// kMaxFallSpeed pixels per tick (LATENCE_GRAVITE, GRAVITE_MAX).
constexpr int kGravityDelay = 7;
constexpr int kMaxFallSpeed = 4;

// An entity of the ported framework: an image, an animation and gravity,
// in a World (the 2002 Sprite).
class Actor : public Entity {
public:
    explicit Actor(World& world) noexcept : world_(world) {}

    void draw() override;
    EntityState state() const override;

    int dy = 0;                    // vertical speed
    AnimationPlayer animation;     // etape, ss_etape
    int direction = 0;             // facing or one of 8 directions (dir)
    int gravityDelay = 0;          // lat_grav
    const Image* image = nullptr;  // pic

protected:
    World& world() const noexcept { return world_; }

    // Shortcuts to the world.
    const Image* enemyImage(int index) const { return world_.media.enemies->image(index); }
    const Image* levelImage(int index) const { return world_.media.level->image(index); }
    bool solidWall(int px, int py) const { return world_.collision.solidWall(px, py); }
    int platformAt(int px, int py) const { return world_.collision.platformAt(px, py); }
    // Draws an image with its hotspot at level position (px, py).
    void drawAt(int px, int py, const Image* picture) const {
        world_.media.canvas->draw(picture, px - world_.camera.x, py);
    }

    // Falls with gravity, landing on platforms (tombe()).
    void fall();
    // Same, but lands only when not already on a platform (tombe2()).
    void fallThrough();
    // Walks dx pixels, following platforms up and down 5 pixels (marche()).
    void walk(int dx);
    // The hitbox is the current image (colFromPic()).
    void hitboxFromImage();
    void noHitbox() noexcept { hitbox.active = false; }
    // Dies when far outside the screen (updateADetruire()).
    void dieWhenOffscreen();
    // Dies once fallen below the screen, where no platform is: for the
    // debris that end only when they land (D-55 fixed: in 2002 a piece
    // that fell into a hole fell for the rest of the level). Nothing that
    // has fallen there lands again, so this ends no landing.
    void dieWhenFallenOut();

private:
    World& world_;
};

// A prop that players and enemies stand on and that moves: it rewrites its
// platform heights in the collision map every tick (the 2002 moving
// platforms, kept in World::movingPlatforms).
class MovingPlatform : public Actor {
public:
    explicit MovingPlatform(World& world) noexcept : Actor(world) {}

protected:
    // Its top over [from, to) on platform layer `layer` (4 for most, 3 for
    // the swinging crate platform).
    void setPlatform(int layer, int from, int to, int height) {
        world().collision.setPlatform(layer, from, to, height);
    }
    // Moves by `step` the players standing on its top (height `top`)
    // between x and x + width (the 2002 lifts; nothing else is carried).
    void carryPlayers(int width, int top, int step);
};

// The states of characters, numbered as in 2002 (ETAT_*): the world-state
// fingerprint records them.
enum CharacterState {
    kWalking = 0,     // ETAT_AVANCE
    kJumping = 1,     // ETAT_SAUTE
    kFalling = 2,     // ETAT_TOMBE
    kShooting = 3,    // ETAT_TIRE
    kDying = 4,       // ETAT_MEURE
    kWaiting = 5,     // ETAT_ATTEND
    kNormal = 6,      // ETAT_NORMAL
    kCarbonised = 7,  // ETAT_CARBONISE
};

// An entity with no behaviour, standing in a normal state: a point or a box
// the game tests against (the target when no player is left, the box of a
// decal; the 2002 code used a Personnage and a FondStatique).
class Marker : public Entity {
public:
    void update() override {}
    void draw() override {}
    EntityState state() const override {
        EntityState s;
        s.state = kNormal;
        return s;
    }
};

// An actor with hit points and a state (Personnage).
class Character : public Actor {
public:
    explicit Character(World& world) noexcept : Actor(world) {}

    EntityState state() const override;

    Health health;       // pv
    int mode = kNormal;  // a CharacterState, or a class's own states (etat)
};

// An enemy (Ennemi): hit by the players' shots, with the shared death
// pipeline (blood on walls, weapon pickups, the kill counter).
class Enemy : public Character, public ShotTarget {
public:
    explicit Enemy(World& world) noexcept;

    ShotTarget* asShotTarget() override { return this; }

    // ShotTarget (estTouche()).
    void hitBy(Shot& shot) override;
    int contactDamage() override { return 1; }
    bool countsForKills() override { return true; }

    int minX;          // does not walk left of this (the camera at birth)
    int bleeding = 0;  // ticks before blood can spray again (blood)
    int treasure = 5;  // weight towards the next weapon pickup (tresor)

protected:
    // A blood spray in the shot's direction, offset by
    // offsetX/offsetY[direction], unless one is running or the shot burns
    // (gicle()).
    void sprayBlood(const Shot& shot, const int* offsetX, const int* offsetY);
    // The speeds of a shot from (fromX, fromY) that lands on (toX, toY)
    // (tirEnCloche()).
    void lob(int fromX, int fromY, int toX, int toY, int& xSpeed, int& ySpeed) const;
    // The burnt-to-ashes animation shared by many enemies (onCarbonise()):
    // `steps` enemy images from firstLeft or firstRight, one every `delay`
    // ticks, then the enemy is dead.
    void burn(int firstLeft, int firstRight, int steps, int delay = 6);
};

// A projectile (Tir).
class Projectile : public Actor, public Shot {
public:
    explicit Projectile(World& world) noexcept : Actor(world) {}

    Shot* asShot() override { return this; }

    int damage() const override { return 1; }
    int burning() const override { return 0; }
    int bloodDirection() const override { return direction / 2; }
    int heading() const override { return direction; }
    void onHit(int) override {
        dead = true;
        hitbox.active = false;
    }
    void onKill() override {}
};

}  // namespace bb::game
