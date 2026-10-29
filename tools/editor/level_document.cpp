#include "level_document.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <numeric>
#include <sstream>
#include <utility>

#include "level_events.hpp"
#include "source_assets.hpp"

namespace bb::editor {

namespace fs = std::filesystem;
using core::assets::kEditorFill;
using core::assets::kEditorFillByte;
using tools::EventSlot;

// --- Events ----------------------------------------------------------------

bool hasPosition(std::int32_t type) noexcept {
    switch (static_cast<LevelEventType>(type)) {
        case LevelEventType::Enemy:
        case LevelEventType::EnemyGenerator:
        case LevelEventType::AnimatedProp:
        case LevelEventType::MidgroundDecal:
        case LevelEventType::Foreground:
        case LevelEventType::Vehicle:
        case LevelEventType::Pickup:
            return true;
        default:
            return false;
    }
}

std::int32_t activationFor(const LevelEventRecord& e) noexcept {
    switch (static_cast<LevelEventType>(e.type)) {
        case LevelEventType::Enemy:
            return e.value == 0 ? e.x - 750 : e.x + 50;
        case LevelEventType::EnemyGenerator:
            // Only a temporary generator facing right waits (the 2002 test
            // "sens == 0 || !tmp").
            return e.temporary == 1 && e.value != 0 ? e.x + 50 : e.x - 800;
        case LevelEventType::MidgroundDecal:
            return e.x - 660;
        case LevelEventType::AnimatedProp:
        case LevelEventType::Foreground:
        case LevelEventType::Vehicle:
        case LevelEventType::Pickup:
            return e.x - 740;
        default:
            return e.activationX;
    }
}

std::int32_t positionX(const LevelEventRecord& e) noexcept {
    return hasPosition(e.type) ? e.x : e.activationX;
}

const char* eventTypeName(std::int32_t type) noexcept {
    const auto& schemas = tools::eventSchemas();
    const auto it = schemas.find(type);
    return it == schemas.end() ? "?" : it->second.name;
}

LevelEventRecord newEvent(LevelEventType type, std::int32_t x, std::int32_t y) {
    // As the shipped levels: the slots a type does not use hold the 2002
    // editor's fill, and so do the padding and the file name.
    LevelEventRecord e;
    e.type = static_cast<std::int32_t>(type);
    e.id = e.x = e.bank = e.y = e.capacity = e.value = e.period = kEditorFill;
    e.temporary = kEditorFillByte;
    e.padding.fill(kEditorFillByte);
    e.file.fill(kEditorFillByte);

    const auto it = tools::eventSchemas().find(e.type);
    if (it != tools::eventSchemas().end()) {
        for (const tools::EventField& field : it->second.fields) {
            tools::slotRef(e, field.slot) = 0;
        }
    }
    // The 2002 editor's defaults (its parameter boxes' initial values).
    switch (type) {
        case LevelEventType::EnemyGenerator:
            e.capacity = 10;
            e.period = 100;
            e.temporary = 0;
            break;
        case LevelEventType::ScrollLock:
        case LevelEventType::HoldFire:
        case LevelEventType::Sound:
            e.value = 1;
            break;
        case LevelEventType::Overkill:
            e.capacity = 200;
            break;
        case LevelEventType::Music:
            e.y = 1;
            break;
        case LevelEventType::PickupGenerator:
            e.period = 500;
            break;
        default:
            break;
    }
    if (hasPosition(e.type)) {
        e.x = x;
        e.y = y;
        e.activationX = activationFor(e);
    } else {
        e.activationX = x;
    }
    return e;
}

// --- The document ----------------------------------------------------------

std::optional<LevelDocument> LevelDocument::load(const std::string& path, std::string& error) {
    std::ifstream in(fs::u8path(path), std::ios::binary);
    if (!in) {
        error = "cannot open " + path;
        return std::nullopt;
    }
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    tools::Json json;
    try {
        json = tools::Json::parse(text);
    } catch (const tools::Json::exception& ex) {
        error = path + ": " + ex.what();
        return std::nullopt;
    }
    auto level = tools::levelFromJson(json, error);
    if (!level) {
        error = path + ": " + error;
        return std::nullopt;
    }
    LevelDocument doc;
    doc.path_ = path;
    doc.data_ = std::move(*level);
    return doc;
}

LevelDocument LevelDocument::created(std::string path, LevelData data) {
    LevelDocument doc;
    doc.path_ = std::move(path);
    doc.data_ = std::move(data);
    doc.revision_ = doc.nextRevision_++;
    return doc;
}

LevelData LevelDocument::blankLevel(const LevelData& like) {
    LevelData level;
    level.assets = like.assets;
    level.screens = {0};
    for (auto& layer : level.platforms) {
        layer.assign(static_cast<std::size_t>(level.width()), LevelData::kNoPlatform);
    }
    const std::size_t cells = static_cast<std::size_t>(level.wallColumns()) * LevelData::kWallRows;
    level.solidWalls.assign(cells, 0);
    level.bloodWalls.assign(cells, 0);
    // As snuf1, the first level.
    level.start1X = 66;
    level.start1Y = 454;
    level.start2X = 147;
    level.start2Y = 454;
    level.victoryX = level.width() - 100;
    return level;
}

std::string LevelDocument::text() const {
    return tools::toJsonText(tools::levelToJson(data_));
}

bool LevelDocument::save(std::string& error) {
    return saveAs(path_, error);
}

bool LevelDocument::saveAs(const std::string& path, std::string& error) {
    const std::string contents = text();
    // Written next to the file first, so a failure leaves it as it was.
    const fs::path target = fs::u8path(path);
    fs::path temporary = target;
    temporary += ".saving";
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        out.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        if (!out) {
            error = "cannot write " + temporary.u8string();
            return false;
        }
    }
    std::error_code ec;
    fs::rename(temporary, target, ec);
    if (ec) {
        error = "cannot replace " + path + ": " + ec.message();
        return false;
    }
    path_ = path;
    savedRevision_ = revision_;
    return true;
}

