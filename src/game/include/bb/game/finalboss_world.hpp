#pragma once

#include "bb/game/entity_registry.hpp"

namespace bb::game {

// Registers the enemy and props of finalboss, the final boss level (legacy
// enemy id 700, prop ids 700 to 704; the star, 705, is never placed).
void registerFinalBossWorld(EntityRegistry& registry);

}  // namespace bb::game
