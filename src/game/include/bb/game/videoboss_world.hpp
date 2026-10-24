#pragma once

#include "bb/game/entity_registry.hpp"

namespace bb::game {

// Registers the enemies and props of videoboss, the video-game boss level
// (legacy enemy ids 600 to 605, prop ids 600 to 611).
void registerVideoBossWorld(EntityRegistry& registry);

}  // namespace bb::game
