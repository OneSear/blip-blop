#include "source_assets.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>

#include "bb/core/assets/asset_source.hpp"
#include "bb/core/assets/text_encoding.hpp"
#include "bb/core/vfs.hpp"
#include "bb/game/content.hpp"
#include "bb/game/entity_registry.hpp"
#include "level_events.hpp"
#include "png_io.hpp"

namespace bb::tools {

namespace fs = std::filesystem;
using core::assets::CampaignEntry;
using core::assets::Image16;
using core::assets::kEditorFill;
using core::assets::kEditorFillByte;
using core::assets::LevelEventRecord;
using core::assets::MusicTrack;
using core::assets::PixelFormat16;
using core::assets::SoundSample;
using core::assets::SpriteImage;
using Slot = EventSlot;
using Entity = EventEntity;

namespace {

constexpr int kFormatVersion = 1;

std::string layoutName(PixelFormat16 f) {
    return f == PixelFormat16::Rgb565 ? "rgb565" : "rgb555";
}

bool layoutFromName(const std::string& s, PixelFormat16& f) {
    if (s == "rgb565") {
        f = PixelFormat16::Rgb565;
        return true;
    }
    if (s == "rgb555") {
        f = PixelFormat16::Rgb555;
        return true;
    }
    return false;
}

Json header(const char* format) {
    Json j;
    j["format"] = format;
    j["version"] = kFormatVersion;
    return j;
}

bool checkHeader(const Json& j, const char* format, std::string& error) {
    if (!j.is_object() || j.value("format", "") != format || j.value("version", 0) != kFormatVersion) {
        error = std::string("not a ") + format + " file, version " + std::to_string(kFormatVersion);
        return false;
    }
    return true;
}

std::optional<Bytes> readBytes(const fs::path& path, std::string& error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        error = "cannot read " + path.string();
        return std::nullopt;
    }
    std::error_code ec;
    const auto size = fs::file_size(path, ec);
    Bytes bytes(static_cast<std::size_t>(size));
    in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!in) {
        error = "cannot read " + path.string();
        return std::nullopt;
    }
    return bytes;
}

std::optional<Json> readJson(const fs::path& path, std::string& error) {
    const auto bytes = readBytes(path, error);
    if (!bytes) {
        return std::nullopt;
    }
    Json j = Json::parse(bytes->begin(), bytes->end(), nullptr, false);
    if (j.is_discarded()) {
        error = path.string() + ": invalid JSON";
        return std::nullopt;
    }
    return j;
}

template <typename T>
bool getInt(const Json& j, const char* key, T& out, std::string& error) {
    if (!j.contains(key) || !j[key].is_number_integer()) {
        error = std::string("missing integer \"") + key + "\"";
        return false;
    }
    out = j[key].get<T>();
    return true;
}

// --- Level events ----------------------------------------------------------

std::string hex(const std::uint8_t* p, std::size_t n) {
    static const char digits[] = "0123456789abcdef";
    std::string s;
    for (std::size_t i = 0; i < n; ++i) {
        s.push_back(digits[p[i] >> 4]);
        s.push_back(digits[p[i] & 15]);
    }
    return s;
}

bool unhex(const std::string& s, std::uint8_t* out, std::size_t n) {
    if (s.size() != 2 * n) {
        return false;
    }
    const auto digit = [](char c) -> int {
        if (c >= '0' && c <= '9') {
            return c - '0';
        }
        if (c >= 'a' && c <= 'f') {
            return c - 'a' + 10;
        }
        return -1;
    };
    for (std::size_t i = 0; i < n; ++i) {
        const int hi = digit(s[2 * i]);
        const int lo = digit(s[2 * i + 1]);
        if (hi < 0 || lo < 0) {
            return false;
        }
        out[i] = static_cast<std::uint8_t>(hi * 16 + lo);
    }
    return true;
}

template <std::size_t N>
bool allFill(const std::array<std::uint8_t, N>& a) {
    return std::all_of(a.begin(), a.end(), [](std::uint8_t b) { return b == kEditorFillByte; });
}

