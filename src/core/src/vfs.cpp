#include "bb/core/vfs.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>

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

Vfs::Vfs(std::string dataRoot, const FileReader& files) : root_(std::move(dataRoot)), files_(&files) {
    if (root_.empty()) {
        root_ = ".";
    }
}

std::optional<std::string> Vfs::resolve(std::string_view path) const {
    const auto parts = splitLegacyPath(path);
    if (parts.empty()) {
        return std::nullopt;
    }
    // Paths are UTF-8 (u8path, u8string: the same bytes on macOS and Linux).
    fs::path current = fs::u8path(root_);
    for (const auto& part : parts) {
        const fs::path exact = current / fs::u8path(part);
        if (files_->kind(exact.u8string()) != FileReader::Kind::Missing) {
            current = exact;
            continue;
        }
        // Case-insensitive match among the directory entries.
        bool found = false;
        for (const std::string& name : files_->list(current.u8string())) {
            if (equalsIgnoreCase(name, part)) {
                current /= fs::u8path(name);
                found = true;
                break;
            }
        }
        if (!found) {
            return std::nullopt;
        }
    }
    if (files_->kind(current.u8string()) != FileReader::Kind::File) {
        return std::nullopt;
    }
    return current.u8string();
}

std::string Vfs::writablePath(std::string_view path) const {
    if (auto existing = resolve(path)) {
        return *existing;
    }
    fs::path p = fs::u8path(root_);
    for (const auto& part : splitLegacyPath(path)) {
        p /= fs::u8path(part);
    }
    return p.u8string();
}

std::optional<std::vector<std::uint8_t>> Vfs::readAll(std::string_view path) const {
    const auto host = resolve(path);
    if (!host) {
        return std::nullopt;
    }
    return files_->readAll(*host);
}

}  // namespace bb::core
