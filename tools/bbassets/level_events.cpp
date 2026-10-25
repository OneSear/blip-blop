#include "level_events.hpp"

#include "bb/game/content.hpp"

namespace bb::tools {

using core::assets::LevelEventRecord;
using Slot = EventSlot;
using Entity = EventEntity;

std::int32_t& slotRef(LevelEventRecord& e, Slot s) {
    switch (s) {
        case Slot::Id:
            return e.id;
        case Slot::X:
            return e.x;
        case Slot::Bank:
            return e.bank;
        case Slot::Y:
            return e.y;
        case Slot::Capacity:
            return e.capacity;
        case Slot::Value:
            return e.value;
        case Slot::Period:
            break;
    }
    return e.period;
}

const game::EntityRegistry& entityRegistry() {
    static const game::EntityRegistry registry = [] {
        game::EntityRegistry r;
        game::registerAllEntities(r);
        return r;
    }();
    return registry;
}

game::EntityCategory categoryOf(EventEntity entity) {
    switch (entity) {
        case Entity::Prop:
            return game::EntityCategory::Prop;
        case Entity::Pickup:
            return game::EntityCategory::Pickup;
        case Entity::Vehicle:
            return game::EntityCategory::Vehicle;
        default:
            return game::EntityCategory::Enemy;
    }
}

// Field names by event type: the values the game's loader (game.cpp) or
// the level editor read (docs/formats/source-assets.md).
const std::map<std::int32_t, EventSchema>& eventSchemas() {
    static const std::map<std::int32_t, EventSchema> schemas = {
        {0,
         {"enemy",
          {{"enemy", Slot::Id, Entity::Enemy}, {"x", Slot::X}, {"y", Slot::Y}, {"facing", Slot::Value}}}},
        {1,
         {"enemyGenerator",
          {{"enemy", Slot::Id, Entity::Enemy},
           {"x", Slot::X},
           {"y", Slot::Y},
           {"facing", Slot::Value},
           {"capacity", Slot::Capacity},
           {"period", Slot::Period}},
          true}},
        {2, {"animatedProp", {{"prop", Slot::Id, Entity::Prop}, {"x", Slot::X}, {"y", Slot::Y}}}},
        {3,
         {"midgroundDecal",
          {{"image", Slot::Id}, {"x", Slot::X}, {"y", Slot::Y}, {"editorImage", Slot::Capacity}}}},
        {4,
         {"foreground",
          {{"image", Slot::Id}, {"x", Slot::X}, {"y", Slot::Y}, {"editorImage", Slot::Capacity}}}},
        {5, {"scrollLock", {{"condition", Slot::Y}, {"flag", Slot::Capacity}, {"value", Slot::Value}}}},
        {6, {"holdFire", {{"flag", Slot::Capacity}, {"value", Slot::Value}}}},
        {7, {"forceScroll", {{"speed", Slot::Y}}}},
        {8,
         {"dialogue",
          {{"scene", Slot::Id}, {"condition", Slot::Y}, {"flag", Slot::Capacity}, {"value", Slot::Value}}}},
        {9, {"setFlag", {{"flag", Slot::Capacity}, {"value", Slot::Value}}}},
        {10,
         {"bannerText",
          {{"text", Slot::Bank}, {"condition", Slot::Y}, {"flag", Slot::Capacity}, {"value", Slot::Value}}}},
        {11,
         {"sound",
          {{"sound", Slot::Id},
           {"soundBank", Slot::Bank},
           {"flag", Slot::Capacity},
           {"value", Slot::Value}}}},
        {12, {"weather", {{"kind", Slot::Id}, {"intensity", Slot::Y}}}},
        {13, {"overkill", {{"enemy", Slot::Id}, {"duration", Slot::Capacity}}}},
        {14, {"dynamicLoad", {{"id", Slot::Id}}}},
        {15, {"music", {{"track", Slot::Id}, {"play", Slot::Y}}}},
        {16, {"pickupGenerator", {{"pickup", Slot::Id}, {"period", Slot::Period}}}},
        {17,
         {"vehicle",
          {{"vehicle", Slot::Id, Entity::Vehicle},
           {"x", Slot::X},
           {"y", Slot::Y},
           {"direction", Slot::Value}}}},
        {18, {"pickup", {{"pickup", Slot::Id, Entity::Pickup}, {"x", Slot::X}, {"y", Slot::Y}}}},
    };
    return schemas;
}

std::int32_t slotValue(const LevelEventRecord& e, Slot s) {
    LevelEventRecord copy = e;
    return slotRef(copy, s);
}

}  // namespace bb::tools