Json eventToJson(LevelEventRecord e) {
    Json j;
    const auto it = eventSchemas().find(e.type);
    if (it != eventSchemas().end()) {
        j["type"] = it->second.name;
    } else {
        j["type"] = e.type;
    }
    j["activationX"] = e.activationX;
    std::array<bool, 7> named{};
    if (it != eventSchemas().end()) {
        for (const EventField& f : it->second.fields) {
            const std::int32_t v = slotRef(e, f.slot);
            const bb::game::EntityType* type = f.entity == Entity::None || v == kEditorFill
                                                   ? nullptr
                                                   : entityRegistry().find(categoryOf(f.entity), v);
            j[f.name] = v == kEditorFill ? Json(nullptr) : type != nullptr ? Json(type->name) : Json(v);
            named[static_cast<std::size_t>(f.slot)] = true;
        }
        if (it->second.temporary && (e.temporary == 0 || e.temporary == 1)) {
            j["temporary"] = e.temporary == 1;
            e.temporary = kEditorFillByte;  // stored
        }
    }
    // Values no field names, kept for an exact round trip to the 2002 file.
    for (std::size_t s = 0; s < kSlotNames.size(); ++s) {
        const std::int32_t v = slotRef(e, static_cast<Slot>(s));
        if (!named[s] && v != kEditorFill) {
            j[std::string("_") + kSlotNames[s]] = v;
        }
    }
    if (e.temporary != kEditorFillByte) {
        j["_temporary"] = e.temporary;
    }
    if (!allFill(e.padding)) {
        j["_padding"] = hex(e.padding.data(), e.padding.size());
    }
    if (!allFill(e.file)) {
        j["_file"] = hex(e.file.data(), e.file.size());
    }
    return j;
}

std::optional<LevelEventRecord> eventFromJson(const Json& j, std::string& error) {
    LevelEventRecord e;
    e.id = e.x = e.bank = e.y = e.capacity = e.value = e.period = kEditorFill;
    e.temporary = kEditorFillByte;
    e.padding.fill(kEditorFillByte);
    e.file.fill(kEditorFillByte);
    if (!j.is_object() || !j.contains("type")) {
        error = "event without a type";
        return std::nullopt;
    }
    const EventSchema* schema = nullptr;
    if (j["type"].is_string()) {
        for (const auto& [type, s] : eventSchemas()) {
            if (j["type"] == s.name) {
                e.type = type;
                schema = &s;
            }
        }
        if (schema == nullptr) {
            error = "unknown event type " + j["type"].dump();
            return std::nullopt;
        }
    } else if (!getInt(j, "type", e.type, error)) {
        return std::nullopt;
    }
    if (!getInt(j, "activationX", e.activationX, error)) {
        return std::nullopt;
    }
    std::set<std::string> known = {"type", "activationX", "_temporary", "_padding", "_file"};
    if (schema != nullptr) {
        for (const EventField& f : schema->fields) {
            known.insert(f.name);
            if (!j.contains(f.name)) {
                error = std::string(schema->name) + " event without \"" + f.name + "\"";
                return std::nullopt;
            }
            if (f.entity != Entity::None && j[f.name].is_string()) {
                const std::string name = j[f.name].get<std::string>();
                const bb::game::EntityType* type = entityRegistry().find(name);
                if (type == nullptr || type->category != categoryOf(f.entity)) {
                    error = std::string(schema->name) + " event: unknown " + f.name + " \"" + name + "\"";
                    return std::nullopt;
                }
                slotRef(e, f.slot) = type->legacyId;
            } else if (!j[f.name].is_null() && !getInt(j, f.name, slotRef(e, f.slot), error)) {
                return std::nullopt;
            }
        }
        if (schema->temporary && j.contains("temporary")) {
            known.insert("temporary");
            e.temporary = j["temporary"].get<bool>() ? 1 : 0;
        }
    }
    for (std::size_t s = 0; s < kSlotNames.size(); ++s) {
        const std::string key = std::string("_") + kSlotNames[s];
        known.insert(key);
        if (j.contains(key) && !getInt(j, key.c_str(), slotRef(e, static_cast<Slot>(s)), error)) {
            return std::nullopt;
        }
    }
    if (j.contains("_temporary") && !getInt(j, "_temporary", e.temporary, error)) {
        return std::nullopt;
    }
    if (j.contains("_padding") &&
        !unhex(j["_padding"].get<std::string>(), e.padding.data(), e.padding.size())) {
        error = "bad _padding";
        return std::nullopt;
    }
    if (j.contains("_file") && !unhex(j["_file"].get<std::string>(), e.file.data(), e.file.size())) {
        error = "bad _file";
        return std::nullopt;
    }
    for (auto it = j.begin(); it != j.end(); ++it) {
        if (known.count(it.key()) == 0) {
            error = "unknown event field \"" + it.key() + "\"";
            return std::nullopt;
        }
    }
    return e;
}

constexpr std::array<const char*, 7> kLevelAssetKeys = {"decor", "props",    "enemies",  "sounds",
                                                        "music", "dialogue", "portraits"};
constexpr std::array<const char*, 6> kLevelKinds = {"bonus", "first",    "intermediate",
                                                    "last",  "complete", "end"};

}  // namespace

