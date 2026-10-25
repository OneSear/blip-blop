#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace bb::core {

// A file open for reading.
class ReadableFile {
public:
    virtual ~ReadableFile() = default;

    virtual std::uint64_t size() const = 0;
    // Reads `count` bytes at `offset`; false when they are not all there.
    virtual bool readAt(std::uint64_t offset, void* out, std::size_t count) = 0;
};

// Read access to the host's files by path (UTF-8): what the data readers
// need (Vfs, PackFile). The platform's FileSystem provides it to the game
// (migration.md 3.5); hostFiles() is the C++ standard library's, for the
// tools and the tests.
class FileReader {
public:
    enum class Kind { Missing, File, Directory };

    virtual ~FileReader() = default;

    // Null when the file cannot be opened.
    virtual std::unique_ptr<ReadableFile> open(const std::string& path) const = 0;
    virtual Kind kind(const std::string& path) const = 0;
    // The names of a directory's entries (none when it cannot be read).
    virtual std::vector<std::string> list(const std::string& directory) const = 0;

    // The whole file, or nullopt when it cannot be read.
    std::optional<std::vector<std::uint8_t>> readAll(const std::string& path) const;
};

// The C++ standard library's file access.
const FileReader& hostFiles();

}  // namespace bb::core
