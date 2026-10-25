#include "bb/core/file_reader.hpp"

#include <filesystem>
#include <fstream>
#include <system_error>
#include <utility>

namespace bb::core {

namespace fs = std::filesystem;

std::optional<std::vector<std::uint8_t>> FileReader::readAll(const std::string& path) const {
    const std::unique_ptr<ReadableFile> file = open(path);
    if (!file) {
        return std::nullopt;
    }
    std::vector<std::uint8_t> data(static_cast<std::size_t>(file->size()));
    if (!file->readAt(0, data.data(), data.size())) {
        return std::nullopt;
    }
    return data;
}

namespace {

// Paths are UTF-8 on every system (u8path: wide on Windows).
class HostFile final : public ReadableFile {
public:
    HostFile(std::ifstream in, std::uint64_t size) : in_(std::move(in)), size_(size) {}

    std::uint64_t size() const override { return size_; }

    bool readAt(std::uint64_t offset, void* out, std::size_t count) override {
        if (count == 0) {
            return true;
        }
        if (offset > size_) {
            return false;
        }
        in_.clear();
        in_.seekg(static_cast<std::streamoff>(offset));
        in_.read(static_cast<char*>(out), static_cast<std::streamsize>(count));
        return static_cast<std::size_t>(in_.gcount()) == count;
    }

private:
    std::ifstream in_;
    std::uint64_t size_;
};

class HostFiles final : public FileReader {
public:
    std::unique_ptr<ReadableFile> open(const std::string& path) const override {
        std::ifstream in(fs::u8path(path), std::ios::binary);
        if (!in) {
            return nullptr;
        }
        in.seekg(0, std::ios::end);
        const std::streamoff end = in.tellg();
        if (end < 0) {
            return nullptr;
        }
        return std::make_unique<HostFile>(std::move(in), static_cast<std::uint64_t>(end));
    }

    Kind kind(const std::string& path) const override {
        std::error_code ec;
        const fs::file_status status = fs::status(fs::u8path(path), ec);
        if (fs::is_regular_file(status)) {
            return Kind::File;
        }
        if (fs::is_directory(status)) {
            return Kind::Directory;
        }
        return Kind::Missing;  // nor the other kinds (devices, sockets)
    }

    std::vector<std::string> list(const std::string& directory) const override {
        std::vector<std::string> names;
        std::error_code ec;
        for (fs::directory_iterator it(fs::u8path(directory), ec), end; !ec && it != end; it.increment(ec)) {
            names.push_back(it->path().filename().u8string());
        }
        return names;
    }
};

}  // namespace

const FileReader& hostFiles() {
    static const HostFiles files;
    return files;
}

}  // namespace bb::core
