#pragma once

// In-memory form of every game asset, independent of the file format it was
// read from (the 2002 files in data/, or the runtime packs). The readers
// in legacy_formats.hpp and pack.hpp produce these structures; the game
// consumes them.

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "bb/core/assets/image16.hpp"

namespace bb::core::assets {

// ---------------------------------------------------------------------------
// Sprites

struct SpriteImage {
    std::int32_t hotspotX = 0;  // drawing origin relative to the top-left
    std::int32_t hotspotY = 0;
    Image16 image;

    bool operator==(const SpriteImage& o) const {
        return hotspotX == o.hotspotX && hotspotY == o.hotspotY && image == o.image;
    }
};

struct SpriteBank {
    std::vector<SpriteImage> images;  // addressed by index
};

// ---------------------------------------------------------------------------
// Fonts

struct Font {
    std::int32_t lineHeight = 0;  // advance between lines of wrapped text
    std::int32_t spaceWidth = 0;  // advance of ' ' (U+0020 never has a glyph)
    // Glyphs by Unicode code point; the advance of a glyph is its width
    // and its hotspot is (0, 0).
    std::map<std::uint32_t, Image16> glyphs;
};

// ---------------------------------------------------------------------------
// Sounds

struct SoundSample {
    // Simultaneous instances wanted by the 2002 banks (1 for every shipped
    // sound; the game ignores it).
    std::int32_t voices = 1;
    std::vector<std::uint8_t> wav;  // a complete RIFF/WAVE file
};

struct SoundBank {
    std::vector<SoundSample> sounds;
};

// Format of a RIFF/WAVE file.
struct WavInfo {
    std::uint16_t formatTag = 0;  // 1 = PCM
    std::uint16_t channels = 0;
    std::uint32_t sampleRate = 0;
    std::uint16_t bitsPerSample = 0;
    std::size_t dataOffset = 0;  // of the sample bytes in the file
    std::size_t dataSize = 0;
};

std::optional<WavInfo> parseWav(const std::vector<std::uint8_t>& wav, std::string& error);

// ---------------------------------------------------------------------------
// Music

struct MusicTrack {
    // 1 = streamed file (every shipped track); 0 = tracker module, which
    // no shipped bank uses and the game rejects.
    std::int32_t type = 1;
    std::string path;  // "data/xxx.zik", relative to the game directory
};

struct MusicList {
    std::vector<MusicTrack> tracks;
};

// ---------------------------------------------------------------------------
// Text

// Strings by numeric id, UTF-8.
struct StringTable {
    std::map<int, std::string> strings;

    const std::string* find(int id) const {
        const auto it = strings.find(id);
        return it == strings.end() ? nullptr : &it->second;
    }
};

// One entry of the campaign list (bb.lst).
struct CampaignEntry {
    enum class Kind : std::int32_t { Cutscene = 0, Level = 1, Briefing = 2 };
    Kind kind = Kind::Level;
    // For levels: 0 bonus, 1 first of a world, 2 intermediate, 3 last,
    // 4 complete, 5 end (docs/formats/lst.md).
    std::int32_t levelKind = 0;
    std::string path;  // "data/xxx", as written in the file
};

struct Campaign {
    std::vector<CampaignEntry> entries;
};

// ---------------------------------------------------------------------------
// Levels

enum class LevelEventType : std::int32_t {
    Enemy = 0,
    EnemyGenerator = 1,
    AnimatedProp = 2,
    MidgroundDecal = 3,
    Foreground = 4,
    ScrollLock = 5,
    HoldFire = 6,
    ForceScroll = 7,
    Dialogue = 8,
    SetFlag = 9,
    BannerText = 10,
    Sound = 11,
    Weather = 12,
    Overkill = 13,
    DynamicLoad = 14,
    Music = 15,
    PickupGenerator = 16,
    Vehicle = 17,
    Pickup = 18,
};

// One 60-byte FICEVENT record (docs/formats/lvl.md), field for field. The
// four union slots carry different values per event type; their names
// here are the union's first member.
struct LevelEventRecord {
    std::int32_t type = 0;  // LevelEventType
    std::int32_t activationX = 0;
    std::int32_t id = 0;
    std::int32_t x = 0;
    std::int32_t bank = 0;      // n_pbk / n_sbk / n_txt
    std::int32_t y = 0;         // y / cond / speed / intensite / play
    std::int32_t capacity = 0;  // capacite / flag / duree
    std::int32_t value = 0;     // val / sens / dir
    std::int32_t period = 0;
    std::uint8_t temporary = 0;  // a bool byte as stored: 0, 1, or 0xCD
    std::array<std::uint8_t, 3> padding{};
    std::array<std::uint8_t, 20> file{};

    bool operator==(const LevelEventRecord& o) const;
    bool operator!=(const LevelEventRecord& o) const { return !(*this == o); }
};

// The 2002 level editor was a debug build: fields it never set hold the
// MSVC heap fill 0xCDCDCDCD (bytes 0xCD).
constexpr std::int32_t kEditorFill = static_cast<std::int32_t>(0xCDCDCDCDu);
constexpr std::uint8_t kEditorFillByte = 0xCD;

using LevelMarkers = std::map<std::string, std::vector<std::int32_t>>;

struct LevelData {
    static constexpr int kAssetCount = 7;
    static constexpr int kPlatformLayers = 8;
    static constexpr int kWallRows = 60;  // 480 / 8
    static constexpr std::int32_t kNoPlatform = 0xFFFF;

    // decor bank, level (props) bank, enemy bank, sound bank, music bank,
    // dialogue script, dialogue portrait bank; "" = none.
    std::array<std::string, kAssetCount> assets;
    std::vector<std::int32_t> screens;  // decor image index per 640-px screen
    std::int32_t start1X = 0, start1Y = 0, start2X = 0, start2Y = 0;
    std::int32_t victoryX = 0;
    std::int32_t victoryFlag1 = 0, victoryValue1 = 0;
    std::int32_t victoryFlag2 = 0, victoryValue2 = 0;
    // Platform height per pixel column and layer (kNoPlatform = none).
    std::array<std::vector<std::int32_t>, kPlatformLayers> platforms;
    // kWallRows rows of width / 8 cells, row-major, 0 or 1 (as stored).
    std::vector<std::uint8_t> solidWalls;
    std::vector<std::uint8_t> bloodWalls;
    // In file order: descending activation X (docs/formats/lvl.md).
    std::vector<LevelEventRecord> events;
    // Positions and settings of this level that its content reads by name
    // (D-51): the bosses' arenas, where the stork drops its rider... The
    // 2002 .lvl format has no place for them; the 2002 data source adds the
    // 2002 values (legacyLevelMarkers()).
    LevelMarkers markers;

    int width() const noexcept { return static_cast<int>(screens.size()) * 640; }
    int wallColumns() const noexcept { return width() / 8; }
};

}  // namespace bb::core::assets
