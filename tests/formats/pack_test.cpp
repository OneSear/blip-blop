// The runtime pack (docs/formats/runtime-pack.md): every shipped asset
// written to a pack and read back must equal what the legacy readers give.

#include <doctest.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "bb/core/assets/legacy_formats.hpp"
#include "bb/core/assets/pack.hpp"
#include "bb/core/vfs.hpp"

namespace fs = std::filesystem;
using namespace bb::core::assets;

namespace {

const fs::path kData = BB_TEST_DATA_DIR;

std::vector<std::string> names(const std::string& extension) {
    std::vector<std::string> out;
    for (const auto& entry : fs::directory_iterator(kData)) {
        if (entry.path().extension() == extension) {
            out.push_back(entry.path().filename().string());
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

bool sameFont(const Font& a, const Font& b) {
    return a.lineHeight == b.lineHeight && a.spaceWidth == b.spaceWidth && a.glyphs == b.glyphs;
}

bool sameSounds(const SoundBank& a, const SoundBank& b) {
    if (a.sounds.size() != b.sounds.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.sounds.size(); ++i) {
        if (a.sounds[i].voices != b.sounds[i].voices || a.sounds[i].wav != b.sounds[i].wav) {
            return false;
        }
    }
    return true;
}

// The pack does not keep the two fields of the 2002 event record the game
// never reads (padding and file name).
LevelData withoutUnreadEventBytes(LevelData level) {
    for (auto& e : level.events) {
        e.padding = {};
        e.file = {};
    }
    return level;
}

bool sameLevel(const LevelData& a, const LevelData& b) {
    return a.assets == b.assets && a.screens == b.screens && a.start1X == b.start1X && a.start1Y == b.start1Y &&
           a.start2X == b.start2X && a.start2Y == b.start2Y && a.victoryX == b.victoryX &&
           a.victoryFlag1 == b.victoryFlag1 && a.victoryValue1 == b.victoryValue1 &&
           a.victoryFlag2 == b.victoryFlag2 && a.victoryValue2 == b.victoryValue2 && a.platforms == b.platforms &&
           a.solidWalls == b.solidWalls && a.bloodWalls == b.bloodWalls && a.events == b.events;
}

struct TempFile {
    fs::path path = fs::temp_directory_path() / "bb_pack_test.bbpack";
    ~TempFile() { fs::remove(path); }
};

}  // namespace

TEST_CASE("spans round-trip images with transparent, flat and noisy rows") {
    Image16 image;
    image.width = 40000 / 100;  // 400
    image.height = 3;
    image.format = PixelFormat16::Rgb555;
    const std::uint16_t key = colorKey16(image.format);
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            const std::uint16_t v = y == 0 ? key : y == 1 ? static_cast<std::uint16_t>(x / 7) : static_cast<std::uint16_t>((x * 7919) & 0x7FFF);
            image.pixels.push_back(v);
        }
    }
    const auto spans = encodeSpans(image);
    std::string error;
    const auto back = decodeSpans(spans.data(), spans.size(), image.width, image.height, image.format, error);
    REQUIRE_MESSAGE(back.has_value(), error);
    CHECK(*back == image);
    for (std::size_t size = 0; size < spans.size(); size += 7) {
        CHECK_FALSE(decodeSpans(spans.data(), size, image.width, image.height, image.format, error).has_value());
    }
}

TEST_CASE("every shipped asset reads back from a pack exactly as from the legacy files") {
    const bb::core::Vfs files(kData.string());
    const LegacyDataSource legacy(files);
    PackWriter writer;
    std::string error;

    for (const auto& name : names(".gfx")) {
        const auto bank = legacy.spriteBank(name, error);
        REQUIRE_MESSAGE(bank.has_value(), error);
        writer.addSpriteBank(name, *bank);
    }
    for (const auto& name : names(".lft")) {
        writer.addFont(name, *legacy.font(name, error));
    }
    for (const auto& name : names(".sfx")) {
        writer.addSoundBank(name, *legacy.soundBank(name, error));
    }
    for (const auto& name : names(".mbk")) {
        writer.addMusicList(name, *legacy.musicList(name, error));
    }
    for (const auto& name : names(".zik")) {
        writer.addMusic(name, *legacy.music(name, error));
    }
    for (const auto& name : names(".lvl")) {
        writer.addLevel(name, *legacy.level(name, error));
    }
    writer.addCampaign("bb.lst", *legacy.campaign("bb.lst", error));
    writer.addStrings("uk.dat", *legacy.strings("uk.dat", error));
    for (const auto& ext : {".cin", ".rpg"}) {
        for (const auto& name : names(ext)) {
            writer.addScript(name, *legacy.script(name, error));
        }
    }

    TempFile temp;
    {
        const auto bytes = writer.finish(error);
        REQUIRE_MESSAGE(bytes.has_value(), error);
        std::ofstream out(temp.path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes->data()), static_cast<std::streamsize>(bytes->size()));
        MESSAGE("pack size: " << bytes->size() << " bytes");
    }
    std::shared_ptr<const PackFile> file = PackFile::open(temp.path.string(), error);
    REQUIRE_MESSAGE(file != nullptr, error);
    const PackSource pack(file);

