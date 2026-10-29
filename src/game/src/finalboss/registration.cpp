#include "bb/game/finalboss_world.hpp"
#include "finalboss/finalboss_world.hpp"

namespace bb::game {

namespace {

namespace fb = finalboss;

template <class T>
Entity* make(World& world, const SpawnPoint& at) {
    auto* entity = new T(world);
    entity->x = at.x;
    entity->y = at.y;
    entity->direction = at.direction;
    return entity;
}

template <class T>
Entity* place(World& world, const SpawnPoint& at) {
    auto* entity = new T(world);
    entity->x = at.x;
    entity->y = at.y;
    return entity;
}

template <const fb::FloaterSpec& kSpec>
Entity* makeFloater(World& world, const SpawnPoint& at) {
    auto* entity = new fb::Floater(world, kSpec);
    entity->x = at.x;
    entity->y = at.y;
    return entity;
}

}  // namespace

void registerFinalBossWorld(EntityRegistry& registry) {
    registry.add({EntityCategory::Enemy, 700, "final.mario", Placement::Enemies, make<fb::Mario>});
    registry.add({EntityCategory::Vehicle, 0, "final.stork", Placement::Vehicles, make<fb::Stork>});
    registry.add(
        {EntityCategory::Prop, 700, "final.battle_cruiser", Placement::Props, place<fb::BattleCruiser>});
    registry.add(
        {EntityCategory::Prop, 701, "final.hyperspace_rock", Placement::Props, place<fb::HyperspaceRock>});
    registry.add({EntityCategory::Prop, 702, "final.monolith", Placement::MovingPlatforms,
                  makeFloater<fb::Floater::kMonolith>});
    registry.add({EntityCategory::Prop, 703, "final.rock_1", Placement::MovingPlatforms,
                  makeFloater<fb::Floater::kRock1>});
    registry.add({EntityCategory::Prop, 704, "final.rock_2", Placement::MovingPlatforms,
                  makeFloater<fb::Floater::kRock2>});
}

}  // namespace bb::game