// ---------------------------------------------------------------------------
// Levels

Json levelToJson(const LevelData& level) {
    Json j = header("blipblop-level");
    Json assets;
    for (std::size_t i = 0; i < kLevelAssetKeys.size(); ++i) {
        assets[kLevelAssetKeys[i]] = level.assets[i];
    }
    j["assets"] = assets;
    j["screens"] = level.screens;
    j["start"] = Json::array(
        {Json::array({level.start1X, level.start1Y}), Json::array({level.start2X, level.start2Y})});
    j["victory"] = Json{{"x", level.victoryX},
                        {"flag1", level.victoryFlag1},
                        {"value1", level.victoryValue1},
                        {"flag2", level.victoryFlag2},
                        {"value2", level.victoryValue2}};
    Json platforms = Json::array();
    for (const auto& layer : level.platforms) {
        Json runs = Json::array();
        for (std::size_t x = 0; x < layer.size();) {
            std::size_t n = 1;
            while (x + n < layer.size() && layer[x + n] == layer[x]) {
                ++n;
            }
            if (layer[x] != LevelData::kNoPlatform) {
                runs.push_back(Json::array({x, n, layer[x]}));
            }
            x += n;
        }
        platforms.push_back(runs);
    }
    j["platforms"] = platforms;
    const std::size_t columns = static_cast<std::size_t>(level.wallColumns());
    for (const auto& [key, walls] :
         {std::pair<const char*, const std::vector<std::uint8_t>*>{"solidWalls", &level.solidWalls},
          {"bloodWalls", &level.bloodWalls}}) {
        Json runs = Json::array();
        for (std::size_t row = 0; row < static_cast<std::size_t>(LevelData::kWallRows); ++row) {
            const std::uint8_t* cells = walls->data() + row * columns;
            for (std::size_t x = 0; x < columns;) {
                std::size_t n = 1;
                while (x + n < columns && cells[x + n] == cells[x]) {
                    ++n;
                }
                if (cells[x] == 1) {
                    runs.push_back(Json::array({row, x, n}));
                } else if (cells[x] != 0) {
                    runs.push_back(Json::array({row, x, n, cells[x]}));
                }
                x += n;
            }
        }
        j[key] = runs;
    }
    Json events = Json::array();
    for (const LevelEventRecord& e : level.events) {
        events.push_back(eventToJson(e));
    }
    j["events"] = events;
    if (!level.markers.empty()) {
        Json markers = Json::object();
        for (const auto& [name, values] : level.markers) {
            markers[name] = values;
        }
        j["markers"] = markers;
    }
    return j;
}

std::optional<LevelData> levelFromJson(const Json& j, std::string& error) {
    if (!checkHeader(j, "blipblop-level", error)) {
        return std::nullopt;
    }
    LevelData level;
    try {
        for (std::size_t i = 0; i < kLevelAssetKeys.size(); ++i) {
            level.assets[i] = j.at("assets").at(kLevelAssetKeys[i]).get<std::string>();
        }
        level.screens = j.at("screens").get<std::vector<std::int32_t>>();
        if (level.screens.empty()) {
            error = "a level has at least one screen";
            return std::nullopt;
        }
        level.start1X = j.at("start").at(0).at(0).get<std::int32_t>();
        level.start1Y = j.at("start").at(0).at(1).get<std::int32_t>();
        level.start2X = j.at("start").at(1).at(0).get<std::int32_t>();
        level.start2Y = j.at("start").at(1).at(1).get<std::int32_t>();
        const Json& v = j.at("victory");
        level.victoryX = v.at("x").get<std::int32_t>();
        level.victoryFlag1 = v.at("flag1").get<std::int32_t>();
        level.victoryValue1 = v.at("value1").get<std::int32_t>();
        level.victoryFlag2 = v.at("flag2").get<std::int32_t>();
        level.victoryValue2 = v.at("value2").get<std::int32_t>();

        const std::size_t width = static_cast<std::size_t>(level.width());
        const Json& platforms = j.at("platforms");
        if (platforms.size() != LevelData::kPlatformLayers) {
            error = "a level has 8 platform layers";
            return std::nullopt;
        }
        for (std::size_t l = 0; l < LevelData::kPlatformLayers; ++l) {
            auto& layer = level.platforms[l];
            layer.assign(width, LevelData::kNoPlatform);
            for (const Json& run : platforms[l]) {
                const auto x = run.at(0).get<std::size_t>();
                const auto n = run.at(1).get<std::size_t>();
                if (x + n > width) {
                    error = "platform run past the end of the level";
                    return std::nullopt;
                }
                std::fill(layer.begin() + static_cast<std::ptrdiff_t>(x),
                          layer.begin() + static_cast<std::ptrdiff_t>(x + n), run.at(2).get<std::int32_t>());
            }
        }
        const std::size_t columns = static_cast<std::size_t>(level.wallColumns());
        for (const auto& [key, walls] :
             {std::pair<const char*, std::vector<std::uint8_t>*>{"solidWalls", &level.solidWalls},
              {"bloodWalls", &level.bloodWalls}}) {
            walls->assign(columns * static_cast<std::size_t>(LevelData::kWallRows), 0);
            for (const Json& run : j.at(key)) {
                const auto row = run.at(0).get<std::size_t>();
                const auto x = run.at(1).get<std::size_t>();
                const auto n = run.at(2).get<std::size_t>();
                const auto value = run.size() > 3 ? run.at(3).get<std::uint8_t>() : std::uint8_t{1};
                if (row >= static_cast<std::size_t>(LevelData::kWallRows) || x + n > columns) {
                    error = std::string(key) + " run outside the level";
                    return std::nullopt;
                }
                std::fill(walls->begin() + static_cast<std::ptrdiff_t>(row * columns + x),
                          walls->begin() + static_cast<std::ptrdiff_t>(row * columns + x + n), value);
            }
        }
        for (const Json& event : j.at("events")) {
            auto e = eventFromJson(event, error);
            if (!e) {
                error = "event " + std::to_string(level.events.size()) + ": " + error;
                return std::nullopt;
            }
            level.events.push_back(*e);
        }
        if (j.contains("markers")) {
            for (auto it = j.at("markers").begin(); it != j.at("markers").end(); ++it) {
                level.markers[it.key()] = it.value().get<std::vector<std::int32_t>>();
            }
        }
    } catch (const nlohmann::json::exception& ex) {
        error = ex.what();
        return std::nullopt;
    }
    return level;
}

