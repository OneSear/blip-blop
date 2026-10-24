#include "bb/game/content.hpp"

#include "bb/game/carebears_world.hpp"
#include "bb/game/finalboss_world.hpp"
#include "bb/game/lemmings_world.hpp"
#include "bb/game/player.hpp"
#include "bb/game/pokemon_world.hpp"
#include "bb/game/smurf_world.hpp"
#include "bb/game/snorks_world.hpp"
#include "bb/game/video_world.hpp"
#include "bb/game/videoboss_world.hpp"

namespace bb::game {

void registerAllEntities(EntityRegistry& registry) {
    registerSmurfWorld(registry);
    registerPokemonWorld(registry);
    registerCareBearWorld(registry);
    registerSnorkWorld(registry);
    registerLemmingsWorld(registry);
    registerVideoWorld(registry);
    registerVideoBossWorld(registry);
    registerFinalBossWorld(registry);
    registerPickups(registry);
}

}  // namespace bb::game
