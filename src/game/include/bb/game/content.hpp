#pragma once

#include "bb/game/entity_registry.hpp"

namespace bb::game {

// Registers every entity type the levels place by number: each world's own,
// and the pickups shared by all (the three 2002 switch factories).
void registerAllEntities(EntityRegistry& registry);

}  // namespace bb::game
