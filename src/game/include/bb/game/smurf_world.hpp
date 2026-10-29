#pragma once

#include "bb/game/entity_registry.hpp"

namespace bb::game {

// Registers the enemies of the Smurf world (legacy ids 0 to 8; the Big
// Smurf, 8, was unfinished in 2002 and is finished in the port).
void registerSmurfWorld(EntityRegistry& registry);

}  // namespace bb::game
