#include "bb/core/assets/legacy_formats.hpp"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <utility>

#include "bb/core/assets/byte_io.hpp"
#include "bb/core/assets/text_encoding.hpp"

namespace bb::core::assets {

namespace {

constexpr std::size_t kNameSize = 20;
constexpr std::size_t kEventRecordSize = 60;
constexpr int kMaxStringId = 10000;          // the 2002 table had 1000 entries
constexpr std::size_t kMaxStringLine = 399;  // getline(buffer, 400)

std::string at(const char* what, std::size_t index) {
    return std::string(what) + " " + std::to_string(index);
}

// Reads a sized LGX blob and decodes it.
bool readLgxBlob(ByteReader& in, Image16& image, std::string& error) {
    const std::int32_t size = in.i32();
    if (!in.ok() || size < 0 || !in.canRead(static_cast<std::size_t>(size))) {
        error = "truncated image";
        return false;
    }
    auto decoded = decodeLgx(in.current(), static_cast<std::size_t>(size), error);
    if (!decoded) {
        return false;
    }
    in.skip(static_cast<std::size_t>(size));
    image = std::move(*decoded);
    return true;
}

void writeLgxBlob(ByteWriter& out, const Image16& image) {
    const Bytes lgx = encodeLgx(image);
    out.i32(static_cast<std::int32_t>(lgx.size()));
    out.bytes(lgx);
}

std::vector<std::string> splitLines(std::string_view text) {
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t end = text.find('\n', start);
        if (end == std::string_view::npos) {
            if (start < text.size()) {
                lines.emplace_back(text.substr(start));
            }
            break;
        }
        lines.emplace_back(text.substr(start, end - start));
        start = end + 1;
    }
    for (auto& line : lines) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
    }
    return lines;
}

std::string toText(const Bytes& file) {
    return std::string(file.begin(), file.end());
}

bool parseInt(std::string_view s, std::int32_t& value) {
    if (s.empty()) {
        return false;
    }
    std::size_t i = 0;
    bool negative = false;
    if (s[0] == '-' || s[0] == '+') {
        negative = s[0] == '-';
        i = 1;
    }
    if (i == s.size()) {
        return false;
    }
    std::int64_t v = 0;
    for (; i < s.size(); ++i) {
        if (s[i] < '0' || s[i] > '9') {
            return false;
        }
        v = v * 10 + (s[i] - '0');
        if (v > 0x7FFFFFFF) {
            return false;
        }
    }
    value = static_cast<std::int32_t>(negative ? -v : v);
    return true;
}

}  // namespace

bool LevelEventRecord::operator==(const LevelEventRecord& o) const {
    return type == o.type && activationX == o.activationX && id == o.id && x == o.x && bank == o.bank &&
           y == o.y && capacity == o.capacity && value == o.value && period == o.period &&
           temporary == o.temporary && padding == o.padding && file == o.file;
}

// ---------------------------------------------------------------------------
// .gfx

std::optional<SpriteBank> readGfx(const Bytes& file, std::string& error) {
    ByteReader in(file);
    const std::int32_t count = in.i32();
    // Each image takes at least 8 bytes (its hotspot): a larger count is
    // damage, refused before anything is allocated for it.
    if (!in.ok() || count < 0 || static_cast<std::size_t>(count) > in.remaining() / 8) {
        error = "bad image count";
        return std::nullopt;
    }
    SpriteBank bank;
    bank.images.resize(static_cast<std::size_t>(count));
    for (std::size_t i = 0; i < bank.images.size(); ++i) {
        SpriteImage& sprite = bank.images[i];
        sprite.hotspotX = in.i32();
        sprite.hotspotY = in.i32();
        if (!readLgxBlob(in, sprite.image, error)) {
            error = at("image", i) + ": " + error;
            return std::nullopt;
        }
    }
    if (in.remaining() != 0) {
        error = std::to_string(in.remaining()) + " bytes after the last image";
        return std::nullopt;
    }
    return bank;
}

Bytes writeGfx(const SpriteBank& bank) {
    ByteWriter out;
    out.i32(static_cast<std::int32_t>(bank.images.size()));
    for (const SpriteImage& sprite : bank.images) {
        out.i32(sprite.hotspotX);
        out.i32(sprite.hotspotY);
        writeLgxBlob(out, sprite.image);
    }
    return out.take();
}

// ---------------------------------------------------------------------------
// .lft

