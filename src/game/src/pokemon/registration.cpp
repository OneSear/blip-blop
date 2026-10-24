#include "bb/game/pokemon_world.hpp"
#include "pokemon/pokemon_world.hpp"

namespace bb::game {

namespace {

using namespace pokemon;

// Enemies get the spawn direction; props keep their own (as in 2002).
template <class T>
void place(T* entity, const SpawnPoint& at, bool facing) {
    entity->x = at.x;
    entity->y = at.y;
    if (facing) {
        entity->direction = at.direction;
    }
}

template <const WalkerSpec& Spec>
Entity* makeWalker(World& world, const SpawnPoint& at) {
    auto* walker = new Walker(world, Spec);
    place(walker, at, true);
    return walker;
}

template <class T>
Entity* makeEnemy(World& world, const SpawnPoint& at) {
    auto* enemy = new T(world);
    place(enemy, at, true);
    return enemy;
}

template <class T>
Entity* makeProp(World& world, const SpawnPoint& at) {
    auto* prop = new T(world);
    place(prop, at, false);
    return prop;
}

template <int First>
Entity* makeAlarm(World& world, const SpawnPoint& at) {
    auto* alarm = new Alarm(world, First);
    place(alarm, at, false);
    return alarm;
}

template <bool LevelTwo>
Entity* makeNeon(World& world, const SpawnPoint& at) {
    auto* sign = new NeonSign(world, LevelTwo);
    place(sign, at, false);
    return sign;
}

template <int SwitchesNeeded>
Entity* makeLift(World& world, const SpawnPoint& at) {
    auto* lift = new Lift(world, SwitchesNeeded);
    place(lift, at, false);
    return lift;
}

void addEnemy(EntityRegistry& r, int id, const char* name, EntityFactory f) {
    r.add({EntityCategory::Enemy, id, name, Placement::Enemies, f});
}

void addProp(EntityRegistry& r, int id, const char* name, EntityFactory f,
             Placement placement = Placement::Props) {
    r.add({EntityCategory::Prop, id, name, placement, f});
}

}  // namespace

void registerPokemonWorld(EntityRegistry& r) {
    const Placement platform = Placement::MovingPlatforms;

    addEnemy(r, 100, "pokemon.pikachu", makeWalker<kPikachu>);
    addEnemy(r, 101, "pokemon.bulbasaur", makeWalker<kBulbasaur>);
    addEnemy(r, 102, "pokemon.charmander", makeWalker<kCharmander>);
    addEnemy(r, 103, "pokemon.com", makeEnemy<Com>);
    addEnemy(r, 104, "pokemon.flying_com", makeEnemy<FlyingCom>);

    addProp(r, 100, "pokemon.alarm_1", makeAlarm<prop::kAlarm1>);
    addProp(r, 101, "pokemon.alarm_2", makeAlarm<prop::kAlarm2>);
    addProp(r, 102, "pokemon.alarm_3", makeAlarm<prop::kAlarm3>);
    addProp(r, 103, "pokemon.arrow", makeProp<Arrow>);
    addProp(r, 104, "pokemon.sky", makeProp<SkyWindow>);
    addProp(r, 105, "pokemon.lift", makeLift<0>, platform);
    addProp(r, 106, "pokemon.switch", makeProp<Switch>, platform);
    addProp(r, 107, "pokemon.falling_crate", makeProp<LockedCrate>, platform);
    addProp(r, 108, "pokemon.lift_2", makeLift<1>, platform);
    addProp(r, 109, "pokemon.bomb", makeProp<Bomb>, platform);
    addProp(r, 110, "pokemon.neon_level_2", makeNeon<true>);
    addProp(r, 111, "pokemon.neon_level_3", makeNeon<false>);
    // A second id for the bomb, as a plain prop; no shipped level uses it.
    addProp(r, 900, "pokemon.bomb_prop", makeProp<Bomb>);
}

}  // namespace bb::game
