#include "bb/game/smurf_world.hpp"
#include "smurf/smurf_world.hpp"

namespace bb::game {

namespace {

using smurf::Smurf;

void place(Actor& actor, const SpawnPoint& at) {
    actor.x = at.x;
    actor.y = at.y;
    actor.direction = at.direction;
}

template <class T>
Entity* make(World& world, const SpawnPoint& at) {
    auto* enemy = new T(world);
    place(*enemy, at);
    return enemy;
}

// Enemy 0 draws one of the six skins.
Entity* makeSmurf(World& world, const SpawnPoint& at) {
    static const Smurf::Skin kSkins[] = {Smurf::Skin::Plain,  Smurf::Skin::Brainy, Smurf::Skin::Vain,
                                         Smurf::Skin::Chilly, Smurf::Skin::Lazy,   Smurf::Skin::Peon};
    auto* enemy = new Smurf(world, kSkins[world.random() % 6]);
    place(*enemy, at);
    return enemy;
}

void addEnemy(EntityRegistry& registry, int id, const char* name, EntityFactory factory) {
    registry.add({EntityCategory::Enemy, id, name, Placement::Enemies, factory});
}

}  // namespace

void registerSmurfWorld(EntityRegistry& registry) {
    addEnemy(registry, 0, "smurf.smurf", makeSmurf);
    addEnemy(registry, 1, "smurf.greedy", make<smurf::GreedySmurf>);
    addEnemy(registry, 2, "smurf.prankster", make<smurf::PranksterSmurf>);
    addEnemy(registry, 3, "smurf.wild", make<smurf::WildSmurf>);
    addEnemy(registry, 4, "smurf.farmer", make<smurf::FarmerSmurf>);
    addEnemy(registry, 5, "smurf.strongman", make<smurf::StrongmanSmurf>);
    addEnemy(registry, 6, "smurf.smurfette", make<smurf::Smurfette>);
    addEnemy(registry, 7, "smurf.stork", make<smurf::Stork>);
    addEnemy(registry, 8, "smurf.big_smurf", make<smurf::BigSmurf>);
}

}  // namespace bb::game
