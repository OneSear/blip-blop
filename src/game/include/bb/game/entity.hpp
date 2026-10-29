#pragma once

#include <climits>
#include <memory>

#include "bb/game/components.hpp"

namespace bb::game {

// The part of an entity's state that is not in Entity itself, for the
// world-state fingerprint of deterministic runs: the animation frame and
// sub-frame, the direction, the vertical speed, and for characters the
// hit points and the state.
struct EntityState {
    int dy = 0;
    int frame = 0;
    int subFrame = 0;
    int direction = 0;
    int hitPoints = 0;
    int state = 0;
};

class Entity;

// The roles that let old and ported entities act on each other. They hold
// no data; an entity exposes the roles it has through Entity::asShot()
// and the like.

// A projectile, as seen by what it hits.
class Shot {
public:
    virtual ~Shot() = default;
    virtual int damage() const = 0;
    // Non-zero: the victim does not bleed; 2: it is carbonised.
    virtual int burning() const = 0;
    // The direction of the blood spray (0..7).
    virtual int bloodDirection() const = 0;
    // Its 2002 direction (dir): for the players' shots 0..15, 0 down,
    // 4 right, 8 up, 12 left (some enemies push or recoil by it).
    virtual int heading() const = 0;
    // The shot hit something and did damageDone; it usually disappears.
    virtual void onHit(int damageDone) = 0;
    // The shot killed what it hit.
    virtual void onKill() = 0;
};

// What the players' shots hurt: enemies.
class ShotTarget {
public:
    virtual ~ShotTarget() = default;
    virtual void hitBy(Shot& shot) = 0;
    // The damage done to a player who touches it.
    virtual int contactDamage() = 0;
    // Whether it counts in the kill statistics (not props hit like enemies).
    virtual bool countsForKills() = 0;
};

// What enemies and their shots hurt: players.
class Hurtable {
public:
    virtual ~Hurtable() = default;
    virtual void hurt(int damage) = 0;
};

// A player as a vehicle sees it.
class Rider {
public:
    virtual ~Rider() = default;
    // Free to board: alive, not coming back, not riding.
    virtual bool canBoard() const = 0;
    virtual bool riding() const = 0;
    // Boards (lockVehicule()) and leaves (unlockVehicule()).
    virtual void board(bool canFire, bool canTurn) = 0;
    virtual void leave() = 0;
    // Faces and aims right.
    virtual void faceRight() = 0;
};

// What players pick up.
class Collectable {
public:
    virtual ~Collectable() = default;
    virtual void takenBy(Entity& player) = 0;
};

// The base of everything that lives in the world's entity lists: players,
// enemies, projectiles, pickups, props, effects (migration.md, section
// 3.4). What the world needs of every entity is here; the rest is in the
// roles above and the bases of actor.hpp.
class Entity {
public:
    Entity() = default;
    // A copy is another entity: handles to the original do not see it.
    Entity(const Entity& other) { copyFields(other); }
    Entity& operator=(const Entity& other) {
        if (this != &other) {
            copyFields(other);
        }
        return *this;
    }
    virtual ~Entity() {
        if (life_) {
            *life_ = nullptr;
        }
    }

    // One tick of simulation.
    virtual void update() = 0;
    // Draws the entity at its place on screen.
    virtual void draw() = 0;
    // See EntityState.
    virtual EntityState state() const = 0;

    // Whether this entity's hitbox touches other's (the laser has a
    // diagonal beam).
    virtual bool hits(const Entity& other) const { return overlaps(other); }

    // The roles of this entity (null when it does not have them).
    virtual Shot* asShot() { return nullptr; }
    virtual ShotTarget* asShotTarget() { return nullptr; }
    virtual Hurtable* asHurtable() { return nullptr; }
    virtual Rider* asRider() { return nullptr; }
    virtual Collectable* asCollectable() { return nullptr; }

    // Position in level coordinates (pixels): the hotspot of the current
    // image; y grows downwards.
    int x = 0;
    int y = 0;
    // Where it was at the start of the tick (World::rememberPositions()),
    // to draw it between two ticks (World::Motion); kNowhere for an entity
    // created since. Not simulation state: nothing reads it but drawing.
    static constexpr int kNowhere = INT_MIN;
    int previousX = kNowhere;
    int previousY = kNowhere;

    // What it hits and is hit with.
    Hitbox hitbox;

    // Set when the entity is finished: the world drops it at the end of
    // the tick.
    bool dead = false;

    // Ticks until the next bubble in underwater levels (Flag::Bubbles).
    int bubbleDelay = 0;

    // Whether the two hitboxes overlap.
    bool overlaps(const Entity& other) const noexcept { return hitbox.overlaps(other.hitbox); }

    // Shared with the handles to this entity; cleared by the destructor.
    const std::shared_ptr<Entity*>& lifeToken() {
        if (!life_) {
            life_ = std::make_shared<Entity*>(this);
        }
        return life_;
    }

private:
    void copyFields(const Entity& other) noexcept {
        x = other.x;
        y = other.y;
        hitbox = other.hitbox;
        dead = other.dead;
        bubbleDelay = other.bubbleDelay;
    }

    std::shared_ptr<Entity*> life_;
};

// A reference to another entity that becomes null when that entity is
// destroyed (migration.md D-04): the 2002 code kept raw pointers, and the
// Princess's guards read her after she was freed. A null handle means the
// entity is gone; an entity still in its list may be dead (removed at the
// end of the tick), as before.
template <class T = Entity>
class Handle {
public:
    Handle() = default;
    Handle(T* entity) {  // NOLINT(google-explicit-constructor): replaces raw pointers
        if (entity != nullptr) {
            life_ = entity->lifeToken();
        }
    }

    T* get() const noexcept { return life_ && *life_ != nullptr ? static_cast<T*>(*life_) : nullptr; }
    T* operator->() const noexcept { return get(); }
    explicit operator bool() const noexcept { return get() != nullptr; }
    void reset() noexcept { life_.reset(); }

private:
    std::shared_ptr<Entity*> life_;
};

// The non-entity things the level runs alongside them.

// Something that spawns enemies or pickups over time.
class Generator {
public:
    virtual ~Generator() = default;
    // One tick.
    virtual void update() = 0;
    // True when the generator has done its job and leaves the level.
    virtual bool finished() = 0;
};

}  // namespace bb::game
