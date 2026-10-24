#include <doctest.h>

#include <cstddef>

#include "bb/core/assets/asset_types.hpp"
#include "bb/game/world.hpp"

namespace {

// A one-screen level with no platform and no wall.
bb::core::assets::LevelData emptyLevel() {
    bb::core::assets::LevelData level;
    level.screens = {0};
    for (auto& layer : level.platforms) {
        layer.assign(640, 0);
    }
    level.solidWalls.assign(std::size_t{60} * 80, 0);
    level.bloodWalls.assign(std::size_t{60} * 80, 0);
    return level;
}

}  // namespace

TEST_CASE("CollisionMap::setPlatform clips to the level") {
    bb::game::CollisionMap map;
    map.load(emptyLevel());
    REQUIRE(map.width == 640);

    map.setPlatform(4, -90, 10, 200);  // starts left of the level
    CHECK(map.platforms[4][0] == 200);
    CHECK(map.platforms[4][9] == 200);
    CHECK(map.platforms[4][10] == 0);
    CHECK(map.platformAt(5, 205) == 200);
    CHECK(map.platformLayerAt(5, 205) == 4);

    map.setPlatform(3, 630, 900, 100);  // ends right of it
    CHECK(map.platforms[3][629] == 0);
    CHECK(map.platforms[3][639] == 100);

    map.setPlatform(8, 0, 640, 1);  // no such layer: nothing happens
    map.setPlatform(-1, 0, 640, 1);
    CHECK(map.platforms[7][0] == 0);

    bb::game::CollisionMap empty;  // nothing loaded
    empty.setPlatform(4, 0, 10, 1);
    CHECK(empty.platforms == nullptr);
}