std::optional<Font> readLft(const Bytes& file, std::string& error) {
    ByteReader in(file);
    Font font;
    font.lineHeight = in.i32();
    font.spaceWidth = in.i32();
    for (int byte = 1; byte < 256; ++byte) {
        const std::int32_t size = in.i32();
        if (!in.ok() || size < 0) {
            error = at("truncated font at glyph", static_cast<std::size_t>(byte));
            return std::nullopt;
        }
        if (size == 0) {
            continue;
        }
        if (!in.canRead(static_cast<std::size_t>(size))) {
            error = at("truncated glyph", static_cast<std::size_t>(byte));
            return std::nullopt;
        }
        auto glyph = decodeLgx(in.current(), static_cast<std::size_t>(size), error);
        if (!glyph) {
            error = at("glyph", static_cast<std::size_t>(byte)) + ": " + error;
            return std::nullopt;
        }
        in.skip(static_cast<std::size_t>(size));
        font.glyphs[windows1252ToCodePoint(static_cast<std::uint8_t>(byte))] = std::move(*glyph);
    }
    if (in.remaining() != 0) {
        error = std::to_string(in.remaining()) + " bytes after the last glyph";
        return std::nullopt;
    }
    return font;
}

std::optional<Bytes> writeLft(const Font& font, std::string& error) {
    std::array<const Image16*, 256> byByte{};
    for (const auto& [codePoint, image] : font.glyphs) {
        const auto byte = codePointToWindows1252(codePoint);
        if (!byte || *byte == 0 || *byte == ' ') {
            error = "glyph U+" + std::to_string(codePoint) + " has no slot in a .lft font";
            return std::nullopt;
        }
        byByte[*byte] = &image;
    }
    ByteWriter out;
    out.i32(font.lineHeight);
    out.i32(font.spaceWidth);
    for (std::size_t byte = 1; byte < 256; ++byte) {
        if (byByte[byte] == nullptr) {
            out.i32(0);
        } else {
            writeLgxBlob(out, *byByte[byte]);
        }
    }
    return out.take();
}

// ---------------------------------------------------------------------------
// .sfx

std::optional<SoundBank> readSfx(const Bytes& file, std::string& error) {
    ByteReader in(file);
    const std::int32_t count = in.i32();
    // SoundBank::loadSFX rejects empty banks; each sound takes at least 8
    // bytes (its voices and size).
    if (!in.ok() || count < 1 || static_cast<std::size_t>(count) > in.remaining() / 8) {
        error = "bad sound count";
        return std::nullopt;
    }
    SoundBank bank;
    bank.sounds.resize(static_cast<std::size_t>(count));
    for (std::size_t i = 0; i < bank.sounds.size(); ++i) {
        SoundSample& sound = bank.sounds[i];
        sound.voices = in.i32();
        const std::int32_t size = in.i32();
        if (!in.ok() || size < 0 || !in.canRead(static_cast<std::size_t>(size))) {
            error = at("truncated sound", i);
            return std::nullopt;
        }
        sound.wav = in.bytes(static_cast<std::size_t>(size));
    }
    if (in.remaining() != 0) {
        error = std::to_string(in.remaining()) + " bytes after the last sound";
        return std::nullopt;
    }
    return bank;
}

Bytes writeSfx(const SoundBank& bank) {
    ByteWriter out;
    out.i32(static_cast<std::int32_t>(bank.sounds.size()));
    for (const SoundSample& sound : bank.sounds) {
        out.i32(sound.voices);
        out.i32(static_cast<std::int32_t>(sound.wav.size()));
        out.bytes(sound.wav);
    }
    return out.take();
}

std::optional<WavInfo> parseWav(const std::vector<std::uint8_t>& wav, std::string& error) {
    ByteReader in(wav);
    char riff[4];
    char wave[4];
    in.bytes(riff, 4);
    in.u32();
    in.bytes(wave, 4);
    if (!in.ok() || std::memcmp(riff, "RIFF", 4) != 0 || std::memcmp(wave, "WAVE", 4) != 0) {
        error = "not a RIFF/WAVE file";
        return std::nullopt;
    }
    WavInfo info;
    bool haveFormat = false;
    bool haveData = false;
    while (in.remaining() >= 8 && !haveData) {
        char id[4];
        in.bytes(id, 4);
        const std::uint32_t size = in.u32();
        if (!in.canRead(size)) {
            error = "truncated chunk";
            return std::nullopt;
        }
        if (std::memcmp(id, "fmt ", 4) == 0 && size >= 16) {
            ByteReader fmt(in.current(), size);
            info.formatTag = fmt.u16();
            info.channels = fmt.u16();
            info.sampleRate = fmt.u32();
            fmt.u32();  // byte rate
            fmt.u16();  // block align
            info.bitsPerSample = fmt.u16();
            haveFormat = true;
        } else if (std::memcmp(id, "data", 4) == 0) {
            info.dataOffset = in.position();
            info.dataSize = size;
            haveData = true;
        }
        in.skip(size + (size & 1u));
    }
    if (!haveFormat || !haveData) {
        error = "missing fmt or data chunk";
        return std::nullopt;
    }
    return info;
}