void LevelDocument::beginGroup() {
    if (groupDepth_++ == 0) {
        groupSaved_ = false;
    }
}

void LevelDocument::endGroup() {
    if (groupDepth_ > 0) {
        --groupDepth_;
    }
}

void LevelDocument::change() {
    if (groupDepth_ > 0 && groupSaved_) {
        return;
    }
    groupSaved_ = groupDepth_ > 0;
    undo_.push_back(data_);
    undoRevisions_.push_back(revision_);
    if (undo_.size() > kUndoLimit) {
        undo_.erase(undo_.begin());
        undoRevisions_.erase(undoRevisions_.begin());
    }
    redo_.clear();
    redoRevisions_.clear();
    revision_ = nextRevision_++;
}

bool LevelDocument::undo() {
    if (undo_.empty()) {
        return false;
    }
    redo_.push_back(std::move(data_));
    redoRevisions_.push_back(revision_);
    data_ = std::move(undo_.back());
    revision_ = undoRevisions_.back();
    undo_.pop_back();
    undoRevisions_.pop_back();
    return true;
}

bool LevelDocument::redo() {
    if (redo_.empty()) {
        return false;
    }
    undo_.push_back(std::move(data_));
    undoRevisions_.push_back(revision_);
    data_ = std::move(redo_.back());
    revision_ = redoRevisions_.back();
    redo_.pop_back();
    redoRevisions_.pop_back();
    return true;
}

// Descending activation X, keeping the order of equal ones (the shipped
// files are already in this order, so loading and saving changes nothing).
// Returns the new index of the event at `follow` (-1: none).
int LevelDocument::sortEvents(int follow) {
    std::vector<int> order(data_.events.size());
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [this](int a, int b) {
        return data_.events[static_cast<std::size_t>(a)].activationX >
               data_.events[static_cast<std::size_t>(b)].activationX;
    });
    std::vector<LevelEventRecord> sorted;
    sorted.reserve(order.size());
    int followed = -1;
    for (std::size_t i = 0; i < order.size(); ++i) {
        if (order[i] == follow) {
            followed = static_cast<int>(i);
        }
        sorted.push_back(data_.events[static_cast<std::size_t>(order[i])]);
    }
    data_.events = std::move(sorted);
    return followed;
}

int LevelDocument::addEvent(LevelEventRecord event) {
    change();
    event.activationX = activationFor(event);
    data_.events.push_back(event);
    return sortEvents(static_cast<int>(data_.events.size()) - 1);
}

void LevelDocument::removeEvent(int index) {
    if (index < 0 || index >= static_cast<int>(data_.events.size())) {
        return;
    }
    change();
    data_.events.erase(data_.events.begin() + index);
}

