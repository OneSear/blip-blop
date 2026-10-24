#include <doctest.h>

#include "bb/game/entity.hpp"

namespace {

using bb::game::Entity;
using bb::game::EntityState;
using bb::game::Handle;

struct Thing : Entity {
    void update() override {}
    void draw() override {}
    EntityState state() const override { return {}; }
    int value = 7;
};

}  // namespace

TEST_CASE("A handle becomes null when its entity is destroyed") {
    auto* thing = new Thing;
    Handle<Thing> typed(thing);
    Handle<> plain(thing);
    Handle<> copy = plain;  // NOLINT(performance-unnecessary-copy-initialization): the copy is the test

    CHECK(typed.get() == thing);
    CHECK(typed->value == 7);
    CHECK(plain.get() == thing);
    CHECK(static_cast<bool>(copy));

    thing->dead = true;  // dead but not destroyed: still reachable
    CHECK(plain->dead);

    delete thing;
    CHECK(typed.get() == nullptr);
    CHECK_FALSE(static_cast<bool>(plain));
    CHECK(copy.get() == nullptr);
}

TEST_CASE("Handles start null, reset, and do not follow copies of an entity") {
    Handle<> none;
    CHECK(none.get() == nullptr);
    Handle<Thing> fromNull(nullptr);
    CHECK_FALSE(static_cast<bool>(fromNull));

    Thing original;
    original.x = 12;
    Handle<> handle(&original);
    {
        Thing copy(original);
        CHECK(copy.x == 12);
        Handle<> toCopy(&copy);
        CHECK(toCopy.get() == &copy);
    }
    CHECK(handle.get() == &original);

    Thing assigned;
    Handle<> toAssigned(&assigned);
    assigned = original;
    CHECK(toAssigned.get() == &assigned);
    CHECK(assigned.x == 12);

    handle.reset();
    CHECK(handle.get() == nullptr);
}