// ---------------------------------------------------------------------------
// .lvl

std::optional<LevelData> readLvl(const Bytes& file, std::string& error) {
    ByteReader in(file);
    LevelData level;
    for (std::string& name : level.assets) {
        char raw[kNameSize];
        in.bytes(raw, kNameSize);
        const void* nul = std::memchr(raw, 0, kNameSize);
        if (nul == nullptr) {
            error = "asset name without terminator";
            return std::nullopt;
        }
        name.assign(raw, static_cast<std::size_t>(static_cast<const char*>(nul) - raw));
    }
    const std::int32_t screens = in.i32();
    if (!in.ok() || screens <= 0 || screens > 1000) {
        error = "bad screen count";
        return std::nullopt;
    }
    level.screens.resize(static_cast<std::size_t>(screens));
    for (std::int32_t& screen : level.screens) {
        screen = in.i32();
    }
    level.start1X = in.i32();
    level.start1Y = in.i32();
    level.start2X = in.i32();
    level.start2Y = in.i32();
    level.victoryX = in.i32();
    level.victoryFlag1 = in.i32();
    level.victoryValue1 = in.i32();
    level.victoryFlag2 = in.i32();
    level.victoryValue2 = in.i32();

    const std::size_t width = static_cast<std::size_t>(level.width());
    for (auto& layer : level.platforms) {
        if (!in.canRead(width * 4)) {
            error = "truncated platforms";
            return std::nullopt;
        }
        layer.resize(width);
        for (std::int32_t& y : layer) {
            y = in.i32();
        }
    }
    const std::size_t cells = static_cast<std::size_t>(LevelData::kWallRows) * (width / 8);
    if (!in.canRead(2 * cells)) {
        error = "truncated walls";
        return std::nullopt;
    }
    level.solidWalls = in.bytes(cells);
    level.bloodWalls = in.bytes(cells);

    const std::int32_t events = in.i32();
    if (!in.ok() || events < 0 || !in.canRead(static_cast<std::size_t>(events) * kEventRecordSize)) {
        error = "bad event count";
        return std::nullopt;
    }
    level.events.resize(static_cast<std::size_t>(events));
    for (LevelEventRecord& e : level.events) {
        e.type = in.i32();
        e.activationX = in.i32();
        e.id = in.i32();
        e.x = in.i32();
        e.bank = in.i32();
        e.y = in.i32();
        e.capacity = in.i32();
        e.value = in.i32();
        e.period = in.i32();
        e.temporary = in.u8();
        in.bytes(e.padding.data(), e.padding.size());
        in.bytes(e.file.data(), e.file.size());
    }
    if (in.remaining() != 0) {
        error = std::to_string(in.remaining()) + " bytes after the last event";
        return std::nullopt;
    }
    return level;
}

std::optional<Bytes> writeLvl(const LevelData& level, std::string& error) {
    const std::size_t width = static_cast<std::size_t>(level.width());
    const std::size_t cells = static_cast<std::size_t>(LevelData::kWallRows) * (width / 8);
    for (const auto& layer : level.platforms) {
        if (layer.size() != width) {
            error = "platform layer size does not match the screen count";
            return std::nullopt;
        }
    }
    if (level.solidWalls.size() != cells || level.bloodWalls.size() != cells) {
        error = "wall grid size does not match the screen count";
        return std::nullopt;
    }
    ByteWriter out;
    for (const std::string& name : level.assets) {
        if (name.size() >= kNameSize) {
            error = "asset name longer than 19 bytes: " + name;
            return std::nullopt;
        }
        out.bytes(name.data(), name.size());
        out.fill(0, kNameSize - name.size());
    }
    out.i32(static_cast<std::int32_t>(level.screens.size()));
    for (const std::int32_t screen : level.screens) {
        out.i32(screen);
    }
    for (const std::int32_t v :
         {level.start1X, level.start1Y, level.start2X, level.start2Y, level.victoryX, level.victoryFlag1,
          level.victoryValue1, level.victoryFlag2, level.victoryValue2}) {
        out.i32(v);
    }
    for (const auto& layer : level.platforms) {
        for (const std::int32_t y : layer) {
            out.i32(y);
        }
    }
    out.bytes(level.solidWalls);
    out.bytes(level.bloodWalls);
    out.i32(static_cast<std::int32_t>(level.events.size()));
    for (const LevelEventRecord& e : level.events) {
        for (const std::int32_t v :
             {e.type, e.activationX, e.id, e.x, e.bank, e.y, e.capacity, e.value, e.period}) {
            out.i32(v);
        }
        out.u8(e.temporary);
        out.bytes(e.padding.data(), e.padding.size());
        out.bytes(e.file.data(), e.file.size());
    }
    return out.take();
}

