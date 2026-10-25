#pragma once

// Runtime pack, version 1 (docs/formats/runtime-pack.md): one file holding
// every game asset, with an index so that each asset - and each image of a
// sprite bank - can be read on its own (decor screens are loaded lazily on
// the smallest targets). Little-endian, 4-byte aligned, no padding inside
// records, no compression library.

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "bb/core/assets/asset_source.hpp"
#include "bb/core/file_reader.hpp"

namespace bb::core::assets {

enum class PackAssetType : std::uint32_t {
    None = 0,  // not set
    SpriteBank = 1,
    Font = 2,
    SoundBank = 3,
    MusicList = 4,
    Music = 5,
    Level = 6,
    Campaign = 7,
    StringTable = 8,
    Script = 9,
};

// 32-bit FNV-1a of an asset name (the pack index is sorted by it).
std::uint32_t packNameHash(std::string_view name) noexcept;

// Sprite payload: a row-offset table, then per row a list of operations
// covering the width exactly: skip n (pixels of the layout's key colour),
// copy n (n pixels follow), fill n (one pixel follows). Decoding restores
// the key colour in skipped pixels, so the image is exact.
std::vector<std::uint8_t> encodeSpans(const Image16& image);
std::optional<Image16> decodeSpans(const std::uint8_t* data, std::size_t size, int width, int height,
                                   PixelFormat16 format, std::string& error);

// Builds a pack in memory. Asset names are legacy file names; they are
// stored lower-case ("snufenn.gfx").
class PackWriter {
public:
    void addSpriteBank(std::string_view name, const SpriteBank& bank);
    void addFont(std::string_view name, const Font& font);
    void addSoundBank(std::string_view name, const SoundBank& bank);
    void addMusicList(std::string_view name, const MusicList& list);
    void addMusic(std::string_view name, std::vector<std::uint8_t> file);
    void addLevel(std::string_view name, const LevelData& level);
    void addCampaign(std::string_view name, const Campaign& campaign);
    void addStrings(std::string_view name, const StringTable& table);
    void addScript(std::string_view name, std::string_view text);

    // The pack file. Fails when two assets have the same name.
    std::optional<std::vector<std::uint8_t>> finish(std::string& error) const;

private:
    struct Chunk {
        std::vector<std::uint8_t> payload;
        std::array<std::int32_t, 6> meta{};
    };
    struct Asset {
        std::string name;
        PackAssetType type{};
        std::int32_t param0 = 0;
        std::int32_t param1 = 0;
        std::vector<Chunk> chunks;
    };
    Asset& add(std::string_view name, PackAssetType type);

    std::vector<Asset> assets_;
};

// Read access to a pack file. The index is read when the pack is opened;
// payloads are read on demand.
class PackFile {
public:
    struct Asset {
        std::string name;
        PackAssetType type{};
        std::uint32_t firstChunk = 0;
        std::uint32_t chunkCount = 0;
        std::int32_t param0 = 0;
        std::int32_t param1 = 0;
    };
    struct Chunk {
        std::uint32_t offset = 0;
        std::uint32_t size = 0;
        std::array<std::int32_t, 6> meta{};
    };

    static std::unique_ptr<PackFile> open(const std::string& path, std::string& error,
                                          const FileReader& files = hostFiles());
    ~PackFile();
    PackFile(const PackFile&) = delete;
    PackFile& operator=(const PackFile&) = delete;

    const std::vector<Asset>& assets() const noexcept { return assets_; }
    const Asset* find(std::string_view name) const;
    const Chunk& chunk(std::uint32_t index) const { return chunks_[index]; }
    std::optional<std::vector<std::uint8_t>> read(const Chunk& chunk, std::string& error) const;

private:
    PackFile() = default;

    std::unique_ptr<ReadableFile> file_;
    std::vector<Asset> assets_;  // sorted by (hash, name)
    std::vector<std::uint32_t> hashes_;
    std::vector<Chunk> chunks_;
};

// One sprite bank of a pack, with images decoded one by one on demand.
class PackSpriteBank {
public:
    PackSpriteBank(std::shared_ptr<const PackFile> pack, PackFile::Asset asset)
        : pack_(std::move(pack)), asset_(std::move(asset)) {}

    std::size_t size() const noexcept { return asset_.chunkCount; }
    std::optional<SpriteImage> image(std::size_t index, std::string& error) const;

private:
    std::shared_ptr<const PackFile> pack_;
    PackFile::Asset asset_;
};

// The assets of a pack file.
class PackSource final : public AssetSource {
public:
    explicit PackSource(std::shared_ptr<const PackFile> pack) : pack_(std::move(pack)) {}

    std::optional<SpriteBank> spriteBank(std::string_view name, std::string& error) const override;
    std::optional<Font> font(std::string_view name, std::string& error) const override;
    std::optional<SoundBank> soundBank(std::string_view name, std::string& error) const override;
    std::optional<MusicList> musicList(std::string_view name, std::string& error) const override;
    std::optional<std::vector<std::uint8_t>> music(std::string_view name, std::string& error) const override;
    std::optional<LevelData> level(std::string_view name, std::string& error) const override;
    std::optional<Campaign> campaign(std::string_view name, std::string& error) const override;
    std::optional<StringTable> strings(std::string_view name, std::string& error) const override;
    std::optional<std::string> script(std::string_view name, std::string& error) const override;

    // Lazy access to the images of a bank.
    std::optional<PackSpriteBank> openSpriteBank(std::string_view name, std::string& error) const;

private:
    const PackFile::Asset* find(std::string_view name, PackAssetType type, std::string& error) const;
    std::optional<std::vector<std::uint8_t>> single(std::string_view name, PackAssetType type,
                                                    std::string& error) const;

    std::shared_ptr<const PackFile> pack_;
};

// The assets of an overlay pack in place of those of another source; the
// others from that source (the level editor's test of one level).
class OverlayPackSource final : public AssetSource {
public:
    OverlayPackSource(std::shared_ptr<const PackFile> overlay, const AssetSource& base)
        : overlay_(overlay), overlaySource_(std::move(overlay)), base_(base) {}

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
    const AssetSource& from(std::string_view name) const {
        return overlay_->find(name) != nullptr ? static_cast<const AssetSource&>(overlaySource_) : base_;
    }

    std::shared_ptr<const PackFile> overlay_;
    PackSource overlaySource_;
    const AssetSource& base_;
};

}  // namespace bb::core::assets
