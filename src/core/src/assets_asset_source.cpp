#include "bb/core/assets/asset_source.hpp"

#include <cctype>

#include "bb/core/assets/legacy_formats.hpp"
#include "bb/core/assets/text_encoding.hpp"
#include "bb/core/vfs.hpp"

namespace bb::core::assets {

std::string assetName(std::string_view legacyPath) {
    const std::size_t slash = legacyPath.find_last_of("/\\");
    const std::string_view file = slash == std::string_view::npos ? legacyPath : legacyPath.substr(slash + 1);
    std::string name;
    name.reserve(file.size());
    for (const char c : file) {
        name.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return name;
}

std::optional<std::vector<std::uint8_t>> LegacyDataSource::read(std::string_view name,
                                                                std::string& error) const {
    auto bytes = files_.readAll(name);
    if (!bytes) {
        error = "cannot read " + std::string(name);
    }
    return bytes;
}

namespace {

// Runs a legacy reader on a file and prefixes its errors with the name.
template <typename T, typename Reader>
std::optional<T> parse(const std::optional<std::vector<std::uint8_t>>& bytes, std::string_view name,
                       std::string& error, Reader reader) {
    if (!bytes) {
        return std::nullopt;
    }
    auto result = reader(*bytes, error);
    if (!result) {
        error = std::string(name) + ": " + error;
    }
    return result;
}

}  // namespace

std::optional<SpriteBank> LegacyDataSource::spriteBank(std::string_view name, std::string& error) const {
    return parse<SpriteBank>(read(name, error), name, error, readGfx);
}

std::optional<Font> LegacyDataSource::font(std::string_view name, std::string& error) const {
    return parse<Font>(read(name, error), name, error, readLft);
}

std::optional<SoundBank> LegacyDataSource::soundBank(std::string_view name, std::string& error) const {
    return parse<SoundBank>(read(name, error), name, error, readSfx);
}

std::optional<MusicList> LegacyDataSource::musicList(std::string_view name, std::string& error) const {
    return parse<MusicList>(read(name, error), name, error, readMbk);
}

std::optional<std::vector<std::uint8_t>> LegacyDataSource::music(std::string_view name,
                                                                 std::string& error) const {
    return read(name, error);
}

std::optional<LevelData> LegacyDataSource::level(std::string_view name, std::string& error) const {
    auto level = parse<LevelData>(read(name, error), name, error, readLvl);
    if (level) {
        // The 2002 format cannot hold them: the 2002 values.
        level->markers = legacyLevelMarkers(name);
    }
    return level;
}

std::optional<Campaign> LegacyDataSource::campaign(std::string_view name, std::string& error) const {
    return parse<Campaign>(read(name, error), name, error, readLst);
}

std::optional<StringTable> LegacyDataSource::strings(std::string_view name, std::string& error) const {
    return parse<StringTable>(read(name, error), name, error, readDat);
}

std::optional<std::string> LegacyDataSource::script(std::string_view name, std::string& error) const {
    const auto bytes = read(name, error);
    if (!bytes) {
        return std::nullopt;
    }
    return windows1252ToUtf8(std::string_view(reinterpret_cast<const char*>(bytes->data()), bytes->size()));
}

}  // namespace bb::core::assets