// ---------------------------------------------------------------------------
// .dat

std::optional<StringTable> readDat(const Bytes& file, std::string& error) {
    StringTable table;
    const std::string text = toText(file);
    const auto lines = splitLines(text);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const std::string& line = lines[i];
        if (line.empty()) {
            continue;
        }
        const std::size_t caret = line.find('^');
        std::int32_t id = 0;
        if (caret == std::string::npos || !parseInt(line.substr(0, caret), id) || id < 0 ||
            id >= kMaxStringId) {
            error = at("bad string id on line", i + 1);
            return std::nullopt;
        }
        if (caret > kMaxStringLine || line.size() - caret - 1 > kMaxStringLine) {
            error = at("line too long for the 2002 reader:", i + 1);
            return std::nullopt;
        }
        table.strings[id] = windows1252ToUtf8(std::string_view(line).substr(caret + 1));
    }
    return table;
}

std::optional<Bytes> writeDat(const StringTable& table, std::string& error) {
    std::string out;
    bool first = true;
    for (const auto& [id, text] : table.strings) {
        const auto encoded = utf8ToWindows1252(text);
        if (!encoded) {
            error = "string " + std::to_string(id) + " cannot be written in Windows-1252";
            return std::nullopt;
        }
        if (!first) {
            out += '\n';
        }
        first = false;
        out += std::to_string(id);
        out += '^';
        out += *encoded;
    }
    return Bytes(out.begin(), out.end());
}

// ---------------------------------------------------------------------------
// bb.lst

std::optional<Campaign> readLst(const Bytes& file, std::string& error) {
    Campaign campaign;
    const auto lines = splitLines(toText(file));
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const std::string& line = lines[i];
        if (line.find_first_not_of(" \t") == std::string::npos) {
            continue;
        }
        const std::size_t star = line.find('*');
        if (star == std::string::npos) {
            error = at("missing '*' on line", i + 1);
            return std::nullopt;
        }
        std::istringstream head(line.substr(0, star));
        std::int32_t kind = -1;
        CampaignEntry entry;
        head >> kind;
        if (head.fail() || kind < 0 || kind > 2) {
            error = at("bad entry type on line", i + 1);
            return std::nullopt;
        }
        entry.kind = static_cast<CampaignEntry::Kind>(kind);
        if (entry.kind == CampaignEntry::Kind::Level) {
            head >> entry.levelKind;
            if (head.fail()) {
                error = at("bad level kind on line", i + 1);
                return std::nullopt;
            }
        }
        entry.path = line.substr(star + 1);
        campaign.entries.push_back(std::move(entry));
    }
    return campaign;
}

Bytes writeLst(const Campaign& campaign) {
    std::string out;
    for (const CampaignEntry& entry : campaign.entries) {
        out += std::to_string(static_cast<int>(entry.kind));
        if (entry.kind == CampaignEntry::Kind::Level) {
            out += ' ' + std::to_string(entry.levelKind);
        }
        out += " *" + entry.path + '\n';
    }
    return Bytes(out.begin(), out.end());
}

// ---------------------------------------------------------------------------
// .mbk

std::optional<MusicList> readMbk(const Bytes& file, std::string& error) {
    std::istringstream in(toText(file));
    int count = 0;
    in >> count;
    if (in.fail() || count < 1) {
        error = "bad track count";
        return std::nullopt;
    }
    MusicList list;
    for (int i = 0; i < count; ++i) {
        MusicTrack track;
        in >> track.type >> track.path;
        if (in.fail()) {
            error = at("bad track", static_cast<std::size_t>(i));
            return std::nullopt;
        }
        list.tracks.push_back(std::move(track));
    }
    return list;
}

Bytes writeMbk(const MusicList& list) {
    std::string out = std::to_string(list.tracks.size());
    for (const MusicTrack& track : list.tracks) {
        out += '\n' + std::to_string(track.type) + ' ' + track.path;
    }
    return Bytes(out.begin(), out.end());
}