// ---------------------------------------------------------------------------
// Strings, campaign, playlists

Json stringsToJson(const StringTable& table) {
    Json j = header("blipblop-strings");
    Json strings = Json::array();
    for (const auto& [id, text] : table.strings) {
        strings.push_back(Json{{"id", id}, {"text", text}});
    }
    j["strings"] = strings;
    return j;
}

std::optional<StringTable> stringsFromJson(const Json& j, std::string& error) {
    if (!checkHeader(j, "blipblop-strings", error)) {
        return std::nullopt;
    }
    StringTable table;
    try {
        for (const Json& s : j.at("strings")) {
            const int id = s.at("id").get<int>();
            if (!table.strings.emplace(id, s.at("text").get<std::string>()).second) {
                error = "string " + std::to_string(id) + " defined twice";
                return std::nullopt;
            }
        }
    } catch (const nlohmann::json::exception& ex) {
        error = ex.what();
        return std::nullopt;
    }
    return table;
}

Json campaignToJson(const Campaign& campaign) {
    Json j = header("blipblop-campaign");
    Json entries = Json::array();
    for (const CampaignEntry& e : campaign.entries) {
        Json entry;
        switch (e.kind) {
            case CampaignEntry::Kind::Cutscene:
                entry["cutscene"] = e.path;
                break;
            case CampaignEntry::Kind::Briefing:
                entry["briefing"] = e.path;
                break;
            case CampaignEntry::Kind::Level:
                entry["level"] = e.path;
                if (e.levelKind >= 0 && static_cast<std::size_t>(e.levelKind) < kLevelKinds.size()) {
                    entry["kind"] = kLevelKinds[static_cast<std::size_t>(e.levelKind)];
                } else {
                    entry["kind"] = e.levelKind;
                }
                break;
        }
        entries.push_back(entry);
    }
    j["entries"] = entries;
    return j;
}

std::optional<Campaign> campaignFromJson(const Json& j, std::string& error) {
    if (!checkHeader(j, "blipblop-campaign", error)) {
        return std::nullopt;
    }
    Campaign campaign;
    try {
        for (const Json& e : j.at("entries")) {
            CampaignEntry entry;
            if (e.contains("cutscene")) {
                entry.kind = CampaignEntry::Kind::Cutscene;
                entry.path = e["cutscene"].get<std::string>();
            } else if (e.contains("briefing")) {
                entry.kind = CampaignEntry::Kind::Briefing;
                entry.path = e["briefing"].get<std::string>();
            } else {
                entry.kind = CampaignEntry::Kind::Level;
                entry.path = e.at("level").get<std::string>();
                const Json& kind = e.at("kind");
                if (kind.is_number_integer()) {
                    entry.levelKind = kind.get<std::int32_t>();
                } else {
                    const auto it =
                        std::find(kLevelKinds.begin(), kLevelKinds.end(), kind.get<std::string>());
                    if (it == kLevelKinds.end()) {
                        error = "unknown level kind " + kind.dump();
                        return std::nullopt;
                    }
                    entry.levelKind = static_cast<std::int32_t>(it - kLevelKinds.begin());
                }
            }
            campaign.entries.push_back(entry);
        }
    } catch (const nlohmann::json::exception& ex) {
        error = ex.what();
        return std::nullopt;
    }
    return campaign;
}

