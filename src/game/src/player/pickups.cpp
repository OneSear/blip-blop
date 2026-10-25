// The pickups (bonus.h, bonus_pm.h, bonus_fusil.h, bonus_laser.h,
// bonus_lf.h, bonus_beer.h, bonus_tonneau.h, bonus_invincible.h,
// bonus_vache.h).

#include "bb/assets/sounds/misc.hpp"
#include "bb/assets/sprites/misc.hpp"
#include "bb/game/player.hpp"
#include "bb/game/trig.hpp"

namespace bb::game {

namespace {

namespace img = bb::assets::sprites::misc;
namespace snd = bb::assets::sounds::misc;

// The pickup ids of the level files (EventBonus).
enum PickupId {
    kSmg = 0,
    kShotgun = 1,
    kLaser = 2,
    kBeer = 3,
    kInvincibility = 4,
    kBarrel = 5,
    kCowBomb = 6,
    kFlamethrower = 7
};

constexpr int kFullHealth = 5;
constexpr int kSparkleDistance = 25;  // DIST_BITONIO

// A pickup lying on the ground, with four sparkles that pulse around it.
class Pickup : public Actor, public Collectable {
public:
    Pickup(World& world, int picture) : Actor(world) {
        image = world.media.misc->image(picture);
        hitbox.active = true;
    }

    Collectable* asCollectable() override { return this; }

    void update() override {
        fall();
        phase_ += 12;
        phase_ %= 360;

        animation.frame += 1;
        animation.frame %= 10;
        if (animation.frame == 0) {
            sparkle_ = !sparkle_;
        }

        if (x < world().camera.x - 50) {
            dead = true;
        }

        // Drawn 15 pixels up, with a wider hitbox.
        y -= 15;
        hitboxFromImage();
        hitbox.x1 -= 10;
        hitbox.y1 -= 10;
        hitbox.x2 += 10;
        hitbox.y2 += 10;
        y += 15;
    }

    void draw() override {
        const ImageBank& misc = *world().media.misc;
        const int top = y - 15;
        const int base = sparkle_ ? img::kPickupSparkle : img::kPickupSparkle + 4;
        const int d = (8 * sine1024(phase_)) >> kTrigShift;
        const int far = kSparkleDistance + d;

        drawAt(x, top, image);
        drawAt(x - far, top - far, misc.image(base + 3));
        drawAt(x + far, top - far, misc.image(base + 2));
        drawAt(x + far, top + far, misc.image(base));
        drawAt(x - far, top + far, misc.image(base + 1));
    }

    void takenBy(Entity& player) override { take(static_cast<Player&>(player)); }

protected:
    virtual void take(Player& player) = 0;

    void taken() {
        dead = true;
        hitbox.active = false;
    }

    void play(int sound) const { world().media.miscSounds->play(sound); }

private:
    int phase_ = 0;
    bool sparkle_ = false;
};

// A weapon with `rounds` of ammo, or more ammo up to `most` for the same
// weapon. The SMG and the shotgun are refused while a well-loaded laser or
// flamethrower is in hand.
struct WeaponPickupSpec {
    int picture;
    int sound;
    Weapon weapon;
    int rounds;
    int most;
    bool refusedForSuperWeapon;
};

const WeaponPickupSpec kSmgPickup{img::kWeaponSmg, snd::kPickupSmg, Weapon::Smg, 200, 400, true};
const WeaponPickupSpec kShotgunPickup{
    img::kWeaponShotgun, snd::kPickupShotgun, Weapon::Shotgun, 20, 40, true};
const WeaponPickupSpec kLaserPickup{img::kWeaponLaser, snd::kPickupLaser, Weapon::Laser, 750, 1500, false};
const WeaponPickupSpec kFlamethrowerPickup{
    img::kWeaponFlamethrower, snd::kPickupFlamethrower, Weapon::Flamethrower, 1000, 2000, false};

class WeaponPickup : public Pickup {
public:
    WeaponPickup(World& world, const WeaponPickupSpec& spec) : Pickup(world, spec.picture), spec_(spec) {}

protected:
    void take(Player& player) override {
        const bool superWeapon =
            (player.weapon == Weapon::Flamethrower || player.weapon == Weapon::Laser) && player.ammo >= 100;
        if (spec_.refusedForSuperWeapon && superWeapon) {
            return;
        }

        play(spec_.sound);
        if (player.weapon == spec_.weapon) {
            player.ammo += spec_.rounds;
            if (player.ammo > spec_.most) {
                player.ammo = spec_.most;
            }
        } else {
            player.equip(spec_.weapon, spec_.rounds);
        }
        taken();
    }

private:
    const WeaponPickupSpec& spec_;
};

// Two hit points, refused at full health.
class Beer : public Pickup {
public:
    explicit Beer(World& world) : Pickup(world, img::kPickupBeer) {}

protected:
    void take(Player& player) override {
        if (player.health.points == kFullHealth) {
            return;
        }
        play(snd::kPickupHealth);
        player.health.heal(2, kFullHealth);
        taken();
    }
};

// Full health.
class Barrel : public Pickup {
public:
    explicit Barrel(World& world) : Pickup(world, img::kPickupBarrel) {}

protected:
    void take(Player& player) override {
        play(snd::kPickupHealth);
        player.health.points = kFullHealth;
        taken();
    }
};

class Invincibility : public Pickup {
public:
    explicit Invincibility(World& world) : Pickup(world, img::kPickupInvincibility) {}

protected:
    void take(Player& player) override {
        player.invincible = 550;
        taken();
    }
};

class CowBomb : public Pickup {
public:
    explicit CowBomb(World& world) : Pickup(world, img::kPickupCowBomb) {}

protected:
    void take(Player& player) override {
        play(snd::kPickupCowBomb);
        player.cowBombs += 1;
        taken();
    }
};

template <const WeaponPickupSpec& Spec>
Entity* weaponPickup(World& world, const SpawnPoint& at) {
    auto* pickup = new WeaponPickup(world, Spec);
    pickup->x = at.x;
    pickup->y = at.y;
    return pickup;
}

template <class T>
Entity* pickup(World& world, const SpawnPoint& at) {
    auto* p = new T(world);
    p->x = at.x;
    p->y = at.y;
    return p;
}

void add(EntityRegistry& registry, int id, const char* name, EntityFactory factory) {
    registry.add({EntityCategory::Pickup, id, name, Placement::Pickups, factory});
}

}  // namespace

void registerPickups(EntityRegistry& registry) {
    add(registry, kSmg, "pickup.smg", weaponPickup<kSmgPickup>);
    add(registry, kShotgun, "pickup.shotgun", weaponPickup<kShotgunPickup>);
    add(registry, kLaser, "pickup.laser", weaponPickup<kLaserPickup>);
    add(registry, kBeer, "pickup.beer", pickup<Beer>);
    add(registry, kInvincibility, "pickup.invincibility", pickup<Invincibility>);
    add(registry, kBarrel, "pickup.barrel", pickup<Barrel>);
    add(registry, kCowBomb, "pickup.cow_bomb", pickup<CowBomb>);
    add(registry, kFlamethrower, "pickup.flamethrower", weaponPickup<kFlamethrowerPickup>);
}

}  // namespace bb::game