int LevelDocument::replaceEvent(int index, LevelEventRecord event) {
    if (index < 0 || index >= static_cast<int>(data_.events.size())) {
        return -1;
    }
    if (data_.events[static_cast<std::size_t>(index)] == event) {
        return index;
    }
    change();
    event.activationX = activationFor(event);
    data_.events[static_cast<std::size_t>(index)] = event;
    return sortEvents(index);
}

int LevelDocument::moveEvent(int index, std::int32_t x, std::int32_t y) {
    if (index < 0 || index >= static_cast<int>(data_.events.size())) {
        return -1;
    }
    LevelEventRecord e = data_.events[static_cast<std::size_t>(index)];
    if (hasPosition(e.type)) {
        e.x = x;
        e.y = y;
    } else {
        e.activationX = x;
    }
    return replaceEvent(index, e);
}

void LevelDocument::setPlatform(int layer, int x1, int x2, std::int32_t height) {
    if (layer < 0 || layer >= LevelData::kPlatformLayers) {
        return;
    }
    auto& columns = data_.platforms[static_cast<std::size_t>(layer)];
    const int last = static_cast<int>(columns.size()) - 1;
    x1 = std::max(x1, 0);
    x2 = std::min(x2, last);
    bool changes = false;
    for (int x = x1; x <= x2 && !changes; ++x) {
        changes = columns[static_cast<std::size_t>(x)] != height;
    }
    if (!changes) {
        return;
    }
    change();
    std::fill(columns.begin() + x1, columns.begin() + x2 + 1, height);
}

void LevelDocument::setWall(bool blood, int column, int row, std::uint8_t value) {
    const int columns = data_.wallColumns();
    if (column < 0 || column >= columns || row < 0 || row >= LevelData::kWallRows) {
        return;
    }
    auto& cells = blood ? data_.bloodWalls : data_.solidWalls;
    const auto i = static_cast<std::size_t>(row) * static_cast<std::size_t>(columns) + static_cast<std::size_t>(column);
    if (cells[i] == value) {
        return;
    }
    change();
    cells[i] = value;
}

void LevelDocument::setStart(int player, std::int32_t x, std::int32_t y) {
    std::int32_t& sx = player == 0 ? data_.start1X : data_.start2X;
    std::int32_t& sy = player == 0 ? data_.start1Y : data_.start2Y;
    if (sx == x && sy == y) {
        return;
    }
    change();
    sx = x;
    sy = y;
}

void LevelDocument::setVictory(std::int32_t x, std::int32_t flag1, std::int32_t value1, std::int32_t flag2,
                               std::int32_t value2) {
    if (data_.victoryX == x && data_.victoryFlag1 == flag1 && data_.victoryValue1 == value1 &&
        data_.victoryFlag2 == flag2 && data_.victoryValue2 == value2) {
        return;
    }
    change();
    data_.victoryX = x;
    data_.victoryFlag1 = flag1;
    data_.victoryValue1 = value1;
    data_.victoryFlag2 = flag2;
    data_.victoryValue2 = value2;
}

void LevelDocument::setAsset(int index, const std::string& name) {
    if (index < 0 || index >= LevelData::kAssetCount || data_.assets[static_cast<std::size_t>(index)] == name) {
        return;
    }
    change();
    data_.assets[static_cast<std::size_t>(index)] = name;
}

void LevelDocument::setScreens(const std::vector<std::int32_t>& screens) {
    if (screens == data_.screens || screens.empty()) {
        return;
    }
    change();
    const int oldColumns = data_.wallColumns();
    data_.screens = screens;
    const auto width = static_cast<std::size_t>(data_.width());
    for (auto& layer : data_.platforms) {
        layer.resize(width, LevelData::kNoPlatform);
    }
    const int newColumns = data_.wallColumns();
    for (auto* cells : {&data_.solidWalls, &data_.bloodWalls}) {
        std::vector<std::uint8_t> resized(static_cast<std::size_t>(newColumns) * LevelData::kWallRows, 0);
        const int kept = std::min(oldColumns, newColumns);
        for (int row = 0; row < LevelData::kWallRows; ++row) {
            std::copy_n(cells->begin() + static_cast<std::ptrdiff_t>(row) * oldColumns, kept,
                        resized.begin() + static_cast<std::ptrdiff_t>(row) * newColumns);
        }
        *cells = std::move(resized);
    }
}

void LevelDocument::setMarkers(const core::assets::LevelMarkers& markers) {
    if (markers == data_.markers) {
        return;
    }
    change();
    data_.markers = markers;
}

}  // namespace bb::editor
