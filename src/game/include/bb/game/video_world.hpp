#pragma once

#include "bb/game/entity_registry.hpp"

namespace bb::game {

// Registers the enemies and props of video, the video-game world's first
// level (legacy enemy ids 500 to 508, except 502, the Princess's guard,
// which only she creates; prop ids 500 to 507).
void registerVideoWorld(EntityRegistry& registry);

}  // namespace bb::game