Json playlistToJson(const MusicList& list) {
    Json j = header("blipblop-playlist");
    Json tracks = Json::array();
    for (const MusicTrack& t : list.tracks) {
        Json track{{"file", t.path}};
        if (t.type != 1) {
            track["type"] = t.type;
        }
        tracks.push_back(track);
    }
    j["tracks"] = tracks;
    return j;
}

std::optional<MusicList> playlistFromJson(const Json& j, std::string& error) {
    if (!checkHeader(j, "blipblop-playlist", error)) {
        return std::nullopt;
    }
    MusicList list;
    try {
        for (const Json& t : j.at("tracks")) {
            MusicTrack track;
            track.path = t.at("file").get<std::string>();
            track.type = t.value("type", 1);
            list.tracks.push_back(track);
        }
    } catch (const nlohmann::json::exception& ex) {
        error = ex.what();
        return std::nullopt;
    }
    if (list.tracks.empty()) {
        error = "a playlist has at least one track";
        return std::nullopt;
    }
    return list;
}

// ---------------------------------------------------------------------------
// Names and music

std::string stemOf(const std::string& name) {
    const std::size_t dot = name.rfind('.');
    return dot == std::string::npos ? name : name.substr(0, dot);
}

std::string extensionOf(const std::string& name) {
    const std::size_t dot = name.rfind('.');
    return dot == std::string::npos ? std::string() : name.substr(dot + 1);
}

std::string musicFormat(const Bytes& f) {
    if (f.size() >= 4 && f[0] == 'O' && f[1] == 'g' && f[2] == 'g' && f[3] == 'S') {
        return "ogg";
    }
    if (f.size() >= 3 && f[0] == 'I' && f[1] == 'D' && f[2] == '3') {
        return "mp3";
    }
    // An MPEG audio frame header, possibly after zero padding (D-64).
    std::size_t i = 0;
    while (i < f.size() && f[i] == 0) {
        ++i;
    }
    if (i + 1 < f.size() && f[i] == 0xFF && (f[i + 1] & 0xE0) == 0xE0) {
        return "mp3";
    }
    return "";
}

// ---------------------------------------------------------------------------
// Legacy tree

