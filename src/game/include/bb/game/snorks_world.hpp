#pragma once

#include "bb/game/entity_registry.hpp"

namespace bb::game {

// Registers the enemies of the Snork world (legacy ids 300 to 307, except
// Diabolo, 306, never placed).
void registerSnorkWorld(EntityRegistry& registry);

}  // namespace bb::game