    for (const auto& name : names(".gfx")) {
        CAPTURE(name);
        const auto a = legacy.spriteBank(name, error);
        const auto b = pack.spriteBank("data/" + name, error);  // legacy-style names work too
        REQUIRE_MESSAGE(b.has_value(), error);
        CHECK(a->images == b->images);
    }
    for (const auto& name : names(".lft")) {
        CAPTURE(name);
        CHECK(sameFont(*legacy.font(name, error), *pack.font(name, error)));
    }
    for (const auto& name : names(".sfx")) {
        CAPTURE(name);
        CHECK(sameSounds(*legacy.soundBank(name, error), *pack.soundBank(name, error)));
    }
    for (const auto& name : names(".mbk")) {
        CAPTURE(name);
        const auto a = legacy.musicList(name, error);
        const auto b = pack.musicList(name, error);
        REQUIRE(b.has_value());
        REQUIRE(a->tracks.size() == b->tracks.size());
        for (std::size_t i = 0; i < a->tracks.size(); ++i) {
            CHECK(a->tracks[i].type == b->tracks[i].type);
            CHECK(a->tracks[i].path == b->tracks[i].path);
        }
    }
    for (const auto& name : names(".zik")) {
        CAPTURE(name);
        CHECK(*legacy.music(name, error) == *pack.music(name, error));
    }
    for (const auto& name : names(".lvl")) {
        CAPTURE(name);
        const auto b = pack.level(name, error);
        REQUIRE_MESSAGE(b.has_value(), error);
        CHECK(sameLevel(withoutUnreadEventBytes(*legacy.level(name, error)), *b));
    }
    {
        const auto a = legacy.campaign("bb.lst", error);
        const auto b = pack.campaign("bb.lst", error);
        REQUIRE(b.has_value());
        REQUIRE(a->entries.size() == b->entries.size());
        for (std::size_t i = 0; i < a->entries.size(); ++i) {
            CHECK(a->entries[i].kind == b->entries[i].kind);
            CHECK(a->entries[i].levelKind == b->entries[i].levelKind);
            CHECK(a->entries[i].path == b->entries[i].path);
        }
    }
    CHECK(legacy.strings("uk.dat", error)->strings == pack.strings("uk.dat", error)->strings);
    for (const auto& ext : {".cin", ".rpg"}) {
        for (const auto& name : names(ext)) {
            CHECK(*legacy.script(name, error) == *pack.script(name, error));
        }
    }

    // Lazy access: one decor screen without decoding the bank.
    const auto decor = pack.openSpriteBank("videofond.gfx", error);
    REQUIRE(decor.has_value());
    CHECK(decor->size() == legacy.spriteBank("videofond.gfx", error)->images.size());
    const auto screen = decor->image(5, error);
    REQUIRE(screen.has_value());
    CHECK(*screen == legacy.spriteBank("videofond.gfx", error)->images[5]);
    CHECK_FALSE(decor->image(decor->size(), error).has_value());

    // Wrong names and types are reported.
    CHECK_FALSE(pack.spriteBank("nothing.gfx", error).has_value());
    CHECK_FALSE(pack.font("misc.gfx", error).has_value());
}

TEST_CASE("damaged pack files are rejected") {
    PackWriter writer;
    writer.addScript("a.cin", "endscene()");
    std::string error;
    const auto bytes = writer.finish(error);
    REQUIRE(bytes.has_value());
    TempFile temp;
    for (std::size_t size = 0; size < bytes->size(); ++size) {
        {
            std::ofstream out(temp.path, std::ios::binary | std::ios::trunc);
            out.write(reinterpret_cast<const char*>(bytes->data()), static_cast<std::streamsize>(size));
        }
        CHECK(PackFile::open(temp.path.string(), error) == nullptr);
    }
    PackWriter twice;
    twice.addScript("a.cin", "x");
    twice.addScript("A.CIN", "y");
    CHECK_FALSE(twice.finish(error).has_value());
}

TEST_CASE("an overlay pack replaces the assets it holds, and only those") {
    const bb::core::Vfs files(BB_TEST_DATA_DIR);
    const LegacyDataSource base(files);
    std::string error;
    auto level = base.level("data/snuf1.lvl", error);
    REQUIRE_MESSAGE(level.has_value(), error);
    level->victoryX = 1234;  // the edited level

    PackWriter writer;
    writer.addLevel("snuf1.lvl", *level);
    const auto bytes = writer.finish(error);
    REQUIRE(bytes.has_value());
    TempFile temp;
    {
        std::ofstream out(temp.path, std::ios::binary | std::ios::trunc);
        out.write(reinterpret_cast<const char*>(bytes->data()), static_cast<std::streamsize>(bytes->size()));
    }
    std::shared_ptr<const PackFile> overlay = PackFile::open(temp.path.string(), error);
    REQUIRE_MESSAGE(overlay != nullptr, error);
    const OverlayPackSource source(overlay, base);

    const auto edited = source.level("data/snuf1.lvl", error);
    REQUIRE(edited.has_value());
    CHECK(edited->victoryX == 1234);
    CHECK(sameLevel(withoutUnreadEventBytes(*level), *edited));

    const auto other = source.level("data/snuf2.lvl", error);
    const auto otherBase = base.level("data/snuf2.lvl", error);
    REQUIRE(other.has_value());
    REQUIRE(otherBase.has_value());
    CHECK(other->victoryX == otherBase->victoryX);
    CHECK(source.spriteBank("data/snufenn.gfx", error).has_value());
    CHECK_FALSE(source.level("data/nothing.lvl", error).has_value());
}
