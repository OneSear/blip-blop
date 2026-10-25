#pragma once

#include <memory>
#include <string>

namespace bb::core {
class FileReader;
class Vfs;
namespace assets {
class AssetSource;
class PackFile;
}
}  // namespace bb::core

namespace bb::app {

// The game's data directory ("data" in the working directory by default,
// --data), where its user files are written, and the source of the game
// assets (owned by bbMain()).
class GameFiles {
public:
    // The files are read through `reader`, which must outlive this object.
    explicit GameFiles(const core::FileReader& reader);
    ~GameFiles();
    GameFiles(const GameFiles&) = delete;
    GameFiles& operator=(const GameFiles&) = delete;

    void setDataRoot(const std::string& dataRoot);
    const core::Vfs& dataFiles() const;
    // Host path where a data file (settings import, scores) is written.
    std::string writablePath(const std::string& path) const;

    // Without --data or --pack: the data directory of the working directory
    // (the build tree, and the 2002 layout) when it has the game's files,
    // else the runtime pack next to the executable (the packages; in a
    // macOS bundle the executable's directory is Resources), else a data
    // directory next to the executable. baseDirectory ends with a separator
    // ("" if unknown).
    void locateGameData(const std::string& baseDirectory);

    // The source of the game assets: the 2002 files of the data directory,
    // unless another source was set (--pack).
    void setAssetSource(std::unique_ptr<core::assets::AssetSource> source);
    // Assets of this pack replace those of the source (--overlay).
    void setOverlay(std::shared_ptr<const core::assets::PackFile> overlay);
    const core::assets::AssetSource& assets();

private:
    const core::FileReader& reader_;
    std::unique_ptr<core::Vfs> vfs_;
    std::unique_ptr<core::assets::AssetSource> assetSource_;
    std::shared_ptr<const core::assets::PackFile> overlay_;
    std::unique_ptr<core::assets::AssetSource> overlaySource_;  // over assetSource_
    bool chosen_ = false;  // --data or --pack was given
};

}  // namespace bb::app
