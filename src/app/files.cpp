// The data files and the asset source (legacy_io.cpp of Phases 1 and 2).

#include "files.hpp"

#include <filesystem>
#include <system_error>

#include "bb/core/assets/asset_source.hpp"
#include "bb/core/assets/pack.hpp"
#include "bb/core/log.hpp"
#include "bb/core/vfs.hpp"

namespace bb::app {

namespace {

// The file every run reads first (App::init()).
constexpr const char* kProbeFile = "data/uk.dat";

}  // namespace

GameFiles::GameFiles() : vfs_(std::make_unique<core::Vfs>("data")) {}

GameFiles::~GameFiles() {
    // The default asset source reads through the files: it goes first.
    assetSource_.reset();
}

void GameFiles::setDataRoot(const std::string& dataRoot) {
    // The default asset source reads through the files: drop it first.
    assetSource_.reset();
    vfs_ = std::make_unique<core::Vfs>(dataRoot);
    chosen_ = true;
}

const core::Vfs& GameFiles::dataFiles() const {
    return *vfs_;
}

std::string GameFiles::writablePath(const std::string& path) const {
    return vfs_->writablePath(path);
}

void GameFiles::locateGameData(const std::string& baseDirectory) {
    if (chosen_ || baseDirectory.empty() || vfs_->resolve(kProbeFile)) {
        return;
    }
    const std::string packPath = baseDirectory + "blipblop.bbpack";
    std::error_code ec;
    if (std::filesystem::is_regular_file(packPath, ec)) {
        std::string error;
        std::shared_ptr<const core::assets::PackFile> pack = core::assets::PackFile::open(packPath, error);
        if (pack) {
            core::Log::info("Game data: " + packPath);
            setAssetSource(std::make_unique<core::assets::PackSource>(pack));
            return;
        }
        core::Log::warning(error);
    }
    const std::string dataRoot = baseDirectory + "data";
    if (core::Vfs(dataRoot).resolve(kProbeFile)) {
        core::Log::info("Game data: " + dataRoot);
        setDataRoot(dataRoot);
    }
}

void GameFiles::setAssetSource(std::unique_ptr<core::assets::AssetSource> source) {
    assetSource_ = std::move(source);
    chosen_ = true;
}

const core::assets::AssetSource& GameFiles::assets() {
    if (!assetSource_) {
        assetSource_ = std::make_unique<core::assets::LegacyDataSource>(*vfs_);
    }
    return *assetSource_;
}

}  // namespace bb::app
