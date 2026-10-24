#include "bb/game/carebears_world.hpp"
#include "carebears/carebears_world.hpp"

namespace bb::game {

namespace {

template <class T>
Entity* make(World& world, const SpawnPoint& at) {
    auto* enemy = new T(world);
    enemy->x = at.x;
    enemy->y = at.y;
    enemy->direction = at.direction;
    return enemy;
}

void addEnemy(EntityRegistry& registry, int id, const char* name, EntityFactory factory) {
    registry.add({EntityCategory::Enemy, id, name, Placement::Enemies, factory});
}

}  // namespace

void registerCareBearWorld(EntityRegistry& registry) {
    addEnemy(registry, 200, "carebear.sky", make<carebears::SkyBear>);
    addEnemy(registry, 201, "carebear.star", make<carebears::StarBear>);
    addEnemy(registry, 202, "carebear.boom", make<carebears::BoomBear>);
    addEnemy(registry, 203, "carebear.cloud", make<carebears::CloudBear>);
    addEnemy(registry, 204, "carebear.zombie", make<carebears::ZombieBear>);
    addEnemy(registry, 205, "carebear.shaman", make<carebears::Shaman>);
    addEnemy(registry, 207, "carebear.jedi", make<carebears::Jedi>);
    addEnemy(registry, 208, "carebear.jedi_hero", make<carebears::JediHero>);
    addEnemy(registry, 209, "carebear.snork_in_black", make<carebears::SnorkCameo>);
}

}  // namespace bb::game
