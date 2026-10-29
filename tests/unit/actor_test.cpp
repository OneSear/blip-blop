#include <doctest.h>

#include "bb/game/actor.hpp"
#include "bb/game/world.hpp"

namespace {

// A piece of debris as the 2002 ones: falls, ends where it lands, and
// (D-55 fixed) once fallen below the screen.
class Debris : public bb::game::Actor {
public:
    explicit Debris(bb::game::World& world) : Actor(world) {}
    void update() override {
        fall();
        if (platformAt(x, y) != 0) {
            landed = true;
            dead = true;
        }
        dieWhenFallenOut();
    }
    bool landed = false;
};

}  // namespace

TEST_CASE("debris that never lands leaves the level once below the screen") {
    bb::game::World world;  // no level loaded: no platform anywhere
    Debris piece(world);
    piece.x = 100;
    piece.y = 300;
    piece.dy = -4;

    int ticks = 0;
    while (!piece.dead && ticks < 10000) {
        piece.update();
        ticks += 1;
        if (!piece.dead) {
            CHECK(piece.y <= 600);
        }
    }
    CHECK(piece.dead);
    CHECK_FALSE(piece.landed);
    CHECK(piece.y > 600);
    CHECK(piece.y <= 600 + bb::game::kMaxFallSpeed);
}

TEST_CASE("smooth motion draws between the last two positions") {
    bb::game::Motion motion;
    int dx = 0;
    int dy = 0;

    // Off, or with no previous position: where the entity is.
    motion.fraction = 128;
    motion.offsets(100, 50, 104, 50, dx, dy);
    CHECK(dx == 0);
    motion.on = true;
    motion.offsets(bb::game::Entity::kNowhere, bb::game::Entity::kNowhere, 104, 50, dx, dy);
    CHECK(dx == 0);

    // Half a tick owed since a 4-pixel move right and 2 up: 2 pixels back
    // left and 1 down, rounded to the nearest pixel.
    motion.offsets(100, 52, 104, 50, dx, dy);
    CHECK(dx == -2);
    CHECK(dy == 1);
    motion.fraction = 0;  // just ticked: where it was
    motion.offsets(100, 52, 104, 50, dx, dy);
    CHECK(dx == -4);
    CHECK(dy == 2);
    motion.fraction = 256;  // a whole tick owed: where it is
    motion.offsets(100, 52, 104, 50, dx, dy);
    CHECK(dx == 0);
    CHECK(dy == 0);
    motion.fraction = 64;  // a quarter: 3 pixels back of 4
    motion.offsets(104, 50, 100, 50, dx, dy);
    CHECK(dx == 3);

    // A jump is drawn where it ends.
    motion.fraction = 128;
    motion.offsets(100, 50, 100 + bb::game::Motion::kMaxStep + 1, 50, dx, dy);
    CHECK(dx == 0);
}
