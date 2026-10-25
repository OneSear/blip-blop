#pragma once

// A level being edited (bbedit): the source level (levels/<stem>.json,
// docs/formats/source-assets.md), its edits and their undo history. No
// user interface here: the editor's window calls these operations, and the
// tests check them.

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "bb/core/assets/asset_types.hpp"

namespace bb::editor {

using core::assets::LevelData;
using core::assets::LevelEventRecord;
using core::assets::LevelEventType;

// --- Events ----------------------------------------------------------------

// Whether an event of this type is placed at a point (an entity, a decal);
// the others act at a level position only, their activation X.
bool hasPosition(std::int32_t type) noexcept;

// The activation X the 2002 editor gave an event (BB3_level_editorDlg.cpp,
// OnButtonSave): from its x, its facing and, for an enemy generator, its
// temporary flag; the activation X itself for the events without a point.
// Every event of the shipped levels has this value.
std::int32_t activationFor(const LevelEventRecord& event) noexcept;

// The event's level position: its x, or its activation X.
std::int32_t positionX(const LevelEventRecord& event) noexcept;

// A new event of this type at a level position, with the 2002 editor's
// defaults (capacity 10, period 100 for a generator...).
LevelEventRecord newEvent(LevelEventType type, std::int32_t x, std::int32_t y);

// The names of the event types, as the source levels write them ("enemy").
const char* eventTypeName(std::int32_t type) noexcept;

// --- The document ----------------------------------------------------------

class LevelDocument {
public:
    // A level read from a source level file; nullopt and error on failure.
    static std::optional<LevelDocument> load(const std::string& path, std::string& error);
    // A level not saved yet (dirty until saved to path).
    static LevelDocument created(std::string path, LevelData data);
    // A new level: one screen of decor image 0, no platform, wall or event,
    // the players' starts at the left, the victory at the right; the assets
    // are taken from `like` (the level of the same world).
    static LevelData blankLevel(const LevelData& like);

    // The source level text of the current state.
    std::string text() const;
    // Writes it to path() (or another file); the document is then clean.
    bool save(std::string& error);
    bool saveAs(const std::string& path, std::string& error);

    const std::string& path() const noexcept { return path_; }
    const LevelData& data() const noexcept { return data_; }
    // Changed since loaded or saved.
    bool dirty() const noexcept { return revision_ != savedRevision_; }

    // --- Edits. Each one can be undone; the edits between beginGroup()
    //     and endGroup() (a brush stroke, a drag) are undone together.
    void beginGroup();
    void endGroup();

    // Adds an event; the events stay in descending activation order (the
    // order the game reads them in). Returns the new event's index.
    int addEvent(LevelEventRecord event);
    void removeEvent(int index);
    // Replaces an event, with its activation X recomputed. Returns its index
    // after reordering.
    int replaceEvent(int index, LevelEventRecord event);
    // Moves an event's point (or level position) to (x, y). Returns its
    // index after reordering.
    int moveEvent(int index, std::int32_t x, std::int32_t y);

    // Platform heights of columns [x1, x2] of a layer (kNoPlatform erases).
    void setPlatform(int layer, int x1, int x2, std::int32_t height);
    // One 8x8 wall cell (column, row) to 0 or 1, solid or blood walls.
    void setWall(bool blood, int column, int row, std::uint8_t value);

    void setStart(int player, std::int32_t x, std::int32_t y);
    void setVictory(std::int32_t x, std::int32_t flag1, std::int32_t value1, std::int32_t flag2,
                    std::int32_t value2);
    void setAsset(int index, const std::string& name);
    // The decor image of each 640-pixel screen. The level's platforms and
    // walls follow its width: columns past the end are dropped, new ones
    // have none.
    void setScreens(const std::vector<std::int32_t>& screens);
    void setMarkers(const core::assets::LevelMarkers& markers);

    bool canUndo() const noexcept { return !undo_.empty(); }
    bool canRedo() const noexcept { return !redo_.empty(); }
    bool undo();
    bool redo();

    // Steps of undo kept (the oldest are dropped).
    static constexpr std::size_t kUndoLimit = 200;

private:
    LevelDocument() = default;

    // Called before each change: saves the state for undo once per group.
    void change();
    int sortEvents(int follow);

    std::string path_;
    LevelData data_;
    std::vector<LevelData> undo_;
    std::vector<LevelData> redo_;
    int groupDepth_ = 0;
    bool groupSaved_ = false;
    std::uint64_t revision_ = 0;
    std::uint64_t savedRevision_ = 0;
    std::uint64_t nextRevision_ = 1;
    std::vector<std::uint64_t> undoRevisions_;
    std::vector<std::uint64_t> redoRevisions_;
};

}  // namespace bb::editor
