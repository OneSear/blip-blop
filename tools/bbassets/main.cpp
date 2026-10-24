// bbassets - the asset tool of the migration (migration.md Phase 2).
//
//   bbassets extract <data-dir> <source-dir>   2002 files -> source assets
//   bbassets pack <source-dir> <pack>          source assets -> runtime pack
//   bbassets pack-legacy <data-dir> <pack>     2002 files -> runtime pack
//   bbassets legacy <source-dir> <out-dir>     source assets -> 2002 files
//   bbassets verify <source-dir> <data-dir>    source assets == 2002 files?
//   bbassets ids <source-dir> <include-dir>    C++ constants of sprite and sound names
//   bbassets ids-check <source-dir> <include-dir>   are they up to date?
//
// The plan's bbpack, bbfont, bbsfx, bbtext and bblevel are the sprite,
// font, sound, text and level parts of these commands.

#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "bb/core/assets/legacy_formats.hpp"
#include "bb/core/assets/pack.hpp"
#include "bb/core/assets/text_encoding.hpp"
#include "source_assets.hpp"
#include "sprite_ids.hpp"

namespace fs = std::filesystem;
using namespace bb::core::assets;
using bb::tools::AssetVisitor;
using bb::tools::Bytes;

namespace {

int fail(const std::string& message) {
    std::fprintf(stderr, "bbassets: %s\n", message.c_str());
    return 1;
}

bool writeFile(const fs::path& path, const Bytes& bytes, std::string& error) {
    std::error_code ec;
    if (path.has_parent_path()) {
        fs::create_directories(path.parent_path(), ec);
    }
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!out) {
        error = "cannot write " + path.string();
        return false;
    }
    return true;
}

std::optional<Bytes> readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return std::nullopt;
    }
    std::error_code ec;
    Bytes bytes(static_cast<std::size_t>(fs::file_size(path, ec)));
    in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return bytes;
}

// Every asset of a visitor into a pack writer.
AssetVisitor packVisitor(PackWriter& pack) {
    AssetVisitor v;
    v.spriteBank = [&](const std::string& n, const SpriteBank& a) { pack.addSpriteBank(n, a); };
    v.font = [&](const std::string& n, const Font& a) { pack.addFont(n, a); };
    v.soundBank = [&](const std::string& n, const SoundBank& a) { pack.addSoundBank(n, a); };
    v.music = [&](const std::string& n, const Bytes& a) { pack.addMusic(n, a); };
    v.musicList = [&](const std::string& n, const MusicList& a) { pack.addMusicList(n, a); };
    v.level = [&](const std::string& n, const LevelData& a) { pack.addLevel(n, a); };
    v.strings = [&](const std::string& n, const StringTable& a) { pack.addStrings(n, a); };
    v.campaign = [&](const std::string& n, const Campaign& a) { pack.addCampaign(n, a); };
    v.script = [&](const std::string& n, const std::string& a) { pack.addScript(n, a); };
    return v;
}

// The 2002 file of every asset, in memory, by file name.
bool legacyFiles(const std::string& sourceDir, std::map<std::string, Bytes>& files, std::string& error) {
    bool ok = true;
    const auto put = [&](const std::string& name, std::optional<Bytes> bytes) {
        if (!bytes) {
            ok = false;
            error = name + ": " + error;
            return;
        }
        files[name] = std::move(*bytes);
    };
    AssetVisitor v;
    v.spriteBank = [&](const std::string& n, const SpriteBank& a) { put(n, writeGfx(a)); };
    v.font = [&](const std::string& n, const Font& a) { put(n, writeLft(a, error)); };
    v.soundBank = [&](const std::string& n, const SoundBank& a) { put(n, writeSfx(a)); };
    v.music = [&](const std::string& n, const Bytes& a) { put(n, a); };
    v.musicList = [&](const std::string& n, const MusicList& a) { put(n, writeMbk(a)); };
    v.level = [&](const std::string& n, const LevelData& a) { put(n, writeLvl(a, error)); };
    v.strings = [&](const std::string& n, const StringTable& a) { put(n, writeDat(a, error)); };
    v.campaign = [&](const std::string& n, const Campaign& a) { put(n, writeLst(a)); };
    v.script = [&](const std::string& n, const std::string& a) {
        const auto encoded = utf8ToWindows1252(a);
        if (!encoded) {
            error = "has a character with no Windows-1252 byte";
        }
        put(n, encoded ? std::optional<Bytes>(Bytes(encoded->begin(), encoded->end())) : std::nullopt);
    };
    if (!bb::tools::readSourceTree(sourceDir, v, error)) {
        return false;
    }
    return ok;
}

