#include "bb/core/assets/pack.hpp"

#include <algorithm>
#include <cstring>
#include <utility>

#include "bb/core/assets/byte_io.hpp"

namespace bb::core::assets {

namespace {

constexpr char kMagic[4] = {'B', 'B', 'P', 'K'};
constexpr std::uint32_t kVersion = 2;  // 2: level markers
constexpr std::size_t kHeaderSize = 32;
constexpr std::size_t kAssetRecordSize = 32;
constexpr std::size_t kChunkRecordSize = 32;

// Span operations: kind in bits 15-14, length (1..16383) in bits 13-0.
constexpr std::uint16_t kOpSkip = 0x0000;
constexpr std::uint16_t kOpCopy = 0x4000;
constexpr std::uint16_t kOpFill = 0x8000;
constexpr std::uint16_t kOpKindMask = 0xC000;
constexpr std::size_t kOpMaxLength = 0x3FFF;
constexpr std::size_t kMinFillRun = 3;  // shorter runs are copied

// Level platforms and walls are stored as runs; this marks "no platform".
constexpr std::int32_t kNoPlatform = LevelData::kNoPlatform;

void writeString(ByteWriter& out, std::string_view s) {
    out.u32(static_cast<std::uint32_t>(s.size()));
    out.bytes(s.data(), s.size());
    out.align(4);
}

bool readString(ByteReader& in, std::string& s) {
    const std::uint32_t size = in.u32();
    if (!in.canRead(size)) {
        return false;
    }
    s.assign(reinterpret_cast<const char*>(in.current()), size);
    in.skip(size);
    in.skip((4 - (size % 4)) % 4);
    return in.ok();
}

std::string lower(std::string_view name) {
    return assetName(name);
}

}  // namespace

std::uint32_t packNameHash(std::string_view name) noexcept {
    std::uint32_t h = 2166136261u;
    for (const char c : name) {
        h ^= static_cast<std::uint8_t>(c);
        h *= 16777619u;
    }
    return h;
}

// ---------------------------------------------------------------------------
// Spans

std::vector<std::uint8_t> encodeSpans(const Image16& image) {
    const std::uint16_t key = colorKey16(image.format);
    const std::size_t width = static_cast<std::size_t>(image.width);
    ByteWriter out;
    const std::size_t table = out.size();
    out.fill(0, 4 * static_cast<std::size_t>(image.height));

    for (int y = 0; y < image.height; ++y) {
        out.patchU32(table + 4 * static_cast<std::size_t>(y), static_cast<std::uint32_t>(out.size()));
        const std::uint16_t* row = image.pixels.data() + static_cast<std::size_t>(y) * width;
        std::size_t x = 0;
        std::size_t literalStart = 0;
        std::size_t literalCount = 0;
        const auto flushLiteral = [&]() {
            std::size_t i = 0;
            while (i < literalCount) {
                const std::size_t n = std::min(literalCount - i, kOpMaxLength);
                out.u16(static_cast<std::uint16_t>(kOpCopy | n));
                for (std::size_t k = 0; k < n; ++k) {
                    out.u16(row[literalStart + i + k]);
                }
                i += n;
            }
            literalCount = 0;
        };
        while (x < width) {
            std::size_t run = 1;
            while (x + run < width && row[x + run] == row[x]) {
                ++run;
            }
            if (row[x] == key || run >= kMinFillRun) {
                flushLiteral();
                for (std::size_t done = 0; done < run;) {
                    const std::size_t n = std::min(run - done, kOpMaxLength);
                    if (row[x] == key) {
                        out.u16(static_cast<std::uint16_t>(kOpSkip | n));
                    } else {
                        out.u16(static_cast<std::uint16_t>(kOpFill | n));
                        out.u16(row[x]);
                    }
                    done += n;
                }
            } else {
                if (literalCount == 0) {
                    literalStart = x;
                }
                literalCount += run;
            }
            x += run;
        }
        flushLiteral();
    }
    out.align(4);
    return out.take();
}

std::optional<Image16> decodeSpans(const std::uint8_t* data, std::size_t size, int width, int height,
                                   PixelFormat16 format, std::string& error) {
    if (width < 0 || height < 0) {
        error = "negative image size";
        return std::nullopt;
    }
    // A row takes a 4-byte offset and covers at most kOpMaxLength pixels
    // per 2-byte operation: refuse sizes the data cannot hold before
    // allocating them.
    if (static_cast<std::size_t>(height) > size / 4 ||
        (height > 0 && static_cast<std::size_t>(width) > (size / 2) * kOpMaxLength)) {
        error = "image size larger than its data";
        return std::nullopt;
    }
    Image16 image;
    image.width = width;
    image.height = height;
    image.format = format;
    const std::size_t w = static_cast<std::size_t>(width);
    image.pixels.resize(w * static_cast<std::size_t>(height));
    const std::uint16_t key = colorKey16(format);

    ByteReader table(data, size);
    for (int y = 0; y < height; ++y) {
        const std::uint32_t offset = table.u32();
        if (!table.ok() || offset > size) {
            error = "bad span row offset";
            return std::nullopt;
        }
        ByteReader in(data + offset, size - offset);
        std::uint16_t* row = image.pixels.data() + static_cast<std::size_t>(y) * w;
        std::size_t x = 0;
        while (x < w) {
            const std::uint16_t op = in.u16();
            const std::size_t n = op & kOpMaxLength;
            if (!in.ok() || n == 0 || n > w - x) {
                error = "bad span operation";
                return std::nullopt;
            }
            switch (op & kOpKindMask) {
                case kOpSkip:
                    std::fill(row + x, row + x + n, key);
                    break;
                case kOpCopy:
                    if (!in.canRead(2 * n)) {
                        error = "truncated span pixels";
                        return std::nullopt;
                    }
                    for (std::size_t k = 0; k < n; ++k) {
                        row[x + k] = in.u16();
                    }
                    break;
                case kOpFill: {
                    const std::uint16_t color = in.u16();
                    if (!in.ok()) {
                        error = "truncated span pixels";
                        return std::nullopt;
                    }
                    std::fill(row + x, row + x + n, color);
                    break;
                }
                default:
                    error = "unknown span operation";
                    return std::nullopt;
            }
            x += n;
        }
    }
    return image;
}

// ---------------------------------------------------------------------------
// Writer

PackWriter::Asset& PackWriter::add(std::string_view name, PackAssetType type) {
    assets_.emplace_back();
    Asset& asset = assets_.back();
    asset.name = lower(name);
    asset.type = type;
    return asset;
}

namespace {

std::array<std::int32_t, 6> imageMeta(const Image16& image, std::int32_t hotspotX, std::int32_t hotspotY,
                                      std::int32_t extra) {
    return {image.width, image.height, hotspotX, hotspotY, static_cast<std::int32_t>(image.format), extra};
}

}  // namespace

void PackWriter::addSpriteBank(std::string_view name, const SpriteBank& bank) {
    Asset& asset = add(name, PackAssetType::SpriteBank);
    for (const SpriteImage& sprite : bank.images) {
        asset.chunks.push_back(
            {encodeSpans(sprite.image), imageMeta(sprite.image, sprite.hotspotX, sprite.hotspotY, 0)});
    }
}

void PackWriter::addFont(std::string_view name, const Font& font) {
    Asset& asset = add(name, PackAssetType::Font);
    asset.param0 = font.lineHeight;
    asset.param1 = font.spaceWidth;
    for (const auto& [codePoint, image] : font.glyphs) {
        asset.chunks.push_back(
            {encodeSpans(image), imageMeta(image, 0, 0, static_cast<std::int32_t>(codePoint))});
    }
}

void PackWriter::addSoundBank(std::string_view name, const SoundBank& bank) {
    Asset& asset = add(name, PackAssetType::SoundBank);
    for (const SoundSample& sound : bank.sounds) {
        Chunk chunk;
        chunk.payload = sound.wav;
        chunk.meta[0] = sound.voices;
        asset.chunks.push_back(std::move(chunk));
    }
}

void PackWriter::addMusicList(std::string_view name, const MusicList& list) {
    ByteWriter out;
    out.u32(static_cast<std::uint32_t>(list.tracks.size()));
    for (const MusicTrack& track : list.tracks) {
        out.i32(track.type);
        writeString(out, track.path);
    }
    add(name, PackAssetType::MusicList).chunks.push_back({out.take(), {}});
}

void PackWriter::addMusic(std::string_view name, std::vector<std::uint8_t> file) {
    add(name, PackAssetType::Music).chunks.push_back({std::move(file), {}});
}

void PackWriter::addLevel(std::string_view name, const LevelData& level) {
    ByteWriter out;
    for (const std::string& asset : level.assets) {
        writeString(out, asset);
    }
    out.u32(static_cast<std::uint32_t>(level.screens.size()));
    for (const std::int32_t screen : level.screens) {
        out.i32(screen);
    }
    for (const std::int32_t v :
         {level.start1X, level.start1Y, level.start2X, level.start2Y, level.victoryX, level.victoryFlag1,
          level.victoryValue1, level.victoryFlag2, level.victoryValue2}) {
        out.i32(v);
    }
    // Platforms: runs of equal height, "no platform" left out.
    for (const auto& layer : level.platforms) {
        const std::size_t countAt = out.size();
        out.u32(0);
        std::uint32_t runs = 0;
        for (std::size_t x = 0; x < layer.size();) {
            std::size_t n = 1;
            while (x + n < layer.size() && layer[x + n] == layer[x]) {
                ++n;
            }
            if (layer[x] != kNoPlatform) {
                out.i32(static_cast<std::int32_t>(x));
                out.i32(static_cast<std::int32_t>(n));
                out.i32(layer[x]);
                ++runs;
            }
            x += n;
        }
        out.patchU32(countAt, runs);
    }
    // Walls: runs of equal non-zero cells, row by row.
    const std::size_t columns = static_cast<std::size_t>(level.wallColumns());
    for (const auto* walls : {&level.solidWalls, &level.bloodWalls}) {
        const std::size_t countAt = out.size();
        out.u32(0);
        std::uint32_t runs = 0;
        for (std::size_t row = 0; row < static_cast<std::size_t>(LevelData::kWallRows); ++row) {
            const std::uint8_t* cells = walls->data() + row * columns;
            for (std::size_t x = 0; x < columns;) {
                std::size_t n = 1;
                while (x + n < columns && cells[x + n] == cells[x]) {
                    ++n;
                }
                if (cells[x] != 0) {
                    out.u16(static_cast<std::uint16_t>(row));
                    out.u16(cells[x]);
                    out.u16(static_cast<std::uint16_t>(x));
                    out.u16(static_cast<std::uint16_t>(n));
                    ++runs;
                }
                x += n;
            }
        }
        out.patchU32(countAt, runs);
    }
    // Events, in file order. The 20-byte file name and the padding of the
    // 2002 record are never read by the game and are not stored.
    out.u32(static_cast<std::uint32_t>(level.events.size()));
    for (const LevelEventRecord& e : level.events) {
        for (const std::int32_t v :
             {e.type, e.activationX, e.id, e.x, e.bank, e.y, e.capacity, e.value, e.period}) {
            out.i32(v);
        }
        out.u8(e.temporary);
        out.fill(0, 3);
    }
    // Markers (version 2), by name.
    out.u32(static_cast<std::uint32_t>(level.markers.size()));
    for (const auto& [markerName, values] : level.markers) {
        writeString(out, markerName);
        out.u32(static_cast<std::uint32_t>(values.size()));
        for (const std::int32_t v : values) {
            out.i32(v);
        }
    }
    add(name, PackAssetType::Level).chunks.push_back({out.take(), {}});
}

void PackWriter::addCampaign(std::string_view name, const Campaign& campaign) {
    ByteWriter out;
    out.u32(static_cast<std::uint32_t>(campaign.entries.size()));
    for (const CampaignEntry& entry : campaign.entries) {
        out.i32(static_cast<std::int32_t>(entry.kind));
        out.i32(entry.levelKind);
        writeString(out, entry.path);
    }
    add(name, PackAssetType::Campaign).chunks.push_back({out.take(), {}});
}

void PackWriter::addStrings(std::string_view name, const StringTable& table) {
    ByteWriter out;
    out.u32(static_cast<std::uint32_t>(table.strings.size()));
    for (const auto& [id, text] : table.strings) {
        out.i32(id);
        writeString(out, text);
    }
    add(name, PackAssetType::StringTable).chunks.push_back({out.take(), {}});
}

void PackWriter::addScript(std::string_view name, std::string_view text) {
    add(name, PackAssetType::Script)
        .chunks.push_back({std::vector<std::uint8_t>(text.begin(), text.end()), {}});
}

std::optional<std::vector<std::uint8_t>> PackWriter::finish(std::string& error) const {
    // Index order: by name hash, then name.
    std::vector<const Asset*> order;
    order.reserve(assets_.size());
    for (const Asset& asset : assets_) {
        order.push_back(&asset);
    }
    // A total order on the names (equal names are refused below): the
    // result does not depend on the pointers.
    // NOLINTNEXTLINE(bugprone-nondeterministic-pointer-iteration-order)
    std::sort(order.begin(), order.end(), [](const Asset* a, const Asset* b) {
        const std::uint32_t ha = packNameHash(a->name);
        const std::uint32_t hb = packNameHash(b->name);
        return ha != hb ? ha < hb : a->name < b->name;
    });
    for (std::size_t i = 1; i < order.size(); ++i) {
        if (order[i - 1]->name == order[i]->name) {
            error = "two assets named " + order[i]->name;
            return std::nullopt;
        }
    }

    ByteWriter out;
    out.bytes(kMagic, 4);
    out.u32(kVersion);
    out.fill(0, kHeaderSize - 8);  // patched below

    // Payloads.
    struct Placed {
        std::uint32_t offset;
        std::uint32_t size;
        const std::array<std::int32_t, 6>* meta;
    };
    std::vector<Placed> chunks;
    for (const Asset* asset : order) {
        for (const Chunk& chunk : asset->chunks) {
            out.align(4);
            chunks.push_back({static_cast<std::uint32_t>(out.size()),
                              static_cast<std::uint32_t>(chunk.payload.size()), &chunk.meta});
            out.bytes(chunk.payload);
        }
    }
    out.align(4);

    // Names.
    const std::size_t namesOffset = out.size();
    std::vector<std::uint32_t> nameOffsets;
    for (const Asset* asset : order) {
        nameOffsets.push_back(static_cast<std::uint32_t>(out.size() - namesOffset));
        out.bytes(asset->name.data(), asset->name.size());
    }
    const std::size_t namesSize = out.size() - namesOffset;
    out.align(4);

    // Asset table.
    const std::size_t assetTable = out.size();
    std::uint32_t firstChunk = 0;
    for (std::size_t i = 0; i < order.size(); ++i) {
        const Asset* asset = order[i];
        out.u32(packNameHash(asset->name));
        out.u32(nameOffsets[i]);
        out.u32(static_cast<std::uint32_t>(asset->name.size()));
        out.u32(static_cast<std::uint32_t>(asset->type));
        out.u32(firstChunk);
        out.u32(static_cast<std::uint32_t>(asset->chunks.size()));
        out.i32(asset->param0);
        out.i32(asset->param1);
        firstChunk += static_cast<std::uint32_t>(asset->chunks.size());
    }

    // Chunk table.
    const std::size_t chunkTable = out.size();
    for (const Placed& chunk : chunks) {
        out.u32(chunk.offset);
        out.u32(chunk.size);
        for (const std::int32_t v : *chunk.meta) {
            out.i32(v);
        }
    }
    if (out.size() > 0xFFFFFFFFu) {
        error = "pack larger than 4 GB";
        return std::nullopt;
    }

    out.patchU32(8, static_cast<std::uint32_t>(order.size()));
    out.patchU32(12, static_cast<std::uint32_t>(chunks.size()));
    out.patchU32(16, static_cast<std::uint32_t>(assetTable));
    out.patchU32(20, static_cast<std::uint32_t>(chunkTable));
    out.patchU32(24, static_cast<std::uint32_t>(namesOffset));
    out.patchU32(28, static_cast<std::uint32_t>(namesSize));
    return out.take();
}

// ---------------------------------------------------------------------------
// Reader

namespace {

bool readAt(std::FILE* file, std::uint64_t offset, void* out, std::size_t size) {
    if (size == 0) {
        return true;
    }
    if (offset > 0x7FFFFFFFu || std::fseek(file, static_cast<long>(offset), SEEK_SET) != 0) {
        return false;
    }
    return std::fread(out, 1, size, file) == size;
}

}  // namespace

PackFile::~PackFile() {
    if (file_ != nullptr) {
        std::fclose(file_);
    }
}

std::unique_ptr<PackFile> PackFile::open(const std::string& path, std::string& error) {
    std::unique_ptr<PackFile> pack(new PackFile());
    pack->file_ = std::fopen(path.c_str(), "rb");
    if (pack->file_ == nullptr) {
        error = "cannot open " + path;
        return nullptr;
    }
    if (std::fseek(pack->file_, 0, SEEK_END) != 0) {
        error = "cannot seek in " + path;
        return nullptr;
    }
    const long end = std::ftell(pack->file_);
    if (end < 0) {
        error = "cannot size " + path;
        return nullptr;
    }
    const std::uint64_t fileSize = static_cast<std::uint64_t>(end);

    std::uint8_t header[kHeaderSize];
    if (!readAt(pack->file_, 0, header, kHeaderSize)) {
        error = path + ": truncated header";
        return nullptr;
    }
    ByteReader h(header, kHeaderSize);
    char magic[4];
    h.bytes(magic, 4);
    const std::uint32_t version = h.u32();
    const std::uint32_t assetCount = h.u32();
    const std::uint32_t chunkCount = h.u32();
    const std::uint32_t assetTable = h.u32();
    const std::uint32_t chunkTable = h.u32();
    const std::uint32_t namesOffset = h.u32();
    const std::uint32_t namesSize = h.u32();
    if (std::memcmp(magic, kMagic, 4) != 0) {
        error = path + ": not a Blip & Blop pack";
        return nullptr;
    }
    if (version != kVersion) {
        error = path + ": pack version " + std::to_string(version) + ", expected " + std::to_string(kVersion);
        return nullptr;
    }
    const auto fits = [fileSize](std::uint64_t offset, std::uint64_t size) {
        return offset <= fileSize && size <= fileSize - offset;
    };
    if (!fits(assetTable, std::uint64_t{assetCount} * kAssetRecordSize) ||
        !fits(chunkTable, std::uint64_t{chunkCount} * kChunkRecordSize) || !fits(namesOffset, namesSize)) {
        error = path + ": index outside the file";
        return nullptr;
    }

    std::vector<std::uint8_t> names(namesSize);
    std::vector<std::uint8_t> assetRecords(std::size_t{assetCount} * kAssetRecordSize);
    std::vector<std::uint8_t> chunkRecords(std::size_t{chunkCount} * kChunkRecordSize);
    if (!readAt(pack->file_, namesOffset, names.data(), names.size()) ||
        !readAt(pack->file_, assetTable, assetRecords.data(), assetRecords.size()) ||
        !readAt(pack->file_, chunkTable, chunkRecords.data(), chunkRecords.size())) {
        error = path + ": cannot read the index";
        return nullptr;
    }

    ByteReader c(chunkRecords);
    pack->chunks_.resize(chunkCount);
    for (Chunk& chunk : pack->chunks_) {
        chunk.offset = c.u32();
        chunk.size = c.u32();
        for (std::int32_t& v : chunk.meta) {
            v = c.i32();
        }
        if (!fits(chunk.offset, chunk.size)) {
            error = path + ": chunk outside the file";
            return nullptr;
        }
    }

    ByteReader a(assetRecords);
    pack->assets_.resize(assetCount);
    pack->hashes_.resize(assetCount);
    for (std::size_t i = 0; i < assetCount; ++i) {
        Asset& asset = pack->assets_[i];
        pack->hashes_[i] = a.u32();
        const std::uint32_t nameOffset = a.u32();
        const std::uint32_t nameSize = a.u32();
        asset.type = static_cast<PackAssetType>(a.u32());
        asset.firstChunk = a.u32();
        asset.chunkCount = a.u32();
        asset.param0 = a.i32();
        asset.param1 = a.i32();
        if (std::uint64_t{nameOffset} + nameSize > names.size() ||
            std::uint64_t{asset.firstChunk} + asset.chunkCount > chunkCount) {
            error = path + ": bad asset record";
            return nullptr;
        }
        asset.name.assign(reinterpret_cast<const char*>(names.data()) + nameOffset, nameSize);
        if (pack->hashes_[i] != packNameHash(asset.name) ||
            (i > 0 && pack->hashes_[i] < pack->hashes_[i - 1])) {
            error = path + ": index not sorted by name hash";
            return nullptr;
        }
    }
    return pack;
}

const PackFile::Asset* PackFile::find(std::string_view name) const {
    const std::string key = lower(name);
    const std::uint32_t hash = packNameHash(key);
    auto it = std::lower_bound(hashes_.begin(), hashes_.end(), hash);
    for (; it != hashes_.end() && *it == hash; ++it) {
        const Asset& asset = assets_[static_cast<std::size_t>(it - hashes_.begin())];
        if (asset.name == key) {
            return &asset;
        }
    }
    return nullptr;
}

std::optional<std::vector<std::uint8_t>> PackFile::read(const Chunk& chunk, std::string& error) const {
    std::vector<std::uint8_t> bytes(chunk.size);
    if (!readAt(file_, chunk.offset, bytes.data(), bytes.size())) {
        error = "cannot read a pack chunk";
        return std::nullopt;
    }
    return bytes;
}

// ---------------------------------------------------------------------------
// Source

namespace {

bool validLayout(std::int32_t layout) {
    return layout == 0 || layout == 1;
}

std::optional<Image16> chunkImage(const PackFile& pack, const PackFile::Chunk& chunk, std::string& error) {
    if (!validLayout(chunk.meta[4])) {
        error = "bad pixel layout";
        return std::nullopt;
    }
    const auto bytes = pack.read(chunk, error);
    if (!bytes) {
        return std::nullopt;
    }
    return decodeSpans(bytes->data(), bytes->size(), chunk.meta[0], chunk.meta[1],
                       static_cast<PixelFormat16>(chunk.meta[4]), error);
}

}  // namespace

const PackFile::Asset* PackSource::find(std::string_view name, PackAssetType type, std::string& error) const {
    const PackFile::Asset* asset = pack_->find(name);
    if (asset == nullptr) {
        error = "no asset " + lower(name) + " in the pack";
        return nullptr;
    }
    if (asset->type != type) {
        error = asset->name + " has another type in the pack";
        return nullptr;
    }
    return asset;
}

std::optional<std::vector<std::uint8_t>> PackSource::single(std::string_view name, PackAssetType type,
                                                            std::string& error) const {
    const PackFile::Asset* asset = find(name, type, error);
    if (asset == nullptr) {
        return std::nullopt;
    }
    if (asset->chunkCount != 1) {
        error = asset->name + ": expected one chunk";
        return std::nullopt;
    }
    return pack_->read(pack_->chunk(asset->firstChunk), error);
}

std::optional<SpriteImage> PackSpriteBank::image(std::size_t index, std::string& error) const {
    if (index >= asset_.chunkCount) {
        error = asset_.name + ": no image " + std::to_string(index);
        return std::nullopt;
    }
    const PackFile::Chunk& chunk = pack_->chunk(asset_.firstChunk + static_cast<std::uint32_t>(index));
    auto image = chunkImage(*pack_, chunk, error);
    if (!image) {
        error = asset_.name + ": " + error;
        return std::nullopt;
    }
    SpriteImage sprite;
    sprite.hotspotX = chunk.meta[2];
    sprite.hotspotY = chunk.meta[3];
    sprite.image = std::move(*image);
    return sprite;
}

std::optional<PackSpriteBank> PackSource::openSpriteBank(std::string_view name, std::string& error) const {
    const PackFile::Asset* asset = find(name, PackAssetType::SpriteBank, error);
    if (asset == nullptr) {
        return std::nullopt;
    }
    return PackSpriteBank(pack_, *asset);
}

std::optional<SpriteBank> PackSource::spriteBank(std::string_view name, std::string& error) const {
    const auto bank = openSpriteBank(name, error);
    if (!bank) {
        return std::nullopt;
    }
    SpriteBank result;
    result.images.reserve(bank->size());
    for (std::size_t i = 0; i < bank->size(); ++i) {
        auto sprite = bank->image(i, error);
        if (!sprite) {
            return std::nullopt;
        }
        result.images.push_back(std::move(*sprite));
    }
    return result;
}

std::optional<Font> PackSource::font(std::string_view name, std::string& error) const {
    const PackFile::Asset* asset = find(name, PackAssetType::Font, error);
    if (asset == nullptr) {
        return std::nullopt;
    }
    Font font;
    font.lineHeight = asset->param0;
    font.spaceWidth = asset->param1;
    for (std::uint32_t i = 0; i < asset->chunkCount; ++i) {
        const PackFile::Chunk& chunk = pack_->chunk(asset->firstChunk + i);
        auto glyph = chunkImage(*pack_, chunk, error);
        if (!glyph) {
            error = asset->name + ": " + error;
            return std::nullopt;
        }
        font.glyphs[static_cast<std::uint32_t>(chunk.meta[5])] = std::move(*glyph);
    }
    return font;
}

std::optional<SoundBank> PackSource::soundBank(std::string_view name, std::string& error) const {
    const PackFile::Asset* asset = find(name, PackAssetType::SoundBank, error);
    if (asset == nullptr) {
        return std::nullopt;
    }
    SoundBank bank;
    for (std::uint32_t i = 0; i < asset->chunkCount; ++i) {
        const PackFile::Chunk& chunk = pack_->chunk(asset->firstChunk + i);
        auto wav = pack_->read(chunk, error);
        if (!wav) {
            return std::nullopt;
        }
        bank.sounds.push_back({chunk.meta[0], std::move(*wav)});
    }
    return bank;
}

std::optional<MusicList> PackSource::musicList(std::string_view name, std::string& error) const {
    const auto bytes = single(name, PackAssetType::MusicList, error);
    if (!bytes) {
        return std::nullopt;
    }
    ByteReader in(*bytes);
    MusicList list;
    const std::uint32_t count = in.u32();
    for (std::uint32_t i = 0; i < count && in.ok(); ++i) {
        MusicTrack track;
        track.type = in.i32();
        if (!readString(in, track.path)) {
            break;
        }
        list.tracks.push_back(std::move(track));
    }
    if (!in.ok() || list.tracks.size() != count) {
        error = lower(name) + ": bad music list";
        return std::nullopt;
    }
    return list;
}

std::optional<std::vector<std::uint8_t>> PackSource::music(std::string_view name, std::string& error) const {
    return single(name, PackAssetType::Music, error);
}

std::optional<LevelData> PackSource::level(std::string_view name, std::string& error) const {
    const auto bytes = single(name, PackAssetType::Level, error);
    if (!bytes) {
        return std::nullopt;
    }
    const auto bad = [&](const char* what) {
        error = lower(name) + ": bad level data (" + what + ")";
        return std::nullopt;
    };
    ByteReader in(*bytes);
    LevelData level;
    for (std::string& asset : level.assets) {
        if (!readString(in, asset)) {
            return bad("asset names");
        }
    }
    const std::uint32_t screens = in.u32();
    if (!in.ok() || screens == 0 || screens > 1000) {
        return bad("screens");
    }
    level.screens.resize(screens);
    for (std::int32_t& screen : level.screens) {
        screen = in.i32();
    }
    for (std::int32_t* v :
         {&level.start1X, &level.start1Y, &level.start2X, &level.start2Y, &level.victoryX,
          &level.victoryFlag1, &level.victoryValue1, &level.victoryFlag2, &level.victoryValue2}) {
        *v = in.i32();
    }
    const std::size_t width = static_cast<std::size_t>(level.width());
    for (auto& layer : level.platforms) {
        layer.assign(width, kNoPlatform);
        const std::uint32_t runs = in.u32();
        for (std::uint32_t r = 0; r < runs && in.ok(); ++r) {
            const std::int32_t x = in.i32();
            const std::int32_t n = in.i32();
            const std::int32_t y = in.i32();
            if (x < 0 || n <= 0 || static_cast<std::size_t>(x) + static_cast<std::size_t>(n) > width) {
                return bad("platform run");
            }
            std::fill(layer.begin() + x, layer.begin() + x + n, y);
        }
    }
    const std::size_t columns = static_cast<std::size_t>(level.wallColumns());
    for (auto* walls : {&level.solidWalls, &level.bloodWalls}) {
        walls->assign(columns * static_cast<std::size_t>(LevelData::kWallRows), 0);
        const std::uint32_t runs = in.u32();
        for (std::uint32_t r = 0; r < runs && in.ok(); ++r) {
            const std::size_t row = in.u16();
            const std::uint16_t value = in.u16();
            const std::size_t x = in.u16();
            const std::size_t n = in.u16();
            if (row >= static_cast<std::size_t>(LevelData::kWallRows) || value > 0xFF || x + n > columns) {
                return bad("wall run");
            }
            std::fill(walls->begin() + static_cast<std::ptrdiff_t>(row * columns + x),
                      walls->begin() + static_cast<std::ptrdiff_t>(row * columns + x + n),
                      static_cast<std::uint8_t>(value));
        }
    }
    const std::uint32_t events = in.u32();
    if (!in.ok() || !in.canRead(std::size_t{events} * 40)) {
        return bad("events");
    }
    level.events.resize(events);
    for (LevelEventRecord& e : level.events) {
        for (std::int32_t* v :
             {&e.type, &e.activationX, &e.id, &e.x, &e.bank, &e.y, &e.capacity, &e.value, &e.period}) {
            *v = in.i32();
        }
        e.temporary = in.u8();
        in.skip(3);
    }
    const std::uint32_t markers = in.u32();
    if (!in.ok() || !in.canRead(std::size_t{markers} * 8)) {
        return bad("markers");
    }
    for (std::uint32_t m = 0; m < markers; ++m) {
        std::string markerName;
        if (!readString(in, markerName)) {
            return bad("marker name");
        }
        const std::uint32_t count = in.u32();
        if (!in.ok() || !in.canRead(std::size_t{count} * 4)) {
            return bad("marker values");
        }
        std::vector<std::int32_t>& values = level.markers[markerName];
        values.resize(count);
        for (std::int32_t& v : values) {
            v = in.i32();
        }
    }
    if (!in.ok() || in.remaining() != 0) {
        return bad("size");
    }
    return level;
}

std::optional<Campaign> PackSource::campaign(std::string_view name, std::string& error) const {
    const auto bytes = single(name, PackAssetType::Campaign, error);
    if (!bytes) {
        return std::nullopt;
    }
    ByteReader in(*bytes);
    Campaign campaign;
    const std::uint32_t count = in.u32();
    for (std::uint32_t i = 0; i < count && in.ok(); ++i) {
        CampaignEntry entry;
        const std::int32_t kind = in.i32();
        entry.levelKind = in.i32();
        if (kind < 0 || kind > 2 || !readString(in, entry.path)) {
            break;
        }
        entry.kind = static_cast<CampaignEntry::Kind>(kind);
        campaign.entries.push_back(std::move(entry));
    }
    if (!in.ok() || campaign.entries.size() != count) {
        error = lower(name) + ": bad campaign";
        return std::nullopt;
    }
    return campaign;
}

std::optional<StringTable> PackSource::strings(std::string_view name, std::string& error) const {
    const auto bytes = single(name, PackAssetType::StringTable, error);
    if (!bytes) {
        return std::nullopt;
    }
    ByteReader in(*bytes);
    StringTable table;
    const std::uint32_t count = in.u32();
    for (std::uint32_t i = 0; i < count && in.ok(); ++i) {
        const std::int32_t id = in.i32();
        std::string text;
        if (!readString(in, text)) {
            break;
        }
        table.strings[id] = std::move(text);
    }
    if (!in.ok() || table.strings.size() != count) {
        error = lower(name) + ": bad string table";
        return std::nullopt;
    }
    return table;
}

std::optional<std::string> PackSource::script(std::string_view name, std::string& error) const {
    const auto bytes = single(name, PackAssetType::Script, error);
    if (!bytes) {
        return std::nullopt;
    }
    return std::string(bytes->begin(), bytes->end());
}

}  // namespace bb::core::assets
