#pragma once

// The source asset tree (docs/formats/source-assets.md) and its conversion
// from and to the 2002 files:
//
//   sprites/<stem>/sprites.json + NNN.png   <-> <stem>.gfx
//   fonts/<stem>/font.json + XXXX.png       <-> <stem>.lft
//   sounds/<stem>/sounds.json + NN.wav      <-> <stem>.sfx
//   music/<stem>.ogg | <stem>.mp3           <-> <stem>.zik
//   music/<stem>.playlist.json              <-> <stem>.mbk
//   levels/<stem>.json                      <-> <stem>.lvl
//   strings/<stem>.json                     <-> <stem>.dat
//   campaign/<stem>.json                    <-> <stem>.lst
//   scripts/<name>.cin, scripts/<name>.rpg  <-> same name (UTF-8 <-> Windows-1252)

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "bb/core/assets/asset_types.hpp"
#include "json_text.hpp"

namespace bb::tools {

using core::assets::Campaign;
using core::assets::Font;
using core::assets::LevelData;
using core::assets::MusicList;
using core::assets::SoundBank;
using core::assets::SpriteBank;
using core::assets::StringTable;
using Bytes = std::vector<std::uint8_t>;

// --- JSON form of each asset (pure functions, no files) ---------------------

Json levelToJson(const LevelData& level);
std::optional<LevelData> levelFromJson(const Json& json, std::string& error);

Json stringsToJson(const StringTable& table);
std::optional<StringTable> stringsFromJson(const Json& json, std::string& error);

Json campaignToJson(const Campaign& campaign);
std::optional<Campaign> campaignFromJson(const Json& json, std::string& error);

Json playlistToJson(const MusicList& list);
std::optional<MusicList> playlistFromJson(const Json& json, std::string& error);

// --- Whole trees -----------------------------------------------------------

// One asset of the game, by legacy file name ("snufenn.gfx"), in memory.
struct AssetVisitor {
    std::function<void(const std::string&, const SpriteBank&)> spriteBank;
    std::function<void(const std::string&, const Font&)> font;
    std::function<void(const std::string&, const SoundBank&)> soundBank;
    std::function<void(const std::string&, const Bytes&)> music;
    std::function<void(const std::string&, const MusicList&)> musicList;
    std::function<void(const std::string&, const LevelData&)> level;
    std::function<void(const std::string&, const StringTable&)> strings;
    std::function<void(const std::string&, const Campaign&)> campaign;
    std::function<void(const std::string&, const std::string&)> script;  // UTF-8
};

// Reads every asset of a 2002 data directory. False (and error) at the
// first file that cannot be read; unknown files are listed in skipped.
bool readLegacyTree(const std::string& dataDir, const AssetVisitor& visit, std::vector<std::string>& skipped,
                    std::string& error);

// Reads every asset of a source tree.
bool readSourceTree(const std::string& sourceDir, const AssetVisitor& visit, std::string& error);

// Writes the source form of one asset under sourceDir; false and error on
// failure. Returns the files written through written.
class SourceWriter {
public:
    explicit SourceWriter(std::string sourceDir) : root_(std::move(sourceDir)) {}

    bool spriteBank(const std::string& name, const SpriteBank& bank, std::string& error);
    bool font(const std::string& name, const Font& font, std::string& error);
    bool soundBank(const std::string& name, const SoundBank& bank, std::string& error);
    bool music(const std::string& name, const Bytes& file, std::string& error);
    bool musicList(const std::string& name, const MusicList& list, std::string& error);
    bool level(const std::string& name, const LevelData& level, std::string& error);
    bool strings(const std::string& name, const StringTable& table, std::string& error);
    bool campaign(const std::string& name, const Campaign& campaign, std::string& error);
    bool script(const std::string& name, const std::string& utf8, std::string& error);

    std::size_t filesWritten() const noexcept { return files_; }

private:
    bool writeText(const std::string& path, const std::string& text, std::string& error);
    bool writeBytes(const std::string& path, const Bytes& bytes, std::string& error);

    std::string root_;
    std::size_t files_ = 0;
};

// The stem and the extension of a legacy name ("snufenn.gfx" -> "snufenn", "gfx").
std::string stemOf(const std::string& name);
std::string extensionOf(const std::string& name);

// Content type of a music file: "ogg", "mp3", or "" when unknown.
std::string musicFormat(const Bytes& file);

}  // namespace bb::tools