bool readLegacyTree(const std::string& dataDir, const AssetVisitor& visit, std::vector<std::string>& skipped,
                    std::string& error) {
    const core::Vfs files(dataDir);
    const core::assets::LegacyDataSource legacy(files);
    std::vector<std::string> names;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(dataDir, ec)) {
        if (entry.is_regular_file()) {
            names.push_back(entry.path().filename().string());
        }
    }
    if (ec) {
        error = "cannot list " + dataDir;
        return false;
    }
    std::sort(names.begin(), names.end());
    for (const std::string& file : names) {
        const std::string name = core::assets::assetName(file);
        const std::string ext = extensionOf(name);
        bool ok = true;
        if (ext == "gfx") {
            auto a = legacy.spriteBank(file, error);
            ok = a.has_value() && (visit.spriteBank(name, *a), true);
        } else if (ext == "lft") {
            auto a = legacy.font(file, error);
            ok = a.has_value() && (visit.font(name, *a), true);
        } else if (ext == "sfx") {
            auto a = legacy.soundBank(file, error);
            ok = a.has_value() && (visit.soundBank(name, *a), true);
        } else if (ext == "zik") {
            auto a = legacy.music(file, error);
            ok = a.has_value() && (visit.music(name, *a), true);
        } else if (ext == "mbk") {
            auto a = legacy.musicList(file, error);
            ok = a.has_value() && (visit.musicList(name, *a), true);
        } else if (ext == "lvl") {
            auto a = legacy.level(file, error);
            ok = a.has_value() && (visit.level(name, *a), true);
        } else if (ext == "dat") {
            auto a = legacy.strings(file, error);
            ok = a.has_value() && (visit.strings(name, *a), true);
        } else if (ext == "lst") {
            auto a = legacy.campaign(file, error);
            ok = a.has_value() && (visit.campaign(name, *a), true);
        } else if (ext == "cin" || ext == "rpg") {
            auto a = legacy.script(file, error);
            ok = a.has_value() && (visit.script(name, *a), true);
        } else {
            skipped.push_back(file);
        }
        if (!ok) {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Source tree: reading

namespace {

std::vector<fs::path> sortedEntries(const fs::path& dir) {
    std::vector<fs::path> out;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        out.push_back(entry.path());
    }
    std::sort(out.begin(), out.end());
    return out;
}

std::optional<SpriteBank> readSprites(const fs::path& dir, std::string& error) {
    const auto j = readJson(dir / "sprites.json", error);
    if (!j || !checkHeader(*j, "blipblop-sprites", error)) {
        return std::nullopt;
    }
    SpriteBank bank;
    try {
        for (const Json& item : j->at("images")) {
            PixelFormat16 layout{};
            if (!layoutFromName(item.at("layout").get<std::string>(), layout)) {
                error = "unknown layout " + item.at("layout").dump();
                return std::nullopt;
            }
            auto image = readPng((dir / item.at("file").get<std::string>()).string(), layout, error);
            if (!image) {
                return std::nullopt;
            }
            SpriteImage sprite;
            sprite.hotspotX = item.at("hotspot").at(0).get<std::int32_t>();
            sprite.hotspotY = item.at("hotspot").at(1).get<std::int32_t>();
            sprite.image = std::move(*image);
            bank.images.push_back(std::move(sprite));
        }
    } catch (const nlohmann::json::exception& ex) {
        error = (dir / "sprites.json").string() + ": " + ex.what();
        return std::nullopt;
    }
    return bank;
}

std::optional<Font> readFont(const fs::path& dir, std::string& error) {
    const auto j = readJson(dir / "font.json", error);
    if (!j || !checkHeader(*j, "blipblop-font", error)) {
        return std::nullopt;
    }
    Font font;
    try {
        font.lineHeight = j->at("lineHeight").get<std::int32_t>();
        font.spaceWidth = j->at("spaceWidth").get<std::int32_t>();
        for (const Json& item : j->at("glyphs")) {
            const std::string ch = item.at("char").get<std::string>();
            std::size_t pos = 0;
            const std::uint32_t cp = core::assets::nextCodePoint(ch, pos);
            if (ch.empty() || pos != ch.size() || !core::assets::isValidUtf8(ch)) {
                error = "a glyph's \"char\" is one character, not " + item.at("char").dump();
                return std::nullopt;
            }
            PixelFormat16 layout{};
            if (!layoutFromName(item.at("layout").get<std::string>(), layout)) {
                error = "unknown layout " + item.at("layout").dump();
                return std::nullopt;
            }
            auto image = readPng((dir / item.at("file").get<std::string>()).string(), layout, error);
            if (!image) {
                return std::nullopt;
            }
            font.glyphs[cp] = std::move(*image);
        }
    } catch (const nlohmann::json::exception& ex) {
        error = (dir / "font.json").string() + ": " + ex.what();
        return std::nullopt;
    }
    return font;
}

std::optional<SoundBank> readSounds(const fs::path& dir, std::string& error) {
    const auto j = readJson(dir / "sounds.json", error);
    if (!j || !checkHeader(*j, "blipblop-sounds", error)) {
        return std::nullopt;
    }
    SoundBank bank;
    try {
        for (const Json& item : j->at("sounds")) {
            auto wav = readBytes(dir / item.at("file").get<std::string>(), error);
            if (!wav) {
                return std::nullopt;
            }
            bank.sounds.push_back(SoundSample{item.value("voices", 1), std::move(*wav)});
        }
    } catch (const nlohmann::json::exception& ex) {
        error = (dir / "sounds.json").string() + ": " + ex.what();
        return std::nullopt;
    }
    return bank;
}

}  // namespace

std::optional<SpriteBank> readSpriteBank(const std::string& dir, std::string& error) {
    return readSprites(fs::u8path(dir), error);
}

bool readSourceTree(const std::string& sourceDir, const AssetVisitor& visit, std::string& error) {
    const fs::path root(sourceDir);
    if (!fs::is_directory(root)) {
        error = sourceDir + " is not a directory";
        return false;
    }
    const auto fail = [&](const fs::path& where) {
        error = where.string() + ": " + error;
        return false;
    };
    for (const fs::path& dir : sortedEntries(root / "sprites")) {
        auto bank = readSprites(dir, error);
        if (!bank) {
            return fail(dir);
        }
        visit.spriteBank(dir.filename().string() + ".gfx", *bank);
    }
    for (const fs::path& dir : sortedEntries(root / "fonts")) {
        auto font = readFont(dir, error);
        if (!font) {
            return fail(dir);
        }
        visit.font(dir.filename().string() + ".lft", *font);
    }
    for (const fs::path& dir : sortedEntries(root / "sounds")) {
        auto bank = readSounds(dir, error);
        if (!bank) {
            return fail(dir);
        }
        visit.soundBank(dir.filename().string() + ".sfx", *bank);
    }
    for (const fs::path& file : sortedEntries(root / "music")) {
        const std::string name = file.filename().string();
        const std::string ext = file.extension().string();
        if (ext == ".ogg" || ext == ".mp3") {
            auto bytes = readBytes(file, error);
            if (!bytes) {
                return false;
            }
            visit.music(file.stem().string() + ".zik", *bytes);
        } else if (name.size() > 14 && name.compare(name.size() - 14, 14, ".playlist.json") == 0) {
            const auto j = readJson(file, error);
            auto list = j ? playlistFromJson(*j, error) : std::nullopt;
            if (!list) {
                return fail(file);
            }
            visit.musicList(name.substr(0, name.size() - 14) + ".mbk", *list);
        }
    }
    for (const fs::path& file : sortedEntries(root / "levels")) {
        const auto j = readJson(file, error);
        auto level = j ? levelFromJson(*j, error) : std::nullopt;
        if (!level) {
            return fail(file);
        }
        visit.level(file.stem().string() + ".lvl", *level);
    }
    for (const fs::path& file : sortedEntries(root / "strings")) {
        const auto j = readJson(file, error);
        auto table = j ? stringsFromJson(*j, error) : std::nullopt;
        if (!table) {
            return fail(file);
        }
        visit.strings(file.stem().string() + ".dat", *table);
    }
    for (const fs::path& file : sortedEntries(root / "campaign")) {
        const auto j = readJson(file, error);
        auto campaign = j ? campaignFromJson(*j, error) : std::nullopt;
        if (!campaign) {
            return fail(file);
        }
        visit.campaign(file.stem().string() + ".lst", *campaign);
    }
    for (const fs::path& file : sortedEntries(root / "scripts")) {
        auto bytes = readBytes(file, error);
        if (!bytes) {
            return false;
        }
        const std::string text(bytes->begin(), bytes->end());
        if (!core::assets::isValidUtf8(text)) {
            error = file.string() + ": not UTF-8";
            return false;
        }
        visit.script(file.filename().string(), text);
    }
    return true;
}

// ---------------------------------------------------------------------------
// Source tree: writing

bool SourceWriter::writeBytes(const std::string& path, const Bytes& bytes, std::string& error) {
    std::error_code ec;
    fs::create_directories(fs::path(path).parent_path(), ec);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!out) {
        error = "cannot write " + path;
        return false;
    }
    ++files_;
    return true;
}

