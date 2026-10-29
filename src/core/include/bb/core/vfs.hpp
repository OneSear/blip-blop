#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "bb/core/file_reader.hpp"

namespace bb::core {

// Read-only view of the game data, rooted at the data directory.
//
// The legacy code names files "data/xxx" (relative to the directory that
// contains data/), with '\' separators in places and the wrong case in
// others (migration.md D-10, D-46). resolve() accepts those names and finds
// the file on any file system:
//   * '\' and '/' are both separators;
//   * a leading "data/" component is relative to the data root;
//   * when the exact name does not exist, each path component is matched
//     without regard to case.
// Paths that do not start with "data/" are resolved relative to the data
// root as well. The files are read through `files`, which must outlive the
// Vfs.
class Vfs {
public:
    explicit Vfs(std::string dataRoot, const FileReader& files = hostFiles());

    const std::string& dataRoot() const noexcept { return root_; }

    // Host path of an existing file, or nullopt.
    std::optional<std::string> resolve(std::string_view path) const;

    // Host path where a file should be written (it may not exist yet).
    std::string writablePath(std::string_view path) const;

    // Whole file contents, or nullopt when it cannot be read.
    std::optional<std::vector<std::uint8_t>> readAll(std::string_view path) const;

private:
    std::string root_;
    const FileReader* files_;
};

// Splits a legacy path into components, accepting both separators and
// dropping a leading "data" component and "." components.
std::vector<std::string> splitLegacyPath(std::string_view path);

}  // namespace bb::core
