#include "bb/game/snorks_world.hpp"
#include "snorks/snorks_world.hpp"

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

void registerSnorkWorld(EntityRegistry& registry) {
    addEnemy(registry, 300, "snork.grunt", make<snorks::Grunt>);
    addEnemy(registry, 301, "snork.gunner", make<snorks::Gunner>);
    addEnemy(registry, 302, "snork.governor", make<snorks::Governor>);
    addEnemy(registry, 303, "snork.mage", make<snorks::Mage>);
    addEnemy(registry, 304, "snork.seahorse_rider", make<snorks::SeahorseRider>);
    addEnemy(registry, 305, "snork.snork_in_black", make<snorks::SnorkInBlack>);
    addEnemy(registry, 307, "snork.snork_in_black_boss", make<snorks::SnorkBoss>);
}

}  // namespace bb::game
