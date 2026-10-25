#pragma once

#include <cstddef>
#include <string>

#include "bb/core/file_reader.hpp"

namespace bb::platform {

// The files of the game and of the user (migration.md 3.5): reading by
// path (core::FileReader, what the data readers use), writing whole files,
// and the two directories the game knows. Paths are UTF-8.
class FileSystem : public core::FileReader {
public:
    // Replaces the file's contents with `size` bytes (creating the file);
    // false when they could not all be written.
    virtual bool writeAll(const std::string& path, const void* data, std::size_t size) = 0;

    // Directory of the executable, with a trailing separator ("" if unknown).
    virtual std::string baseDirectory() const = 0;

    // Per-user writable directory for settings and scores, created if
    // needed, with a trailing separator ("" if unavailable).
    virtual std::string preferencesDirectory() const = 0;
};

}  // namespace bb::platform
