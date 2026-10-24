#include <doctest.h>

#include <stdexcept>

#include "bb/game/entity_registry.hpp"
#include "bb/game/world.hpp"

namespace {

using bb::game::Entity;
using bb::game::EntityCategory;
using bb::game::EntityRegistry;
using bb::game::EntityState;
using bb::game::Placement;
using bb::game::SpawnPoint;
using bb::game::World;

class Thing : public Entity, public bb::game::ShotTarget {
public:
    explicit Thing(bool counts) : counts_(counts) {}
    void update() override {}
    void draw() override {}
    EntityState state() const override { return {}; }
    ShotTarget* asShotTarget() override { return this; }
    void hitBy(bb::game::Shot&) override {}
    int contactDamage() override { return 0; }
    bool countsForKills() override { return counts_; }
    int direction = -1;

private:
    bool counts_;
};

Entity* makeCounted(World&, const SpawnPoint& at) {
    auto* thing = new Thing(true);
    thing->x = at.x;
    thing->y = at.y;
    thing->direction = at.direction;
    return thing;
}

Entity* makeUncounted(World&, const SpawnPoint&) { return new Thing(false); }

EntityRegistry sample() {
    EntityRegistry registry;
    registry.add({EntityCategory::Enemy, 0, "counted", Placement::Enemies, makeCounted});
    registry.add({EntityCategory::Enemy, 1, "uncounted", Placement::Enemies, makeUncounted});
    registry.add({EntityCategory::Pickup, 0, "pickup", Placement::Pickups, makeUncounted});
    registry.add({EntityCategory::Prop, 5, "prop", Placement::Props, makeUncounted});
    registry.add({EntityCategory::Prop, 6, "platform", Placement::MovingPlatforms,
                  makeUncounted});
    return registry;
}

}  // namespace

TEST_CASE("EntityRegistry finds types by category and id, and by name") {
    EntityRegistry registry = sample();
    CHECK(registry.types().size() == 5);
    REQUIRE(registry.find(EntityCategory::Enemy, 0) != nullptr);
    CHECK(registry.find(EntityCategory::Enemy, 0)->name == "counted");
    CHECK(registry.find(EntityCategory::Pickup, 0)->name == "pickup");
    CHECK(registry.find(EntityCategory::Prop, 0) == nullptr);
    CHECK(registry.find("platform")->legacyId == 6);
    CHECK(registry.find("missing") == nullptr);
}

TEST_CASE("EntityRegistry rejects duplicates and missing factories") {
    EntityRegistry registry = sample();
    CHECK_THROWS_AS(registry.add({EntityCategory::Enemy, 0, "other", Placement::Enemies,
                                  makeCounted}),
                    std::logic_error);
    CHECK_THROWS_AS(registry.add({EntityCategory::Prop, 9, "counted", Placement::Props,
                                  makeCounted}),
                    std::logic_error);
    CHECK_THROWS_AS(registry.add({EntityCategory::Prop, 9, "none", Placement::Props, nullptr}),
                    std::logic_error);
    CHECK(registry.types().size() == 5);
}

TEST_CASE("EntityRegistry::spawn places the entity in its list") {
    EntityRegistry registry = sample();
    World world;

    Entity* enemy = registry.spawn(world, EntityCategory::Enemy, 0, {10, 20, 1});
    REQUIRE(enemy != nullptr);
    CHECK(enemy->x == 10);
    CHECK(enemy->y == 20);
    CHECK(static_cast<Thing*>(enemy)->direction == 1);
    CHECK(world.enemies.size() == 1);
    CHECK(world.enemiesCreated == 1);

    registry.spawn(world, EntityCategory::Enemy, 1, {});
    CHECK(world.enemies.size() == 2);
    CHECK(world.enemiesCreated == 1);

    registry.spawn(world, EntityCategory::Pickup, 0, {});
    registry.spawn(world, EntityCategory::Prop, 5, {});
    registry.spawn(world, EntityCategory::Prop, 6, {});
    CHECK(world.pickups.size() == 1);
    CHECK(world.props.size() == 1);
    CHECK(world.movingPlatforms.size() == 1);

    CHECK(registry.spawn(world, EntityCategory::Prop, 7, {}) == nullptr);
}