// ---------------------------------------------------------------------------
// .cin

std::optional<std::vector<CutsceneCommand>> parseCutscene(std::string_view text, std::string& error) {
    std::vector<CutsceneCommand> commands;
    const auto lines = splitLines(text);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const std::string& raw = lines[i];
        if (raw.empty() || raw[0] == ';') {
            continue;
        }
        std::string line;
        for (const char c : raw) {
            if (c == ';') {
                break;
            }
            if (c != ' ' && c != '"') {
                line.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            }
        }
        const std::size_t paren = line.find('(');
        if (paren == std::string::npos || paren == 0) {
            error = at("syntax error on line", i + 1);
            return std::nullopt;
        }
        CutsceneCommand command;
        command.line = static_cast<int>(i + 1);
        command.name = line.substr(0, paren);
        // strtok(",)\n"): empty arguments are skipped.
        std::string current;
        for (std::size_t k = paren + 1; k <= line.size(); ++k) {
            const char c = k < line.size() ? line[k] : ',';
            if (c == ',' || c == ')') {
                if (!current.empty()) {
                    command.args.push_back(current);
                }
                current.clear();
            } else {
                current.push_back(c);
            }
        }
        commands.push_back(std::move(command));
    }
    return commands;
}

// ---------------------------------------------------------------------------
// .rpg

std::optional<std::vector<std::string>> dialogueScene(std::string_view text, int n) {
    const auto lines = splitLines(text);
    const std::string header = "rpg=" + std::to_string(n);
    std::size_t i = 0;
    while (i < lines.size() && lines[i] != header) {
        ++i;
    }
    if (i == lines.size()) {
        return std::nullopt;
    }
    std::vector<std::string> statements;
    for (++i; i < lines.size() && lines[i] != "stop"; ++i) {
        if (!lines[i].empty() && lines[i][0] == ';') {
            continue;
        }
        statements.push_back(lines[i]);
    }
    return statements;
}

std::vector<int> dialogueScenes(std::string_view text) {
    std::vector<int> scenes;
    for (const std::string& line : splitLines(text)) {
        std::int32_t n = 0;
        if (line.rfind("rpg=", 0) == 0 && parseInt(std::string_view(line).substr(4), n)) {
            scenes.push_back(n);
        }
    }
    return scenes;
}

// ---------------------------------------------------------------------------
// bb.scr

namespace {

std::uint32_t scoreKey(std::size_t rank) {
    return 0x35674A1Fu << rank;
}

std::uint32_t scoreChecksum(const HiScoreTable& table) {
    std::uint32_t sum = 0;
    for (const HiScoreEntry& e : table.entries) {
        sum += static_cast<std::uint32_t>(e.score);
        for (const char c : e.name) {
            // signed char, as in 2002, on every platform
            sum += static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<signed char>(c)));
        }
    }
    return sum;
}

}  // namespace

std::optional<HiScoreTable> readScr(const Bytes& file, std::string& error) {
    if (file.size() != 244) {
        error = "a score file is 244 bytes";
        return std::nullopt;
    }
    ByteReader in(file);
    HiScoreTable table;
    for (std::size_t rank = 0; rank < table.entries.size(); ++rank) {
        const std::uint32_t key = scoreKey(rank);
        HiScoreEntry& e = table.entries[rank];
        e.score = static_cast<std::int32_t>(in.u32() ^ key);
        for (std::size_t j = 0; j < e.name.size(); j += 4) {
            const std::uint32_t word = in.u32() ^ key;
            for (std::size_t k = 0; k < 4; ++k) {
                e.name[j + k] = static_cast<char>((word >> (8 * k)) & 0xFFu);
            }
        }
    }
    if (in.u32() != scoreChecksum(table)) {
        error = "wrong score checksum";
        return std::nullopt;
    }
    return table;
}

Bytes writeScr(const HiScoreTable& table) {
    ByteWriter out;
    for (std::size_t rank = 0; rank < table.entries.size(); ++rank) {
        const std::uint32_t key = scoreKey(rank);
        const HiScoreEntry& e = table.entries[rank];
        out.u32(static_cast<std::uint32_t>(e.score) ^ key);
        for (std::size_t j = 0; j < e.name.size(); j += 4) {
            std::uint32_t word = 0;
            for (std::size_t k = 0; k < 4; ++k) {
                word |= static_cast<std::uint32_t>(static_cast<std::uint8_t>(e.name[j + k])) << (8 * k);
            }
            out.u32(word ^ key);
        }
    }
    out.u32(scoreChecksum(table));
    return out.take();
}

}  // namespace bb::core::assets
