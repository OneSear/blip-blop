#include "bb/game/video_world.hpp"
#include "video/video_world.hpp"

namespace bb::game {

namespace {

template <class T>
Entity* make(World& world, const SpawnPoint& at) {
    auto* enemy = new T(world);
    enemy->x = at.x;
    enemy->y = at.y;
    enemy->direction = at.direction;
    return enemy;
}

template <bool kFlying>
Entity* makeTails(World& world, const SpawnPoint& at) {
    auto* tails = new video::Tails(world, kFlying);
    tails->x = at.x;
    tails->y = at.y;
    tails->direction = at.direction;
    return tails;
}

// The block, coin and ring clocks: each kind animates in step (2002 class
// statics, in World::shared(); reset at each level start, D-50).
struct BlockClock : video::SyncedProp::Clock {};
struct CoinClock : video::SyncedProp::Clock {};
struct RingClock : video::SyncedProp::Clock {};

template <typename Clock, int kFirst, int kSteps, int kDelay>
Entity* makeSynced(World& world, const SpawnPoint& at) {
    auto* prop = new video::SyncedProp(world, world.shared<Clock>(), kFirst, kSteps, kDelay);
    prop->x = at.x;
    prop->y = at.y;
    return prop;
}

template <int kFirst, int kSteps, int kDelay>
Entity* makeLoop(World& world, const SpawnPoint& at) {
    auto* prop = new video::LoopProp(world, kFirst, kSteps, kDelay);
    prop->x = at.x;
    prop->y = at.y;
    return prop;
}

void addEnemy(EntityRegistry& registry, int id, const char* name, EntityFactory factory) {
    registry.add({EntityCategory::Enemy, id, name, Placement::Enemies, factory});
}

void addProp(EntityRegistry& registry, int id, const char* name, EntityFactory factory) {
    registry.add({EntityCategory::Prop, id, name, Placement::Props, factory});
}

namespace prop = video::prop;

}  // namespace

void registerVideoWorld(EntityRegistry& registry) {
    addEnemy(registry, 500, "video.toad", make<video::Toad>);
    addEnemy(registry, 501, "video.yoshi", make<video::Yoshi>);
    addEnemy(registry, 503, "video.princess", make<video::Princess>);
    addEnemy(registry, 504, "video.tails", makeTails<false>);
    addEnemy(registry, 505, "video.yoshi_turret", make<video::YoshiTurret>);
    addEnemy(registry, 506, "video.knuckles", make<video::Knuckles>);
    addEnemy(registry, 507, "video.rayman", make<video::Rayman>);
    addEnemy(registry, 508, "video.flying_tails", makeTails<true>);

    addProp(registry, 500, "video.question_block", makeSynced<BlockClock, prop::kQuestionBlock, 9, 5>);
    addProp(registry, 501, "video.coin", makeSynced<CoinClock, prop::kCoin, 4, 6>);
    addProp(registry, 502, "video.ring", makeSynced<RingClock, prop::kRing, 4, 6>);
    addProp(registry, 503, "video.sunflower_1", makeLoop<prop::kSunflower1, 2, 10>);
    addProp(registry, 504, "video.sunflower_2", makeLoop<prop::kSunflower2, 2, 10>);
    addProp(registry, 505, "video.sunflower_3", makeLoop<prop::kSunflower3, 2, 10>);
    addProp(registry, 506, "video.sunflower_4", makeLoop<prop::kSunflower4, 2, 10>);
    addProp(registry, 507, "video.flag", makeLoop<prop::kFlag, 10, 5>);
}

}  // namespace bb::game
