#include "bb/core/vfs.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <system_error>

namespace bb::core {

namespace fs = std::filesystem;

namespace {

bool equalsIgnoreCase(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

}  // namespace

std::vector<std::string> splitLegacyPath(std::string_view path) {
    std::vector<std::string> parts;
    std::string current;
    for (const char c : path) {
        if (c == '/' || c == '\\') {
            if (!current.empty() && current != ".") {
                parts.push_back(current);
            }
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty() && current != ".") {
        parts.push_back(current);
    }
    if (!parts.empty() && equalsIgnoreCase(parts.front(), "data")) {
        parts.erase(parts.begin());
    }
    return parts;
}

Vfs::Vfs(std::string dataRoot) : root_(std::move(dataRoot)) {
    if (root_.empty()) {
        root_ = ".";
    }
}

std::optional<std::string> Vfs::resolve(std::string_view path) const {
    const auto parts = splitLegacyPath(path);
    if (parts.empty()) {
        return std::nullopt;
    }
    fs::path current(root_);
    std::error_code ec;
    for (const auto& part : parts) {
        const fs::path exact = current / part;
        if (fs::exists(exact, ec)) {
            current = exact;
            continue;
        }
        // Case-insensitive match among the directory entries.
        bool found = false;
        for (fs::directory_iterator it(current, ec), end; !ec && it != end; it.increment(ec)) {
            if (equalsIgnoreCase(it->path().filename().string(), part)) {
                current = it->path();
                found = true;
                break;
            }
        }
        if (!found) {
            return std::nullopt;
        }
    }
    if (!fs::is_regular_file(current, ec)) {
        return std::nullopt;
    }
    return current.string();
}

std::string Vfs::writablePath(std::string_view path) const {
    if (auto existing = resolve(path)) {
        return *existing;
    }
    fs::path p(root_);
    for (const auto& part : splitLegacyPath(path)) {
        p /= part;
    }
    return p.string();
}

std::optional<std::vector<std::uint8_t>> Vfs::readAll(std::string_view path) const {
    const auto host = resolve(path);
    if (!host) {
        return std::nullopt;
    }
    std::ifstream in(*host, std::ios::binary);
    if (!in) {
        return std::nullopt;
    }
    in.seekg(0, std::ios::end);
    const std::streamoff size = in.tellg();
    if (size < 0) {
        return std::nullopt;
    }
    in.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> data(static_cast<std::size_t>(size));
    if (size > 0 && !in.read(reinterpret_cast<char*>(data.data()), size)) {
        return std::nullopt;
    }
    return data;
}

}  // namespace bb::core
