#pragma once

#include "bb/game/entity_registry.hpp"

namespace bb::game {

// Registers the enemies of the Care Bear world (legacy ids 200 to 209,
// except the heart bear, 206, never placed).
void registerCareBearWorld(EntityRegistry& registry);

}  // namespace bb::game
