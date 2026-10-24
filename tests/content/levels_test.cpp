// Content checks: every level of the campaign, read as the game reads it
// (from the 2002 data directory and from the runtime pack built from the
// source assets), places only entities the game registers and refers only
// to images, sounds and musics its banks have.

#include <doctest.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bb/core/assets/asset_source.hpp"
#include "bb/core/assets/pack.hpp"
#include "bb/core/vfs.hpp"
#include "bb/game/content.hpp"
#include "bb/game/entity_registry.hpp"

namespace {

using bb::core::assets::CampaignEntry;
using bb::core::assets::LevelData;
using bb::core::assets::LevelEventRecord;
using bb::core::assets::LevelEventType;
using bb::game::EntityCategory;

bb::game::EntityRegistry makeRegistry() {
    bb::game::EntityRegistry r;
    bb::game::registerAllEntities(r);
    return r;
}

// The markers each entity type reads (World::marker()): a level that
// places one must have them.
const std::map<std::string, std::vector<std::string>>& requiredMarkers() {
    static const std::map<std::string, std::vector<std::string>> required = {
        {"carebear.shaman", {"carebears.shamanSpots"}},
        {"snork.snork_in_black_boss", {"snorks.bossPerches", "snorks.bossZones", "snorks.sharkPath"}},
        {"lemmings.lemming", {"lemmings.cowBombX"}},
        {"lemmings.flying_lemming", {"lemmings.cowBombX"}},
        {"video.princess", {"video.princessWalk"}},
        {"video.rayman", {"video.raymanArena"}},
        {"videoboss.lara", {"videoboss.laraSpikes", "videoboss.laraWall"}},
        {"videoboss.weight", {"videoboss.laraWeightX", "videoboss.laraExitX"}},
        {"videoboss.small_mario", {"videoboss.arenaX"}},
        {"videoboss.luigi", {"videoboss.arenaX"}},
        {"videoboss.sonic", {"videoboss.arenaX"}},
        {"final.stork", {"finalboss.storkDropX"}},
        {"final.mario", {"finalboss.marioFloorY"}},
    };
    return required;
}

// The sizes of the level's banks ("" = none: 0).
std::size_t imageCount(const bb::core::assets::AssetSource& source, const std::string& name) {
    if (name.empty()) {
        return 0;
    }
    std::string error;
    const auto bank = source.spriteBank(name, error);
    REQUIRE_MESSAGE(bank.has_value(), (name + ": " + error));
    return bank->images.size();
}

std::size_t soundCount(const bb::core::assets::AssetSource& source, const std::string& name) {
    if (name.empty()) {
        return 0;
    }
    std::string error;
    const auto bank = source.soundBank(name, error);
    REQUIRE_MESSAGE(bank.has_value(), (name + ": " + error));
    return bank->sounds.size();
}

std::size_t musicCount(const bb::core::assets::AssetSource& source, const std::string& name) {
    if (name.empty()) {
        return 0;
    }
    std::string error;
    const auto list = source.musicList(name, error);
    REQUIRE_MESSAGE(list.has_value(), (name + ": " + error));
    return list->tracks.size();
}

void checkLevel(const bb::core::assets::AssetSource& source, const std::string& path,
                const bb::game::EntityRegistry& registry) {
    CAPTURE(path);
    std::string error;
    const auto level = source.level(path, error);
    REQUIRE_MESSAGE(level.has_value(), error);

    const std::size_t images = imageCount(source, level->assets[1]);
    const std::size_t sounds = soundCount(source, level->assets[3]);
    const std::size_t musics = musicCount(source, level->assets[4]);

    const auto registered = [&](EntityCategory category, int id) {
        const bb::game::EntityType* type = registry.find(category, id);
        if (type != nullptr) {
            const auto it = requiredMarkers().find(type->name);
            if (it != requiredMarkers().end()) {
                for (const std::string& marker : it->second) {
                    CAPTURE(marker);
                    CHECK(level->markers.count(marker) == 1);
                }
            }
        }
        return type != nullptr;
    };
    const auto index = [](int i, std::size_t count) { return i >= 0 && static_cast<std::size_t>(i) < count; };

    for (std::size_t i = 0; i < level->events.size(); ++i) {
        const LevelEventRecord& e = level->events[i];
        CAPTURE(i);
        CAPTURE(e.type);
        CAPTURE(e.id);
        CAPTURE(e.activationX);
        if (i > 0) {
            CHECK(level->events[i - 1].activationX >= e.activationX);
        }
        REQUIRE(e.type >= 0);
        REQUIRE(e.type <= static_cast<int>(LevelEventType::Pickup));
        switch (static_cast<LevelEventType>(e.type)) {
            case LevelEventType::Enemy:
            case LevelEventType::EnemyGenerator:
                CHECK(registered(EntityCategory::Enemy, e.id));
                break;
            case LevelEventType::AnimatedProp:
                CHECK(registered(EntityCategory::Prop, e.id));
                break;
            case LevelEventType::Pickup:
                CHECK(registered(EntityCategory::Pickup, e.id));
                break;
            case LevelEventType::Vehicle:
                CHECK(registered(EntityCategory::Vehicle, e.id));
                break;
            case LevelEventType::MidgroundDecal:
            case LevelEventType::Foreground:
                CHECK(index(e.id, images));
                break;
            case LevelEventType::Sound:
                CHECK(index(e.id, sounds));
                break;
            case LevelEventType::Music:
                CHECK(index(e.id, musics));
                break;
            default:
                break;
        }
    }
}

void checkCampaign(const bb::core::assets::AssetSource& source) {
    const bb::game::EntityRegistry registry = makeRegistry();
    std::string error;
    const auto campaign = source.campaign("data/bb.lst", error);
    REQUIRE_MESSAGE(campaign.has_value(), error);
    int levels = 0;
    for (const CampaignEntry& entry : campaign->entries) {
        if (entry.kind == CampaignEntry::Kind::Level) {
            checkLevel(source, entry.path, registry);
            ++levels;
        }
    }
    CHECK(levels == 12);
}

}  // namespace

