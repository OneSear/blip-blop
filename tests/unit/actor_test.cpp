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
