// The level editor's document (tools/editor/level_document.hpp).

#include <doctest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "level_document.hpp"
#include "level_events.hpp"
#include "json_text.hpp"

namespace fs = std::filesystem;
using bb::editor::LevelDocument;
using bb::editor::LevelEventRecord;
using bb::editor::LevelEventType;

namespace {

std::vector<fs::path> sourceLevels() {
    std::vector<fs::path> levels;
    for (const auto& entry : fs::directory_iterator(fs::path(BB_TEST_SOURCE_DIR) / "levels")) {
        if (entry.path().extension() == ".json") {
            levels.push_back(entry.path());
        }
    }
    return levels;
}

std::string fileText(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

LevelDocument loadLevel(const char* stem) {
    std::string error;
    auto doc = LevelDocument::load((fs::path(BB_TEST_SOURCE_DIR) / "levels" / stem).u8string(), error);
    REQUIRE_MESSAGE(doc.has_value(), error);
    return std::move(*doc);
}

bool inGameOrder(const LevelDocument& doc) {
    const auto& events = doc.data().events;
    for (std::size_t i = 1; i < events.size(); ++i) {
        if (events[i - 1].activationX < events[i].activationX) {
            return false;
        }
    }
    return true;
}

}  // namespace

TEST_CASE("every source level loads and saves unchanged, byte for byte") {
    const auto levels = sourceLevels();
    REQUIRE(levels.size() == 12);
    for (const fs::path& path : levels) {
        CAPTURE(path.u8string());
        std::string error;
        const auto doc = LevelDocument::load(path.u8string(), error);
        REQUIRE_MESSAGE(doc.has_value(), error);
        CHECK_FALSE(doc->dirty());
        CHECK(doc->text() == fileText(path));
    }
}

TEST_CASE("the 2002 activation rules give every shipped event its activation X") {
    int events = 0;
    for (const fs::path& path : sourceLevels()) {
        std::string error;
        const auto doc = LevelDocument::load(path.u8string(), error);
        REQUIRE(doc.has_value());
        for (const LevelEventRecord& e : doc->data().events) {
            CAPTURE(path.filename().u8string());
            CAPTURE(bb::editor::eventTypeName(e.type));
            CHECK(bb::editor::activationFor(e) == e.activationX);
            ++events;
        }
        CHECK(inGameOrder(*doc));
    }
    CHECK(events > 1000);
}

TEST_CASE("activation rules by event type") {
    auto e = bb::editor::newEvent(LevelEventType::Enemy, 2000, 400);
    CHECK(e.activationX == 1250);  // facing left: x - 750
    e.value = 1;
    CHECK(bb::editor::activationFor(e) == 2050);  // facing right: x + 50

    auto g = bb::editor::newEvent(LevelEventType::EnemyGenerator, 2000, 400);
    g.value = 1;
    CHECK(bb::editor::activationFor(g) == 1200);  // not temporary: x - 800 either way
    g.temporary = 1;
    CHECK(bb::editor::activationFor(g) == 2050);

    CHECK(bb::editor::newEvent(LevelEventType::MidgroundDecal, 2000, 0).activationX == 1340);
    for (auto type : {LevelEventType::AnimatedProp, LevelEventType::Foreground, LevelEventType::Vehicle,
                      LevelEventType::Pickup}) {
        CHECK(bb::editor::newEvent(type, 2000, 0).activationX == 1260);
    }
    CHECK(bb::editor::newEvent(LevelEventType::ScrollLock, 2000, 0).activationX == 2000);
}

TEST_CASE("new events write only their named fields, like the shipped ones") {
    LevelDocument doc = loadLevel("snuf1.json");
    const std::string shipped = doc.text();
    for (std::int32_t type = 0; type <= 18; ++type) {
        CAPTURE(type);
        const LevelEventRecord e = bb::editor::newEvent(static_cast<LevelEventType>(type), 1000 + type, 300);
        // Every named field is set (the 2002 editor left some at its fill,
        // which the source levels write as null).
        for (const bb::tools::EventField& field : bb::tools::eventSchemas().at(type).fields) {
            CAPTURE(field.name);
            CHECK(bb::tools::slotValue(e, field.slot) != bb::core::assets::kEditorFill);
        }
        doc.addEvent(e);
    }
    // No slot outside the named fields is written ("_id": ...), as in the
    // shipped levels.
    CHECK(shipped.find("\"_") == std::string::npos);
    CHECK(doc.text().find("\"_") == std::string::npos);
    CHECK(inGameOrder(doc));

    // And they read back the same.
    const fs::path file = fs::temp_directory_path() / "bb_editor_test_level.json";
    std::string error;
    REQUIRE(doc.saveAs(file.u8string(), error));
    const auto again = LevelDocument::load(file.u8string(), error);
    REQUIRE_MESSAGE(again.has_value(), error);
    CHECK(again->data().events == doc.data().events);
    fs::remove(file);
}

TEST_CASE("moving an event keeps the game's order and follows the event") {
    LevelDocument doc = loadLevel("snuf1.json");
    const auto& events = doc.data().events;
    REQUIRE(events.size() > 10);
    // The last event (smallest activation X) of a positioned type, moved to
    // the far end of the level.
    int index = static_cast<int>(events.size()) - 1;
    while (!bb::editor::hasPosition(events[static_cast<std::size_t>(index)].type)) {
        --index;
    }
    const LevelEventRecord before = events[static_cast<std::size_t>(index)];
    const int moved = doc.moveEvent(index, doc.data().width() - 10, before.y);
    REQUIRE(moved >= 0);
    CHECK(events[static_cast<std::size_t>(moved)].x == doc.data().width() - 10);
    CHECK(events[static_cast<std::size_t>(moved)].activationX ==
          bb::editor::activationFor(events[static_cast<std::size_t>(moved)]));
    CHECK(moved < index);
    CHECK(inGameOrder(doc));
}

TEST_CASE("undo and redo, groups and the saved state") {
    LevelDocument doc = loadLevel("snuf1.json");
    const auto original = doc.data().platforms[2];

    doc.beginGroup();
    doc.setPlatform(2, 100, 110, 300);
    doc.setPlatform(2, 111, 120, 301);
    doc.endGroup();
    CHECK(doc.dirty());
    CHECK(doc.data().platforms[2][100] == 300);
    CHECK(doc.data().platforms[2][120] == 301);

    doc.setWall(false, 3, 4, 1);
    REQUIRE(doc.undo());  // the wall
    CHECK(doc.data().platforms[2][120] == 301);
    REQUIRE(doc.undo());  // the whole stroke
    CHECK(doc.data().platforms[2] == original);
    CHECK_FALSE(doc.dirty());
    CHECK_FALSE(doc.canUndo());

    REQUIRE(doc.redo());
    CHECK(doc.data().platforms[2][100] == 300);
    CHECK(doc.dirty());

    // An edit that changes nothing is not a step.
    doc.setStart(0, doc.data().start1X, doc.data().start1Y);
    REQUIRE(doc.undo());
    CHECK_FALSE(doc.dirty());
}

TEST_CASE("changing the screens resizes the platforms and keeps the walls in place") {
    LevelDocument doc = loadLevel("snuf1.json");
    const int screens = static_cast<int>(doc.data().screens.size());
    const int columns = doc.data().wallColumns();
    doc.setWall(true, columns - 1, 59, 1);  // last cell of the level
    doc.setWall(true, 5, 30, 1);

    std::vector<std::int32_t> longer = doc.data().screens;
    longer.push_back(0);
    doc.setScreens(longer);
    CHECK(doc.data().width() == (screens + 1) * 640);
    for (const auto& layer : doc.data().platforms) {
        CHECK(layer.size() == static_cast<std::size_t>(doc.data().width()));
        CHECK(layer.back() == bb::core::assets::LevelData::kNoPlatform);
    }
    const int newColumns = doc.data().wallColumns();
    CHECK(doc.data().bloodWalls.size() == static_cast<std::size_t>(newColumns) * 60);
    CHECK(doc.data().bloodWalls[59 * static_cast<std::size_t>(newColumns) + static_cast<std::size_t>(columns) - 1] == 1);
    CHECK(doc.data().bloodWalls[30 * static_cast<std::size_t>(newColumns) + 5] == 1);

    REQUIRE(doc.undo());
    REQUIRE(doc.undo());
    REQUIRE(doc.undo());
    CHECK_FALSE(doc.dirty());
    CHECK(doc.text() == fileText(fs::path(BB_TEST_SOURCE_DIR) / "levels" / "snuf1.json"));
}

TEST_CASE("the sprite manifests the editor rewrites come out unchanged when not edited") {
    // bbedit's sprite window saves sprites.json through the same writer.
    int manifests = 0;
    for (const auto& entry : fs::directory_iterator(fs::path(BB_TEST_SOURCE_DIR) / "sprites")) {
        const fs::path file = entry.path() / "sprites.json";
        if (!fs::exists(file)) {
            continue;
        }
        CAPTURE(file.u8string());
        const std::string text = fileText(file);
        CHECK(bb::tools::toJsonText(bb::tools::Json::parse(text)) == text);
        ++manifests;
    }
    CHECK(manifests > 50);
}
