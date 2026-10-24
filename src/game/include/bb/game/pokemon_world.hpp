#pragma once

#include "bb/game/entity_registry.hpp"

namespace bb::game {

// Registers the enemies (legacy ids 100 to 104) and props (100 to 111, and
// 900) of the Pokémon bonus level.
void registerPokemonWorld(EntityRegistry& registry);

}  // namespace bb::game
