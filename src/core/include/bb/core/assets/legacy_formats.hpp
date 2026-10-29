#pragma once

// Readers and writers for the 2002 data files (docs/formats/). Every
// reader checks bounds and returns nullopt with a message instead of
// reading past the end; every writer produces the bytes the 2002 tools
// wrote, so shipped files round-trip (the tests check all of them).

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "bb/core/assets/asset_types.hpp"

namespace bb::core::assets {

using Bytes = std::vector<std::uint8_t>;

// .gfx sprite bank: count, then per image hotspot X/Y, LGX size, LGX image.
std::optional<SpriteBank> readGfx(const Bytes& file, std::string& error);
Bytes writeGfx(const SpriteBank& bank);

// .lft font: line height, space width, then for bytes 1..255 a size and an
// LGX image when the size is not 0. Glyph bytes are Windows-1252 and become
// code points (writeLft fails for a glyph with no Windows-1252 byte).
std::optional<Font> readLft(const Bytes& file, std::string& error);
std::optional<Bytes> writeLft(const Font& font, std::string& error);

// .sfx sound bank: count, then per sound a voice count, a size and a WAV.
std::optional<SoundBank> readSfx(const Bytes& file, std::string& error);
Bytes writeSfx(const SoundBank& bank);

// .lvl level. Each of the seven asset names is stored in 20 bytes; the
// 2002 editor left stack garbage after the terminating NUL, which the game
// never reads: writeLvl writes zeros there, and this is the only
// difference with the shipped files.
std::optional<LevelData> readLvl(const Bytes& file, std::string& error);
// The markers of a 2002 level ("data/snuf1.lvl"; none for unknown names):
// the values the 2002 classes had in their code.
LevelMarkers legacyLevelMarkers(std::string_view levelPath);
std::optional<Bytes> writeLvl(const LevelData& level, std::string& error);

// .dat string table: lines "id^text" in Windows-1252, ids 0..9999 (the
// 2002 game read 0..999; the credits now use 1000 and up). The strings
// become UTF-8.
std::optional<StringTable> readDat(const Bytes& file, std::string& error);
std::optional<Bytes> writeDat(const StringTable& table, std::string& error);

// bb.lst campaign: lines "type [levelKind] *path". writeLst writes one
// line per entry, each ending with a newline, as the shipped file.
std::optional<Campaign> readLst(const Bytes& file, std::string& error);
Bytes writeLst(const Campaign& campaign);

// .mbk music bank: a count, then "type path" pairs (whitespace separated).
// writeMbk writes the count and one pair per line, without a final newline
// (most shipped banks have none).
std::optional<MusicList> readMbk(const Bytes& file, std::string& error);
Bytes writeMbk(const MusicList& list);

// .cin cutscene script, as the 2002 player reads it: blank lines and lines
// starting with ';' are skipped; spaces and '"' are removed, the rest is
// lower-cased and cut at ';'; a command is "name(arg,arg,...)".
struct CutsceneCommand {
    int line = 0;  // 1-based line in the file
    std::string name;
    std::vector<std::string> args;
};
std::optional<std::vector<CutsceneCommand>> parseCutscene(std::string_view text, std::string& error);

// .rpg dialogue script: scenes from a line "rpg=N" to a line "stop", made
// of statements ("focus=haut", "txt=207", "bbswap", ...); lines starting
// with ';' are comments. Returns the statements of scene n, nullopt when
// the scene does not exist.
std::optional<std::vector<std::string>> dialogueScene(std::string_view text, int n);
// Scene numbers, in file order.
std::vector<int> dialogueScenes(std::string_view text);

// bb.scr hi-scores: 10 x (int32 score, char name[20]) XOR-obfuscated with
// 0x35674A1F << rank, then the int32 sum of the plain scores and of the name
// bytes as signed chars (the 2002 x86 compilers' char).
struct HiScoreEntry {
    std::int32_t score = 0;
    std::array<char, 20> name{};
};
struct HiScoreTable {
    std::array<HiScoreEntry, 10> entries{};
};
// nullopt when the size or the checksum is wrong.
std::optional<HiScoreTable> readScr(const Bytes& file, std::string& error);
Bytes writeScr(const HiScoreTable& table);

}  // namespace bb::core::assets