bool SourceWriter::writeText(const std::string& path, const std::string& text, std::string& error) {
    return writeBytes(path, Bytes(text.begin(), text.end()), error);
}

bool SourceWriter::spriteBank(const std::string& name, const SpriteBank& bank, std::string& error) {
    const fs::path dir = fs::path(root_) / "sprites" / stemOf(name);
    std::error_code ec;
    // Symbolic names are added by hand (and by the migration): extracting
    // again over a tree keeps those of a bank with the same image count.
    std::vector<std::string> names(bank.images.size());
    if (fs::exists(dir / "sprites.json")) {
        std::string ignored;
        const auto old = readJson(dir / "sprites.json", ignored);
        if (old && old->contains("images") && (*old)["images"].size() == bank.images.size()) {
            for (std::size_t i = 0; i < names.size(); ++i) {
                names[i] = (*old)["images"][i].value("name", "");
            }
        }
    }
    fs::create_directories(dir, ec);
    Json j = header("blipblop-sprites");
    Json images = Json::array();
    const int digits = bank.images.size() > 1000 ? 4 : 3;
    for (std::size_t i = 0; i < bank.images.size(); ++i) {
        char file[32];  // room for any size_t (GCC -Wformat-truncation)
        std::snprintf(file, sizeof file, "%0*zu.png", digits, i);
        const SpriteImage& sprite = bank.images[i];
        if (!writePng((dir / file).string(), sprite.image, error)) {
            return false;
        }
        ++files_;
        Json item{{"file", file},
                  {"hotspot", Json::array({sprite.hotspotX, sprite.hotspotY})},
                  {"layout", layoutName(sprite.image.format)}};
        if (!names[i].empty()) {
            item["name"] = names[i];
        }
        images.push_back(item);
    }
    j["images"] = images;
    return writeText((dir / "sprites.json").string(), toJsonText(j), error);
}

