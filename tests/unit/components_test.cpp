#include <doctest.h>

#include "bb/game/components.hpp"

using bb::game::AnimationPlayer;
using bb::game::Health;
using bb::game::Hitbox;

TEST_CASE("AnimationPlayer::tick advances a step every `delay` ticks") {
    AnimationPlayer a;
    CHECK_FALSE(a.tick(3));
    CHECK_FALSE(a.tick(3));
    CHECK(a.tick(3));
    CHECK(a.frame == 1);
    CHECK(a.subFrame == 0);
}

TEST_CASE("AnimationPlayer::cycle wraps, tickUpTo holds the last step") {
    AnimationPlayer a;
    for (int i = 0; i < 2 * 3; ++i) {
        a.cycle(2, 3);
    }
    CHECK(a.frame == 0);

    AnimationPlayer b;
    for (int i = 0; i < 20; ++i) {
        b.tickUpTo(2, 4);
    }
    CHECK(b.frame == 4);
}

TEST_CASE("AnimationPlayer::loop plays a table as the 2002 anime()") {
    static const int kTable[] = {10, 20, 30};
    AnimationPlayer a;
    a.frame = -2;  // a step left negative by a class resets to 0
    CHECK(a.loop(kTable, 3, 5) == 10);
    for (int i = 0; i < 4; ++i) {
        a.loop(kTable, 3, 5);
    }
    CHECK(a.loop(kTable, 3, 5) == 20);
    a.restart();
    CHECK(a.frame == 0);
    CHECK(a.subFrame == 0);
}

TEST_CASE("Hitbox overlaps only when both are active, corners inclusive") {
    Hitbox a;
    Hitbox b;
    a.set(0, 0, 10, 10);
    b.set(10, 10, 20, 20);
    CHECK(a.overlaps(b));
    b.set(11, 0, 20, 10);
    CHECK_FALSE(a.overlaps(b));
    b.set(5, 5, 6, 6);
    b.active = false;
    CHECK_FALSE(a.overlaps(b));
}

// The 2002 Ennemi::estTouche(): a wound smaller than the points left is
// taken whole, else the points left are taken and the enemy dies.
TEST_CASE("Health::take matches the 2002 damage rule") {
    for (int points = -3; points <= 6; ++points) {
        for (int wound = -3; wound <= 6; ++wound) {
            Health h;
            h.points = points;
            const int taken = h.take(wound);
            if (wound < points) {
                CHECK(taken == wound);
                CHECK(h.points == points - wound);
                CHECK(h.left());
            } else {
                CHECK(taken == points);
                CHECK(h.points == 0);
                CHECK_FALSE(h.left());
            }
        }
    }
}

TEST_CASE("Health::heal stops at the maximum") {
    Health h;
    h.points = 4;
    h.heal(2, 5);
    CHECK(h.points == 5);
}
