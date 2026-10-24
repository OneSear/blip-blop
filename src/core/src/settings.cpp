#include "bb/core/settings.hpp"

#include <sstream>

namespace bb::core {

using input::Action;
using input::kActions;
using input::kPlayerCount;

namespace {

std::vector<std::string_view> words(std::string_view line) {
    std::vector<std::string_view> out;
    std::size_t i = 0;
    while (i < line.size()) {
        while (i < line.size() && (line[i] == ' ' || line[i] == '\t' || line[i] == '\r')) {
            ++i;
        }
        const std::size_t start = i;
        while (i < line.size() && line[i] != ' ' && line[i] != '\t' && line[i] != '\r') {
            ++i;
        }
        if (i > start) {
            out.push_back(line.substr(start, i - start));
        }
    }
    return out;
}

std::int32_t readInt32(const std::vector<std::uint8_t>& b, std::size_t offset) {
    const std::uint32_t v =
        static_cast<std::uint32_t>(b[offset]) | (static_cast<std::uint32_t>(b[offset + 1]) << 8) |
        (static_cast<std::uint32_t>(b[offset + 2]) << 16) | (static_cast<std::uint32_t>(b[offset + 3]) << 24);
    return static_cast<std::int32_t>(v);
}

}  // namespace

std::string Settings::serialize() const {
    std::ostringstream out;
    out << "blipblop-settings 1\n";
    out << "vsync " << (vsync ? "on" : "off") << "\n";
    out << "fullscreen " << (fullscreen ? "on" : "off") << "\n";
    out << "language " << (language == input::Language::French ? "french" : "english") << "\n";
    for (int p = 0; p < kPlayerCount; ++p) {
        for (const Action a : kActions) {
            out << 'p' << (p + 1) << '.' << input::actionName(a) << ' ' << input::codeName(bindings.get(p, a))
                << "\n";
        }
    }
    return out.str();
}

std::optional<Settings> Settings::parse(std::string_view text, std::string& error,
                                        std::vector<std::string>& warnings) {
    Settings s;
    bool header = false;
    std::size_t lineNumber = 0;
    std::size_t pos = 0;
    while (pos <= text.size()) {
        std::size_t eol = text.find('\n', pos);
        if (eol == std::string_view::npos) {
            eol = text.size();
        }
        std::string_view line = text.substr(pos, eol - pos);
        pos = eol + 1;
        ++lineNumber;
        if (const std::size_t hash = line.find('#'); hash != std::string_view::npos) {
            line = line.substr(0, hash);
        }
        const auto w = words(line);
        if (w.empty()) {
            continue;
        }
        const std::string where = "line " + std::to_string(lineNumber) + ": ";
        if (!header) {
            if (w.size() != 2 || w[0] != "blipblop-settings" || w[1] != "1") {
                error = where + "expected 'blipblop-settings 1'";
                return std::nullopt;
            }
            header = true;
            continue;
        }
        if (w.size() != 2) {
            warnings.push_back(where + "expected a name and a value");
            continue;
        }
        if (w[0] == "vsync") {
            if (w[1] != "on" && w[1] != "off") {
                error = where + "vsync must be on or off";
                return std::nullopt;
            }
            s.vsync = w[1] == "on";
            continue;
        }
        if (w[0] == "fullscreen") {
            if (w[1] != "on" && w[1] != "off") {
                error = where + "fullscreen must be on or off";
                return std::nullopt;
            }
            s.fullscreen = w[1] == "on";
            continue;
        }
        if (w[0] == "language") {
            if (w[1] == "english") {
                s.language = input::Language::English;
            } else if (w[1] == "french") {
                s.language = input::Language::French;
            } else {
                error = where + "language must be english or french";
                return std::nullopt;
            }
            continue;
        }
        bool bound = false;
        for (int p = 0; p < kPlayerCount && !bound; ++p) {
            for (const Action a : kActions) {
                const std::string key = "p" + std::to_string(p + 1) + "." + std::string(input::actionName(a));
                if (w[0] == key) {
                    const auto code = input::codeFromName(w[1]);
                    if (!code) {
                        error = where + "unknown input '" + std::string(w[1]) + "'";
                        return std::nullopt;
                    }
                    s.bindings.set(p, a, *code);
                    bound = true;
                    break;
                }
            }
        }
        if (!bound) {
            warnings.push_back(where + "unknown setting '" + std::string(w[0]) + "'");
        }
    }
    if (!header) {
        error = "empty file: expected 'blipblop-settings 1'";
        return std::nullopt;
    }
    return s;
}

std::optional<Settings> Settings::importLegacyCfg(const std::vector<std::uint8_t>& bytes,
                                                  std::vector<std::string>& notes) {
    if (bytes.size() < 1 + 4 + 14 * 4) {
        notes.push_back("bb.cfg is " + std::to_string(bytes.size()) + " bytes, 61 expected");
        return std::nullopt;
    }
    Settings s;
    s.vsync = bytes[0] != 0;
    s.language = input::Language::English;
    std::size_t offset = 5;
    for (int p = 0; p < kPlayerCount; ++p) {
        input::Bindings imported = s.bindings;
        bool valid = true;
        for (const Action a : kActions) {
            const auto code = static_cast<input::Code>(readInt32(bytes, offset));
            offset += 4;
            if (!input::isKnownCode(code)) {
                valid = false;
            }
            imported.set(p, a, code);
        }
        if (valid) {
            for (const Action a : kActions) {
                s.bindings.set(p, a, imported.get(p, a));
            }
        } else {
            notes.push_back(
                "player " + std::to_string(p + 1) +
                ": bb.cfg holds codes that are not SDL keys (DirectInput scan codes?), defaults used");
        }
    }
    return s;
}

}  // namespace bb::core