int extract(const std::string& dataDir, const std::string& sourceDir) {
    bb::tools::SourceWriter out(sourceDir);
    std::string error;
    bool ok = true;
    const auto check = [&](const std::string& name, bool written) {
        if (!written && ok) {
            ok = false;
            error = name + ": " + error;
        }
    };
    AssetVisitor v;
    v.spriteBank = [&](const std::string& n, const SpriteBank& a) { check(n, out.spriteBank(n, a, error)); };
    v.font = [&](const std::string& n, const Font& a) { check(n, out.font(n, a, error)); };
    v.soundBank = [&](const std::string& n, const SoundBank& a) { check(n, out.soundBank(n, a, error)); };
    v.music = [&](const std::string& n, const Bytes& a) { check(n, out.music(n, a, error)); };
    v.musicList = [&](const std::string& n, const MusicList& a) { check(n, out.musicList(n, a, error)); };
    v.level = [&](const std::string& n, const LevelData& a) { check(n, out.level(n, a, error)); };
    v.strings = [&](const std::string& n, const StringTable& a) { check(n, out.strings(n, a, error)); };
    v.campaign = [&](const std::string& n, const Campaign& a) { check(n, out.campaign(n, a, error)); };
    v.script = [&](const std::string& n, const std::string& a) { check(n, out.script(n, a, error)); };
    std::vector<std::string> skipped;
    if (!bb::tools::readLegacyTree(dataDir, v, skipped, error) || !ok) {
        return fail(error);
    }
    for (const std::string& name : skipped) {
        std::printf("skipped %s (not a game asset)\n", name.c_str());
    }
    std::printf("wrote %zu files under %s\n", out.filesWritten(), sourceDir.c_str());
    return 0;
}

int pack(bool fromLegacy, const std::string& in, const std::string& packPath) {
    PackWriter writer;
    std::string error;
    std::vector<std::string> skipped;
    const AssetVisitor v = packVisitor(writer);
    const bool read = fromLegacy ? bb::tools::readLegacyTree(in, v, skipped, error)
                                 : bb::tools::readSourceTree(in, v, error);
    if (!read) {
        return fail(error);
    }
    const auto bytes = writer.finish(error);
    if (!bytes || !writeFile(packPath, *bytes, error)) {
        return fail(error);
    }
    std::printf("wrote %s (%zu bytes)\n", packPath.c_str(), bytes->size());
    return 0;
}

int legacy(const std::string& sourceDir, const std::string& outDir) {
    std::map<std::string, Bytes> files;
    std::string error;
    if (!legacyFiles(sourceDir, files, error)) {
        return fail(error);
    }
    for (const auto& [name, bytes] : files) {
        if (!writeFile(fs::path(outDir) / name, bytes, error)) {
            return fail(error);
        }
    }
    std::printf("wrote %zu files under %s\n", files.size(), outDir.c_str());
    return 0;
}

// Zeroes the bytes after the NUL of the seven 20-byte asset names of a .lvl
// file: the 2002 editor left stack garbage there, which nothing reads.
Bytes withoutNameTails(Bytes lvl) {
    for (std::size_t n = 0; n < 7 && lvl.size() >= 140; ++n) {
        bool afterNul = false;
        for (std::size_t i = n * 20; i < n * 20 + 20; ++i) {
            if (afterNul) {
                lvl[i] = 0;
            }
            afterNul = afterNul || lvl[i] == 0;
        }
    }
    return lvl;
}

