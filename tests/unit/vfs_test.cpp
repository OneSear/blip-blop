#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "bb/core/vfs.hpp"

namespace fs = std::filesystem;

namespace {

struct TempTree {
    fs::path root;

    TempTree() {
        root = fs::temp_directory_path() / ("bb_vfs_test_" + std::to_string(reinterpret_cast<std::uintptr_t>(this)));
        fs::remove_all(root);
        fs::create_directories(root / "Sub");
        write("snork.mbk", "1\n1 data/snork.zik\n");
        write("Sub/Level.LVL", "level");
    }
    ~TempTree() { fs::remove_all(root); }

    void write(const std::string& name, const std::string& text) const {
        std::ofstream(root / name, std::ios::binary) << text;
    }
};

}  // namespace

TEST_CASE("splitLegacyPath accepts both separators and drops the data prefix") {
    using V = std::vector<std::string>;
    CHECK(bb::core::splitLegacyPath("data\\bisou.zik") == V{"bisou.zik"});
    CHECK(bb::core::splitLegacyPath("data/Snork.mbk") == V{"Snork.mbk"});
    CHECK(bb::core::splitLegacyPath("./data//x/./y") == V{"x", "y"});
    CHECK(bb::core::splitLegacyPath("DATA/bb.lst") == V{"bb.lst"});
    CHECK(bb::core::splitLegacyPath("bb.lst") == V{"bb.lst"});
}

TEST_CASE("Vfs resolves legacy names on a case-sensitive or insensitive file system") {
    TempTree tree;
    bb::core::Vfs vfs(tree.root.string());

    const auto exact = vfs.resolve("data/snork.mbk");
    REQUIRE(exact.has_value());
    CHECK(fs::equivalent(*exact, tree.root / "snork.mbk"));

    CHECK(vfs.resolve("data\\Snork.mbk").has_value());
    CHECK(vfs.resolve("data/sub/level.lvl").has_value());
    CHECK_FALSE(vfs.resolve("data/missing.gfx").has_value());
    CHECK_FALSE(vfs.resolve("data/Sub").has_value());  // directories are not files

    const auto bytes = vfs.readAll("data/SUB/level.lvl");
    REQUIRE(bytes.has_value());
    CHECK(std::string(bytes->begin(), bytes->end()) == "level");

    CHECK(fs::path(vfs.writablePath("data/bb.cfg")) == tree.root / "bb.cfg");
    // Same file whatever case the file system reports.
    CHECK(fs::equivalent(vfs.writablePath("data/SNORK.MBK"), tree.root / "snork.mbk"));
}
