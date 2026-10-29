// The legacy readers on damaged and random input (migration.md section 6):
// they may accept or reject it, but must never crash or read out of bounds
// (the sanitizer builds catch the latter). The damage is seeded, so a
// failure is reproducible.

#include <doctest.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <string>
#include <vector>

#include "bb/core/assets/legacy_formats.hpp"
#include "bb/core/rng.hpp"

namespace fs = std::filesystem;
using namespace bb::core::assets;

namespace {

const fs::path kData = BB_TEST_DATA_DIR;

Bytes readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    Bytes bytes(static_cast<std::size_t>(fs::file_size(path)));
    in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return bytes;
}

// A random number in [0, n) from the game's generator (n > 0).
std::size_t below(bb::core::Rng& rng, std::size_t n) {
    const auto high = static_cast<std::size_t>(rng.next()) << 15;
    return (high | static_cast<std::size_t>(rng.next())) % n;
}

// One damaged copy of a file: flipped bytes, a size field set to an extreme
// value, a cut, or inserted bytes.
Bytes damage(const Bytes& original, bb::core::Rng& rng) {
    Bytes b = original;
    if (b.empty()) {
        return b;
    }
    switch (below(rng, 4)) {
        case 0: {
            const std::size_t flips = 1 + below(rng, 8);
            for (std::size_t i = 0; i < flips; ++i) {
                b[below(rng, b.size())] ^= static_cast<std::uint8_t>(1 + below(rng, 255));
            }
            break;
        }
        case 1: {
            static const std::uint32_t kExtremes[] = {0,           1,       0x7FFFFFFFu, 0x80000000u,
                                                      0xFFFFFFFFu, 0xFFFFu, 0x10000u};
            const std::uint32_t value = kExtremes[below(rng, std::size(kExtremes))];
            // Header fields are where the sizes are: aim at the start more often.
            const std::size_t limit = below(rng, 2) == 0 ? std::min<std::size_t>(b.size(), 256) : b.size();
            const std::size_t at = below(rng, limit);
            for (std::size_t i = 0; i < 4 && at + i < b.size(); ++i) {
                b[at + i] = static_cast<std::uint8_t>(value >> (8 * i));
            }
            break;
        }
        case 2:
            b.resize(below(rng, b.size()));
            break;
        default: {
            const std::size_t at = below(rng, b.size());
            Bytes extra(1 + below(rng, 64));
            for (auto& byte : extra) {
                byte = static_cast<std::uint8_t>(below(rng, 256));
            }
            b.insert(b.begin() + static_cast<std::ptrdiff_t>(at), extra.begin(), extra.end());
            break;
        }
    }
    return b;
}

Bytes garbage(bb::core::Rng& rng) {
    Bytes b(below(rng, 2048));
    for (auto& byte : b) {
        byte = static_cast<std::uint8_t>(below(rng, 256));
    }
    return b;
}

// Runs a reader on damaged copies of a shipped file and on random bytes;
// returns how many inputs it accepted (to show the damage is not always
// fatal, so the reader's deeper paths run too).
int fuzz(const char* name, const std::function<bool(const Bytes&)>& read, int rounds = 300) {
    const Bytes original = readFile(kData / name);
    bb::core::Rng rng;
    // The same seed on every platform: a sum over the name's bytes.
    std::uint32_t seed = 1;
    for (const char* c = name; *c != '\0'; ++c) {
        seed = seed * 31 + static_cast<unsigned char>(*c);
    }
    rng.seed(seed);
    int accepted = 0;
    for (int i = 0; i < rounds; ++i) {
        accepted += read(damage(original, rng)) ? 1 : 0;
        accepted += read(garbage(rng)) ? 1 : 0;
    }
    return accepted;
}

std::string text(const Bytes& b) {
    return std::string(b.begin(), b.end());
}

}  // namespace

TEST_CASE("the readers survive damaged and random files") {
    std::string error;
    const auto check = [&](const char* name, const std::function<bool(const Bytes&)>& read) {
        CAPTURE(name);
        const int accepted = fuzz(name, read);
        MESSAGE(std::string(name) << ": " << accepted << " of 600 inputs accepted");
    };
    check("misc.gfx", [&](const Bytes& b) { return readGfx(b, error).has_value(); });
    check("ammo1.lft", [&](const Bytes& b) { return readLft(b, error).has_value(); });
    check("bb.sfx", [&](const Bytes& b) { return readSfx(b, error).has_value(); });
    check("snuf1.lvl", [&](const Bytes& b) { return readLvl(b, error).has_value(); });
    check("uk.dat", [&](const Bytes& b) { return readDat(b, error).has_value(); });
    check("bb.lst", [&](const Bytes& b) { return readLst(b, error).has_value(); });
    check("inter.mbk", [&](const Bytes& b) { return readMbk(b, error).has_value(); });
    check("bb.scr", [&](const Bytes& b) { return readScr(b, error).has_value(); });
    check("intro.cin", [&](const Bytes& b) { return parseCutscene(text(b), error).has_value(); });
}