TEST_CASE("every level of the 2002 data places registered entities and existing assets") {
    const bb::core::Vfs files(BB_TEST_DATA_DIR);
    const bb::core::assets::LegacyDataSource source(files);
    checkCampaign(source);
}

TEST_CASE("every level of the runtime pack places registered entities and existing assets") {
    std::string error;
    std::shared_ptr<const bb::core::assets::PackFile> pack =
        bb::core::assets::PackFile::open(BB_TEST_PACK, error);
    REQUIRE_MESSAGE(static_cast<bool>(pack), error);
    const bb::core::assets::PackSource source(pack);
    checkCampaign(source);
}

TEST_CASE("the levels have the same markers in the 2002 data and in the runtime pack") {
    const bb::core::Vfs files(BB_TEST_DATA_DIR);
    const bb::core::assets::LegacyDataSource data(files);
    std::string error;
    std::shared_ptr<const bb::core::assets::PackFile> pack =
        bb::core::assets::PackFile::open(BB_TEST_PACK, error);
    REQUIRE_MESSAGE(static_cast<bool>(pack), error);
    const bb::core::assets::PackSource packed(pack);
    const auto campaign = data.campaign("data/bb.lst", error);
    REQUIRE_MESSAGE(campaign.has_value(), error);
    int withMarkers = 0;
    for (const CampaignEntry& entry : campaign->entries) {
        if (entry.kind != CampaignEntry::Kind::Level) {
            continue;
        }
        CAPTURE(entry.path);
        const auto a = data.level(entry.path, error);
        const auto b = packed.level(entry.path, error);
        REQUIRE(a.has_value());
        REQUIRE(b.has_value());
        CHECK(a->markers == b->markers);
        withMarkers += a->markers.empty() ? 0 : 1;
    }
    CHECK(withMarkers == 8);
}
