#include "bb/core/assets/text_encoding.hpp"

namespace bb::core::assets {

namespace {

// Windows-1252 bytes 0x80..0x9F (WHATWG index; undefined bytes map to the
// C1 control of the same value).
constexpr std::uint16_t kHighTable[32] = {
    0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,  //
    0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F,  //
    0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,  //
    0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178,
};

}  // namespace

std::uint32_t windows1252ToCodePoint(std::uint8_t byte) noexcept {
    if (byte >= 0x80 && byte < 0xA0) {
        return kHighTable[byte - 0x80];
    }
    return byte;
}

std::optional<std::uint8_t> codePointToWindows1252(std::uint32_t codePoint) noexcept {
    if (codePoint < 0x80 || (codePoint >= 0xA0 && codePoint <= 0xFF)) {
        return static_cast<std::uint8_t>(codePoint);
    }
    for (std::uint8_t i = 0; i < 32; ++i) {
        if (kHighTable[i] == codePoint) {
            return static_cast<std::uint8_t>(0x80 + i);
        }
    }
    return std::nullopt;
}

void appendUtf8(std::string& out, std::uint32_t cp) {
    if (cp < 0x80) {
        out.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

std::string windows1252ToUtf8(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (const char c : text) {
        appendUtf8(out, windows1252ToCodePoint(static_cast<std::uint8_t>(c)));
    }
    return out;
}

namespace {

// Decodes one well-formed UTF-8 sequence; false when there is none at pos.
bool decodeUtf8(std::string_view text, std::size_t pos, std::uint32_t& cp, std::size_t& length) noexcept {
    const auto byte = [&](std::size_t i) { return static_cast<std::uint8_t>(text[i]); };
    const std::uint8_t b0 = byte(pos);
    if (b0 < 0x80) {
        cp = b0;
        length = 1;
        return true;
    }
    std::uint32_t min = 0;
    if ((b0 & 0xE0) == 0xC0) {
        length = 2;
        cp = b0 & 0x1Fu;
        min = 0x80;
    } else if ((b0 & 0xF0) == 0xE0) {
        length = 3;
        cp = b0 & 0x0Fu;
        min = 0x800;
    } else if ((b0 & 0xF8) == 0xF0) {
        length = 4;
        cp = b0 & 0x07u;
        min = 0x10000;
    } else {
        return false;
    }
    if (pos + length > text.size()) {
        return false;
    }
    for (std::size_t i = 1; i < length; ++i) {
        const std::uint8_t b = byte(pos + i);
        if ((b & 0xC0) != 0x80) {
            return false;
        }
        cp = (cp << 6) | (b & 0x3Fu);
    }
    return cp >= min && cp <= 0x10FFFF && (cp < 0xD800 || cp > 0xDFFF);
}

}  // namespace

std::uint32_t nextCodePoint(std::string_view text, std::size_t& pos) noexcept {
    std::uint32_t cp = 0;
    std::size_t length = 0;
    if (decodeUtf8(text, pos, cp, length)) {
        pos += length;
        return cp;
    }
    return static_cast<std::uint8_t>(text[pos++]);
}

bool isValidUtf8(std::string_view text) noexcept {
    std::size_t pos = 0;
    while (pos < text.size()) {
        std::uint32_t cp = 0;
        std::size_t length = 0;
        if (!decodeUtf8(text, pos, cp, length)) {
            return false;
        }
        pos += length;
    }
    return true;
}

std::optional<std::string> utf8ToWindows1252(std::string_view text) {
    if (!isValidUtf8(text)) {
        return std::nullopt;
    }
    std::string out;
    std::size_t pos = 0;
    while (pos < text.size()) {
        const auto byte = codePointToWindows1252(nextCodePoint(text, pos));
        if (!byte) {
            return std::nullopt;
        }
        out.push_back(static_cast<char>(*byte));
    }
    return out;
}

}  // namespace bb::core::assets
