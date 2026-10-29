// The markers of the 2002 levels: the level positions the 2002 classes kept
// in their code (migration.md D-51). The source levels (assets/source)
// carry them; the 2002 .lvl files cannot, so the 2002 data source adds them
// from here, and `bbassets extract` writes them into the source levels.

#include <cctype>
#include <string>

#include "bb/core/assets/legacy_formats.hpp"

namespace bb::core::assets {

namespace {

// "data\\SnorkNiv2.lvl" -> "snorkniv2".
std::string levelStem(std::string_view path) {
    const std::size_t slash = path.find_last_of("/\\");
    std::string_view name = slash == std::string_view::npos ? path : path.substr(slash + 1);
    const std::size_t dot = name.rfind('.');
    if (dot != std::string_view::npos) {
        name = name.substr(0, dot);
    }
    std::string stem;
    for (const char c : name) {
        stem.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return stem;
}

}  // namespace

LevelMarkers legacyLevelMarkers(std::string_view levelPath) {
    const std::string stem = levelStem(levelPath);
    if (stem == "snuf1") {
        // The wild Smurfs never go right of this (EnnemiSmurfSauvage).
        return {{"smurf.wildMaxX", {5850}}};
    }
    if (stem == "bisous3") {
        // Where the shaman appears, in turn, (x, y) (EnnemiBisouChaman).
        return {{"carebears.shamanSpots", {8144, 296, 8082, 130, 7653, 188}}};
    }
    if (stem == "snorkniv") {
        return {{"level.flamethrower", {0}}};  // under water (game.cpp)
    }
    if (stem == "snorkniv2") {
        return {
            {"level.flamethrower", {0}},
            // The Snork in Black's four perches (x, y), the target's zones
            // that choose one (left of x, right of x, above y), and the
            // shark's path (start, sinks until, takes him at, rises from,
            // gone at).
            {"snorks.bossPerches", {5904, 175, 6351, 189, 5831, 342, 6319, 328}},
            {"snorks.bossZones", {5950, 6200, 200}},
            {"snorks.sharkPath", {5604, 5800, 5904, 6004, 6800}},
        };
    }
    if (stem == "lem") {
        // Where the cow bomb pickup falls after the 200th kill.
        return {{"lemmings.cowBombX", {1500}}};
    }
    if (stem == "video") {
        return {
            {"video.princessWalk", {5760, 7500}},   // walks until, waits from
            {"video.raymanArena", {10428, 10930}},  // left edge, entry
        };
    }
    if (stem == "videoboss") {
        return {
            // Lara's spikes: x of the first column (outer rows, middle row),
            // then the y of the three rows; she is gone right of the exit;
            // her wall (column, first and last row) opens when she dies.
            {"videoboss.laraSpikes", {515, 547, 429, 449, 471}},
            {"videoboss.laraExitX", {950}},
            {"videoboss.laraWeightX", {565}},  // the weight that crushes her
            {"videoboss.laraWall", {108, 28, 55}},
            // The left edge of the arena of Mario, Luigi and Sonic: their
            // positions are relative to it.
            {"videoboss.arenaX", {1280}},
        };
    }
    if (stem == "finalboss") {
        return {
            {"finalboss.storkDropX", {1480}},
            {"finalboss.marioFloorY", {444}},
        };
    }
    return {};
}

}  // namespace bb::core::assets
