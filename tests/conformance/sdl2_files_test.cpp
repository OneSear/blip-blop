// The SDL 2 file system: what the game reads and writes through it, with
// UTF-8 names (wide paths on Windows).

#include <doctest.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "bb/core/vfs.hpp"
#include "bb/platform/file_system.hpp"
#include "bb/platform/sdl2/sdl2_platform.hpp"

namespace fs = std::filesystem;

namespace {

struct TempDir {
    fs::path root;

    TempDir() {
        root = fs::temp_directory_path() /
               ("bb_sdl2_files_test_" + std::to_string(reinterpret_cast<std::uintptr_t>(this)));
        fs::remove_all(root);
        fs::create_directories(root / "Sub");
    }
    ~TempDir() { fs::remove_all(root); }

    std::string path(const std::string& name) const { return (root / fs::u8path(name)).u8string(); }
};

}  // namespace

TEST_CASE("the SDL 2 file system writes, lists and reads files with UTF-8 names") {
    bb::platform::sdl2::Options options;
    options.headless = true;
    std::string error;
    auto platform = bb::platform::sdl2::createPlatform(options, error);
    REQUIRE_MESSAGE(platform != nullptr, error);
    bb::platform::FileSystem& files = platform->files();
    using Kind = bb::core::FileReader::Kind;

    TempDir dir;
    const std::string name = u8"écran 1.bin";  // écran 1.bin
    std::vector<std::uint8_t> bytes(256);
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        bytes[i] = static_cast<std::uint8_t>(i);
    }
    REQUIRE(files.writeAll(dir.path(name), bytes.data(), bytes.size()));
    CHECK_FALSE(files.writeAll(dir.path("Missing/x.bin"), bytes.data(), bytes.size()));

    CHECK(files.kind(dir.path(name)) == Kind::File);
    CHECK(files.kind(dir.path("Sub")) == Kind::Directory);
    CHECK(files.kind(dir.path("nothing")) == Kind::Missing);
    const std::vector<std::string> names = files.list(dir.root.u8string());
    CHECK(names.size() == 2);
    CHECK(std::find(names.begin(), names.end(), name) != names.end());

    auto file = files.open(dir.path(name));
    REQUIRE(file != nullptr);
    CHECK(file->size() == 256);
    std::uint8_t part[4] = {};
    REQUIRE(file->readAt(100, part, 4));
    CHECK(part[0] == 100);
    CHECK(part[3] == 103);
    CHECK_FALSE(file->readAt(254, part, 4));
    CHECK(files.open(dir.path("nothing")) == nullptr);

    const auto all = files.readAll(dir.path(name));
    REQUIRE(all.has_value());
    CHECK(*all == bytes);

    // The game's data paths, through the platform's files: case-insensitive
    // on any file system.
    const bb::core::Vfs vfs(dir.root.u8string(), files);
    CHECK(vfs.readAll(u8"data/éCRAN 1.BIN") == bytes);

    // Rewriting replaces the contents, once the file is closed (Windows
    // refuses to write a file open for reading).
    file.reset();
    REQUIRE(files.writeAll(dir.path(name), "ab", 2));
    CHECK(files.readAll(dir.path(name)) == std::vector<std::uint8_t>{'a', 'b'});

    const std::string base = files.baseDirectory();
    REQUIRE_FALSE(base.empty());
    CHECK((base.back() == '/' || base.back() == '\\'));
}
