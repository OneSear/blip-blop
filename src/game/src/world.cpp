#include "bb/game/world.hpp"

#include <cstddef>

#include "bb/core/assets/asset_types.hpp"

namespace bb::game {

void CollisionMap::load(const core::assets::LevelData& level) {
    clear();

    screens = static_cast<int>(level.screens.size());
    width = screens * kScreenWidth;
    const auto w = static_cast<std::size_t>(width);
    const auto columns = static_cast<std::size_t>(width / kCellSize);

    for (std::size_t layer = 0; layer < static_cast<std::size_t>(kLayers); ++layer) {
        auto row = std::make_unique<int[]>(w);
        const auto& source = level.platforms[layer];
        for (std::size_t x = 0; x < w; ++x) {
            row[x] = static_cast<int>(source[x]);
        }
        platformTable_.push_back(row.get());
        platformRows_.push_back(std::move(row));
    }

    for (std::size_t r = 0; r < static_cast<std::size_t>(kWallRows); ++r) {
        auto solid = std::make_unique<bool[]>(columns);
        auto blood = std::make_unique<bool[]>(columns);
        for (std::size_t c = 0; c < columns; ++c) {
            solid[c] = level.solidWalls[r * columns + c] != 0;
            blood[c] = level.bloodWalls[r * columns + c] != 0;
        }
        solidTable_.push_back(solid.get());
        bloodTable_.push_back(blood.get());
        solidRows_.push_back(std::move(solid));
        bloodRows_.push_back(std::move(blood));
    }

    platforms = platformTable_.data();
    solidWalls = solidTable_.data();
    bloodWalls = bloodTable_.data();
}

void CollisionMap::clear() {
    platforms = nullptr;
    solidWalls = nullptr;
    bloodWalls = nullptr;
    platformTable_.clear();
    solidTable_.clear();
    bloodTable_.clear();
    platformRows_.clear();
    solidRows_.clear();
    bloodRows_.clear();
    screens = 0;
    width = 0;
}

int CollisionMap::platformAt(int x, int y) const {
    if (x < 0 || x >= width || y >= kScreenHeight) {
        return 0;
    }
    for (int layer = 0; layer < kLayers; ++layer) {
        const int h = platforms[layer][x];
        if (y >= h && y <= h + 10) {
            return h;
        }
    }
    return 0;
}

void CollisionMap::setPlatform(int layer, int from, int to, int height) {
    if (layer < 0 || layer >= kLayers || platforms == nullptr) {
        return;
    }
    const int first = from < 0 ? 0 : from;
    const int last = to > width ? width : to;
    for (int i = first; i < last; ++i) {
        platforms[layer][i] = height;
    }
}

int CollisionMap::platformLayerAt(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= kScreenHeight) {
        return -1;
    }
    for (int layer = 0; layer < kLayers; ++layer) {
        const int h = platforms[layer][x];
        if (y >= h && y <= h + 10) {
            return layer;
        }
    }
    return -1;
}

bool CollisionMap::solidWall(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= kScreenHeight) {
        return false;
    }
    return solidWalls[y / kCellSize][x / kCellSize];
}

bool CollisionMap::bloodWall(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= kScreenHeight) {
        return false;
    }
    return bloodWalls[y / kCellSize][x / kCellSize];
}

namespace {

// For each a of first, each b of second (both newest first, with the
// lists' cursors, as the 2002 loops did), calls f(a, b).
template <class F>
void forEachPair(EntityList<Entity>& first, EntityList<Entity>& second, F&& f) {
    for (first.rewind(); !first.atEnd(); first.next()) {
        Entity* a = first.current();
        for (second.rewind(); !second.atEnd(); second.next()) {
            f(*a, *second.current());
        }
    }
}

}  // namespace

void World::rememberPositions() {
    motion.previousCameraX = camera.x;
    const auto remember = [](Entity* e) {
        e->previousX = e->x;
        e->previousY = e->y;
    };
    for (EntityList<Entity>* list :
         {&players, &playerShots, &cowBombs, &impacts, &vehicles, &enemies, &enemyShots, &pickups, &props,
          &midground, &foreground, &movingPlatforms, &blood, &gore, &weatherParticles, &bubbles}) {
        list->forEach(remember);
    }
}

void World::updateEvents() {
    // Sleeping events move to the active list as the camera reaches them.
    if (!sleepingEvents.empty()) {
        sleepingEvents.rewind();
        LevelEvent* event = sleepingEvents.current();

        while (!sleepingEvents.empty() && !sleepingEvents.atEnd() && event->wakes(*this)) {
            events.add(event);
            sleepingEvents.releaseCurrent();  // moved, not destroyed

            if (!sleepingEvents.empty()) {
                sleepingEvents.rewind();
                event = sleepingEvents.current();
            }
        }
    }

    // Active events run when ready, then go (in that order: removing first
    // would run a destroyed event).
    if (!events.empty()) {
        events.rewind();

        while (!events.empty() && !events.atEnd()) {
            LevelEvent* event = events.current();

            if (event->ready(*this)) {
                event->run(*this);
                events.removeCurrent();
            } else {
                events.next();
            }
        }
    }
}

void World::resolveCollisions(bool playersVulnerable) {
    const auto shotHitsEnemy = [](Entity& shot, Entity& enemy) {
        if (shot.hits(enemy)) {
            enemy.asShotTarget()->hitBy(*shot.asShot());
        }
    };
    forEachPair(playerShots, enemies, shotHitsEnemy);
    forEachPair(cowBombs, enemies, shotHitsEnemy);

    forEachPair(pickups, players, [](Entity& pickup, Entity& player) {
        if (pickup.hits(player)) {
            pickup.asCollectable()->takenBy(player);
        }
    });

    if (!playersVulnerable) {
        return;
    }
    forEachPair(enemies, players, [](Entity& enemy, Entity& player) {
        if (enemy.hits(player)) {
            player.asHurtable()->hurt(enemy.asShotTarget()->contactDamage());
        }
    });
    forEachPair(enemyShots, players, [](Entity& shot, Entity& player) {
        if (shot.hits(player)) {
            player.asHurtable()->hurt(shot.asShot()->damage());
        }
    });
}

}  // namespace bb::game
