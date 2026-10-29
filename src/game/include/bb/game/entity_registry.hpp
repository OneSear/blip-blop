#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "bb/game/entity.hpp"

namespace bb::game {

struct World;

// The kinds of entity a level file places by number. Each has its own
// range of legacy ids (they overlap: pickup 0 is not enemy 0).
enum class EntityCategory { Enemy, Prop, Pickup, Vehicle };

// The world list a new entity goes to.
enum class Placement { Enemies, Props, MovingPlatforms, Pickups, Vehicles };

// Where a level event or a generator puts a new entity.
struct SpawnPoint {
    int x = 0;
    int y = 0;
    int direction = 0;  // enemies only (0 = left, 1 = right)
};

// Creates an entity of one type at a spawn point. The entity is not in a
// list yet.
using EntityFactory = Entity* (*)(World& world, const SpawnPoint& at);

struct EntityType {
    EntityCategory category = EntityCategory::Enemy;
    int legacyId = 0;  // the number in the level files
    std::string name;  // symbolic id, unique across categories
    Placement placement = Placement::Enemies;
    EntityFactory create = nullptr;
};

// The entity types a level can place, by legacy id (the number in the level
// files) or by name. Replaces the three switch factories of 2002
// (event_ennemi.cpp, event_fond_anime.cpp, event_bonus.cpp); each world
// registers its own types.
class EntityRegistry {
public:
    // Registers a type. Throws std::logic_error when its id (in its
    // category) or its name is already taken, or when it has no factory.
    void add(EntityType type);

    const EntityType* find(EntityCategory category, int legacyId) const;
    const EntityType* find(std::string_view name) const;
    const std::vector<EntityType>& types() const noexcept { return types_; }

    // Creates an entity of the given type and adds it to its world list;
    // an enemy that counts for the kill statistics adds one to
    // World::enemiesCreated. Returns null for an unknown id (the caller
    // reports it).
    Entity* spawn(World& world, EntityCategory category, int legacyId, const SpawnPoint& at) const;

private:
    std::vector<EntityType> types_;
};

}  // namespace bb::game
