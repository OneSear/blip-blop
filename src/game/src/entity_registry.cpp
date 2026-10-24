#include "bb/game/entity_registry.hpp"

#include <stdexcept>
#include <utility>

#include "bb/game/world.hpp"

namespace bb::game {

void EntityRegistry::add(EntityType type) {
    if (type.create == nullptr) {
        throw std::logic_error("entity type without a factory: " + type.name);
    }
    if (find(type.category, type.legacyId) != nullptr) {
        throw std::logic_error("entity id registered twice: " + std::to_string(type.legacyId) + " (" +
                               type.name + ")");
    }
    if (find(type.name) != nullptr) {
        throw std::logic_error("entity name registered twice: " + type.name);
    }
    types_.push_back(std::move(type));
}

const EntityType* EntityRegistry::find(EntityCategory category, int legacyId) const {
    for (const EntityType& type : types_) {
        if (type.category == category && type.legacyId == legacyId) {
            return &type;
        }
    }
    return nullptr;
}

const EntityType* EntityRegistry::find(std::string_view name) const {
    for (const EntityType& type : types_) {
        if (type.name == name) {
            return &type;
        }
    }
    return nullptr;
}

Entity* EntityRegistry::spawn(World& world, EntityCategory category, int legacyId,
                              const SpawnPoint& at) const {
    const EntityType* type = find(category, legacyId);
    if (type == nullptr) {
        return nullptr;
    }
    Entity* entity = type->create(world, at);
    switch (type->placement) {
        case Placement::Enemies: {
            world.enemies.add(entity);
            ShotTarget* target = entity->asShotTarget();
            if (target != nullptr && target->countsForKills()) {
                world.enemiesCreated += 1;
            }
            break;
        }
        case Placement::Props:
            world.props.add(entity);
            break;
        case Placement::MovingPlatforms:
            world.movingPlatforms.add(entity);
            break;
        case Placement::Pickups:
            world.pickups.add(entity);
            break;
        case Placement::Vehicles:
            world.vehicles.add(entity);
            break;
    }
    return entity;
}

}  // namespace bb::game
