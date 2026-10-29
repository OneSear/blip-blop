#pragma once

#include "bb/game/player.hpp"

namespace bb::game {

// The level's head-up display (screens/hud.cpp of the legacy port): per
// player still alive the life points, the score, the lives (or "Bonus
// Stage"), the weapon and its ammunition and the cow bombs, player 1 from
// the left edge and player 2 mirrored from the right, players 3 and 4 (the
// four-player mode) likewise in a second row under them; the timer in the
// middle when the level has one.
class Hud {
public:
    // players: kMaxPlayers of them, null when absent. blink: the shared
    // blinking phase (the ammunition of a firing player).
    static void draw(const World& world, const Player* const* players, bool bonusLevel, bool blink);
};

}  // namespace bb::game
