#pragma once

#include "bb/game/entity_registry.hpp"

namespace bb::game {

// Registers the enemies (legacy ids 400, 401) and props (400 to 403) of the
// Lemmings bonus level.
void registerLemmingsWorld(EntityRegistry& registry);

}  // namespace bb::game