int verify(const std::string& sourceDir, const std::string& dataDir) {
    std::map<std::string, Bytes> files;
    std::string error;
    if (!legacyFiles(sourceDir, files, error)) {
        return fail(error);
    }
    int identical = 0;
    int sameContent = 0;
    int different = 0;
    for (const auto& [name, rebuilt] : files) {
        const auto original = readFile(fs::path(dataDir) / name);
        if (!original) {
            std::printf("MISSING   %s (not in %s)\n", name.c_str(), dataDir.c_str());
            ++different;
            continue;
        }
        const std::string ext = bb::tools::extensionOf(name);
        if (rebuilt == *original) {
            ++identical;
            continue;
        }
        // Formats whose bytes the tools do not keep: the garbage after the
        // level asset names, and the layout of the small text files.
        bool same = false;
        std::string e1;
        std::string e2;
        if (ext == "lvl") {
            same = rebuilt == withoutNameTails(*original);
        } else if (ext == "lst") {
            const auto a = readLst(rebuilt, e1);
            const auto b = readLst(*original, e2);
            same = a && b && writeLst(*a) == writeLst(*b);
        } else if (ext == "mbk") {
            const auto a = readMbk(rebuilt, e1);
            const auto b = readMbk(*original, e2);
            same = a && b && writeMbk(*a) == writeMbk(*b);
        }
        if (same) {
            ++sameContent;
            std::printf("same content, other bytes: %s\n", name.c_str());
        } else {
            ++different;
            std::printf("DIFFERENT %s\n", name.c_str());
        }
    }
    std::printf("%d identical, %d with the same content, %d different\n", identical, sameContent, different);
    return different == 0 ? 0 : 1;
}

// Writes (or, with check, compares) the generated sprite-name headers.
// Generated headers of banks that no longer name images are removed.
int ids(const std::string& sourceDir, const std::string& includeDir, bool check) {
    std::map<std::string, std::string> headers;
    std::string error;
    if (!bb::tools::assetIdHeaders(sourceDir, headers, error)) {
        return fail(error);
    }
    int stale = 0;
    for (const auto& [relative, text] : headers) {
        const fs::path path = fs::path(includeDir) / relative;
        const auto current = readFile(path);
        if (current && std::string(current->begin(), current->end()) == text) {
            continue;
        }
        ++stale;
        if (check) {
            std::printf("out of date: %s\n", path.string().c_str());
        } else if (!writeFile(path, Bytes(text.begin(), text.end()), error)) {
            return fail(error);
        } else {
            std::printf("wrote %s\n", path.string().c_str());
        }
    }
    for (const char* kind : {"sprites", "sounds"}) {
        std::error_code ec;
        const fs::path generated = fs::path(includeDir) / "bb/assets" / kind;
        for (const auto& entry : fs::directory_iterator(generated, ec)) {
            const std::string relative =
                std::string("bb/assets/") + kind + "/" + entry.path().filename().string();
            if (headers.count(relative) == 0) {
                ++stale;
                if (check) {
                    std::printf("not generated any more: %s\n", entry.path().string().c_str());
                } else {
                    fs::remove(entry.path(), ec);
                    std::printf("removed %s\n", entry.path().string().c_str());
                }
            }
        }
    }
    if (check && stale != 0) {
        std::printf("run: bbassets ids %s %s\n", sourceDir.c_str(), includeDir.c_str());
        return 1;
    }
    return 0;
}

void usage() {
    std::fprintf(stderr,
                 "usage: bbassets extract <data-dir> <source-dir>\n"
                 "       bbassets pack <source-dir> <pack-file>\n"
                 "       bbassets pack-legacy <data-dir> <pack-file>\n"
                 "       bbassets legacy <source-dir> <out-dir>\n"
                 "       bbassets verify <source-dir> <data-dir>\n"
                 "       bbassets ids <source-dir> <include-dir>\n"
                 "       bbassets ids-check <source-dir> <include-dir>\n");
}

}  // namespace

static int run(int argc, char** argv) {
    const std::vector<std::string> args(argv + 1, argv + argc);
    if (args.size() != 3) {
        usage();
        return 2;
    }
    const std::string& command = args[0];
    if (command == "extract") {
        return extract(args[1], args[2]);
    }
    if (command == "pack") {
        return pack(false, args[1], args[2]);
    }
    if (command == "pack-legacy") {
        return pack(true, args[1], args[2]);
    }
    if (command == "legacy") {
        return legacy(args[1], args[2]);
    }
    if (command == "verify") {
        return verify(args[1], args[2]);
    }
    if (command == "ids" || command == "ids-check") {
        return ids(args[1], args[2], command == "ids-check");
    }
    usage();
    return 2;
}

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "bbassets: %s\n", e.what());
        return 1;
    }
}
