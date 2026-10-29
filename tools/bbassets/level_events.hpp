#pragma once

// The fields of each level event type (docs/formats/source-assets.md): the
// names the source levels give them and the slot of the 2002 record they
// live in. Shared by the source asset conversion and the level editor.

#include <array>
#include <cstdint>
#include <map>
#include <vector>

#include "bb/core/assets/asset_types.hpp"
#include "bb/game/entity_registry.hpp"

namespace bb::tools {

enum class EventSlot { Id, X, Bank, Y, Capacity, Value, Period };
constexpr std::array<const char*, 7> kSlotNames = {"id", "x", "bank", "y", "capacity", "value", "period"};

std::int32_t& slotRef(core::assets::LevelEventRecord& e, EventSlot s);
std::int32_t slotValue(const core::assets::LevelEventRecord& e, EventSlot s);

// The entity an event places: levels name it (bb::game's registry), the
// pack and the 2002 files store its number.
enum class EventEntity { None, Enemy, Prop, Pickup, Vehicle };

struct EventField {
    const char* name;
    EventSlot slot;
    EventEntity entity = EventEntity::None;
};

struct EventSchema {
    const char* name;
    std::vector<EventField> fields;
    bool temporary = false;  // the enemy generator's bool
};

// By event type (core::assets::LevelEventType).
const std::map<std::int32_t, EventSchema>& eventSchemas();

// Every entity type of the game (bb::game::registerAllEntities).
const game::EntityRegistry& entityRegistry();
game::EntityCategory categoryOf(EventEntity entity);

}  // namespace bb::tools
