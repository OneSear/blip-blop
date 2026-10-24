#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "bb/core/assets/asset_types.hpp"

namespace bb::core {
class Vfs;
}

namespace bb::core::assets {

// Where the game's assets come from (migration.md Phase 2): the 2002 data
// directory or the runtime packs. The game asks for an asset by its legacy
// file name ("snufenn.gfx", or "data/snufenn.gfx" as the 2002 code and data
// write it) and gets the decoded structure, whatever the storage.
//
// Every function returns nullopt with a message in error when the asset is
// missing or invalid.
class AssetSource {
public:
    virtual ~AssetSource() = default;

    virtual std::optional<SpriteBank> spriteBank(std::string_view name, std::string& error) const = 0;
    virtual std::optional<Font> font(std::string_view name, std::string& error) const = 0;
    virtual std::optional<SoundBank> soundBank(std::string_view name, std::string& error) const = 0;
    virtual std::optional<MusicList> musicList(std::string_view name, std::string& error) const = 0;
    // A music file (Ogg Vorbis or MP3), whole.
    virtual std::optional<std::vector<std::uint8_t>> music(std::string_view name,
                                                           std::string& error) const = 0;
    virtual std::optional<LevelData> level(std::string_view name, std::string& error) const = 0;
    virtual std::optional<Campaign> campaign(std::string_view name, std::string& error) const = 0;
    virtual std::optional<StringTable> strings(std::string_view name, std::string& error) const = 0;
    // A cutscene (.cin) or dialogue (.rpg) script, UTF-8.
    virtual std::optional<std::string> script(std::string_view name, std::string& error) const = 0;
};

// The 2002 files under a data directory, read with legacy_formats.hpp.
class LegacyDataSource final : public AssetSource {
public:
    explicit LegacyDataSource(const Vfs& files) : files_(files) {}

    std::optional<SpriteBank> spriteBank(std::string_view name, std::string& error) const override;
    std::optional<Font> font(std::string_view name, std::string& error) const override;
    std::optional<SoundBank> soundBank(std::string_view name, std::string& error) const override;
    std::optional<MusicList> musicList(std::string_view name, std::string& error) const override;
    std::optional<std::vector<std::uint8_t>> music(std::string_view name, std::string& error) const override;
    std::optional<LevelData> level(std::string_view name, std::string& error) const override;
    std::optional<Campaign> campaign(std::string_view name, std::string& error) const override;
    std::optional<StringTable> strings(std::string_view name, std::string& error) const override;
    std::optional<std::string> script(std::string_view name, std::string& error) const override;

private:
    std::optional<std::vector<std::uint8_t>> read(std::string_view name, std::string& error) const;

    const Vfs& files_;
};

// The asset name of a legacy path: the file name, lower-case ("data\\Snork.mbk"
// -> "snork.mbk"). Directories are dropped: every 2002 asset sits directly
// in data/.
std::string assetName(std::string_view legacyPath);

}  // namespace bb::core::assets
