#include <doctest.h>

#include "bb/core/assets/asset_types.hpp"
#include "bb/game/level_event.hpp"
#include "bb/game/world.hpp"

namespace {

using bb::core::assets::LevelEventRecord;
using bb::core::assets::LevelEventType;
using bb::game::Flag;
using bb::game::World;

LevelEventRecord record(LevelEventType type, int activationX) {
    LevelEventRecord r;
    r.type = static_cast<int>(type);
    r.activationX = activationX;
    return r;
}

void addEvent(World& world, const LevelEventRecord& r) {
    auto event = bb::game::makeLevelEvent(r);
    REQUIRE(event != nullptr);
    world.sleepingEvents.add(event.release());
}

}  // namespace

TEST_CASE("makeLevelEvent makes every used event type") {
    for (auto type : {LevelEventType::EnemyGenerator, LevelEventType::BannerText,
                      LevelEventType::Music, LevelEventType::PickupGenerator,
                      LevelEventType::Vehicle, LevelEventType::Enemy, LevelEventType::Dialogue}) {
        CHECK(bb::game::makeLevelEvent(record(type, 0)) != nullptr);
    }
    // Never used by the shipped levels.
    for (auto type : {LevelEventType::Overkill, LevelEventType::DynamicLoad}) {
        CHECK(bb::game::makeLevelEvent(record(type, 0)) == nullptr);
    }
}

TEST_CASE("A banner text waits for its flag condition") {
    World world;
    bb::game::BannerTextEvent always(500, 0, 2, 7);
    bb::game::BannerTextEvent equal(500, 1, 2, 7);
    bb::game::BannerTextEvent atLeast(500, 2, 2, 7);

    CHECK(always.ready(world));
    CHECK_FALSE(equal.ready(world));
    CHECK_FALSE(atLeast.ready(world));

    world.flags[Flag::User2] = 7;
    CHECK(equal.ready(world));
    CHECK(atLeast.ready(world));

    world.flags[Flag::User2] = 8;
    CHECK_FALSE(equal.ready(world));
    CHECK(atLeast.ready(world));
}

namespace {

class Dummy : public bb::game::Entity {
public:
    void update() override {}
    void draw() override {}
    bb::game::EntityState state() const override { return {}; }
};

bb::game::Entity* makeDummy(World&, const bb::game::SpawnPoint& at) {
    auto* dummy = new Dummy;
    dummy->x = at.x;
    dummy->y = at.y;
    return dummy;
}

}  // namespace

TEST_CASE("An enemy generator spawns every period, away from players, until its capacity") {
    using bb::game::EnemyGenerator;
    bb::game::EntityRegistry registry;
    registry.add({bb::game::EntityCategory::Enemy, 3, "dummy", bb::game::Placement::Enemies, makeDummy});
    World world;
    world.registry = &registry;

    Dummy player;
    player.x = 1000;
    player.y = 300;
    world.players.add(&player);

    // At (500, 300), every 4 ticks, 2 times.
    EnemyGenerator generator(world, EnemyGenerator::Setup{3, 500, 300, 0, 4, 2, false});
    for (int tick = 0; tick < 4; tick++) {
        generator.update();
    }
    REQUIRE(world.enemies.count() == 1);
    CHECK_FALSE(generator.finished());

    // A player within 100 pixels: that spawn is skipped, but it counts.
    player.x = 550;
    for (int tick = 0; tick < 4; tick++) {
        generator.update();
    }
    CHECK(world.enemies.count() == 1);
    CHECK(generator.finished());

    // Off while the generators are off.
    EnemyGenerator paused(world, EnemyGenerator::Setup{3, 500, 300, 0, 1, 5, false});
    player.x = 1000;
    world.flags[Flag::GeneratorsOff] = 1;
    paused.update();
    CHECK(world.enemies.count() == 1);
    world.flags[Flag::GeneratorsOff] = 0;
    paused.update();
    CHECK(world.enemies.count() == 2);

    world.players.clear();
}

TEST_CASE("Events wake with the camera, then run when ready") {
    World world;

    // Added in reverse level order: the list visits the newest first.
    LevelEventRecord dialogue = record(LevelEventType::Dialogue, 200);
    dialogue.id = 4;
    dialogue.y = 1;  // conditional
    dialogue.capacity = 2;
    dialogue.value = 7;
    addEvent(world, dialogue);

    LevelEventRecord lock = record(LevelEventType::ScrollLock, 100);
    lock.y = 2;
    lock.capacity = 1;
    lock.value = 3;
    addEvent(world, lock);

    LevelEventRecord flag = record(LevelEventType::SetFlag, 0);
    flag.capacity = 2;
    flag.value = 7;
    addEvent(world, flag);

    world.camera.x = 50;
    world.updateEvents();
    CHECK(world.flags[Flag::User2] == 7);
    CHECK_FALSE(world.scrollLock.on);
    CHECK(world.sleepingEvents.size() == 2);
    CHECK(world.events.size() == 0);

    world.camera.x = 250;
    world.flags[Flag::User2] = 0;
    world.updateEvents();
    CHECK(world.scrollLock.on);
    CHECK(world.scrollLock.x == 100);
    CHECK(world.scrollLock.condition == 2);
    CHECK(world.scrollLock.flag == 1);
    CHECK(world.scrollLock.value == 3);
    CHECK(world.dialogueToPlay == -1);  // waits for flag 2 == 7
    CHECK(world.events.size() == 1);

    world.flags[Flag::User2] = 7;
    world.updateEvents();
    CHECK(world.dialogueToPlay == 4);
    CHECK(world.events.empty());
}