bool SourceWriter::font(const std::string& name, const Font& font, std::string& error) {
    const fs::path dir = fs::path(root_) / "fonts" / stemOf(name);
    std::error_code ec;
    fs::create_directories(dir, ec);
    Json j = header("blipblop-font");
    j["lineHeight"] = font.lineHeight;
    j["spaceWidth"] = font.spaceWidth;
    Json glyphs = Json::array();
    for (const auto& [cp, image] : font.glyphs) {
        char file[16];
        std::snprintf(file, sizeof file, "%04X.png", static_cast<unsigned>(cp));
        if (!writePng((dir / file).string(), image, error)) {
            return false;
        }
        ++files_;
        std::string ch;
        core::assets::appendUtf8(ch, cp);
        glyphs.push_back(Json{{"char", ch}, {"file", file}, {"layout", layoutName(image.format)}});
    }
    j["glyphs"] = glyphs;
    return writeText((dir / "font.json").string(), toJsonText(j), error);
}

bool SourceWriter::soundBank(const std::string& name, const SoundBank& bank, std::string& error) {
    const fs::path dir = fs::path(root_) / "sounds" / stemOf(name);
    // Names are kept as for sprite banks (same sound count).
    std::vector<std::string> names(bank.sounds.size());
    if (fs::exists(dir / "sounds.json")) {
        std::string ignored;
        const auto old = readJson(dir / "sounds.json", ignored);
        if (old && old->contains("sounds") && (*old)["sounds"].size() == bank.sounds.size()) {
            for (std::size_t i = 0; i < names.size(); ++i) {
                names[i] = (*old)["sounds"][i].value("name", "");
            }
        }
    }
    Json j = header("blipblop-sounds");
    Json sounds = Json::array();
    for (std::size_t i = 0; i < bank.sounds.size(); ++i) {
        char file[32];  // room for any size_t (GCC -Wformat-truncation)
        std::snprintf(file, sizeof file, "%02zu.wav", i);
        if (!writeBytes((dir / file).string(), bank.sounds[i].wav, error)) {
            return false;
        }
        Json item{{"file", file}, {"voices", bank.sounds[i].voices}};
        if (!names[i].empty()) {
            item["name"] = names[i];
        }
        sounds.push_back(item);
    }
    j["sounds"] = sounds;
    return writeText((dir / "sounds.json").string(), toJsonText(j), error);
}

bool SourceWriter::music(const std::string& name, const Bytes& file, std::string& error) {
    const std::string format = musicFormat(file);
    if (format.empty()) {
        error = name + ": neither Ogg Vorbis nor MP3";
        return false;
    }
    return writeBytes((fs::path(root_) / "music" / (stemOf(name) + "." + format)).string(), file, error);
}

bool SourceWriter::musicList(const std::string& name, const MusicList& list, std::string& error) {
    const fs::path path = fs::path(root_) / "music" / (stemOf(name) + ".playlist.json");
    Json j = playlistToJson(list);
    // The track names are added by hand: extracting again keeps those of a
    // playlist with the same track count (as for the sprites).
    if (fs::exists(path)) {
        std::string ignored;
        const auto old = readJson(path, ignored);
        if (old && old->contains("tracks") && (*old)["tracks"].size() == list.tracks.size()) {
            for (std::size_t i = 0; i < list.tracks.size(); ++i) {
                const std::string trackName = (*old)["tracks"][i].value("name", "");
                if (!trackName.empty()) {
                    j["tracks"][i]["name"] = trackName;
                }
            }
        }
    }
    return writeText(path.string(), toJsonText(j), error);
}

bool SourceWriter::level(const std::string& name, const LevelData& level, std::string& error) {
    return writeText((fs::path(root_) / "levels" / (stemOf(name) + ".json")).string(),
                     toJsonText(levelToJson(level)), error);
}

bool SourceWriter::strings(const std::string& name, const StringTable& table, std::string& error) {
    return writeText((fs::path(root_) / "strings" / (stemOf(name) + ".json")).string(),
                     toJsonText(stringsToJson(table)), error);
}

bool SourceWriter::campaign(const std::string& name, const Campaign& campaign, std::string& error) {
    return writeText((fs::path(root_) / "campaign" / (stemOf(name) + ".json")).string(),
                     toJsonText(campaignToJson(campaign)), error);
}

bool SourceWriter::script(const std::string& name, const std::string& utf8, std::string& error) {
    return writeText((fs::path(root_) / "scripts" / name).string(), utf8, error);
}

}  // namespace bb::tools
