#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace bb::core::assets {

// The 2002 text files and fonts use single-byte Windows-1252. These map
// its bytes to Unicode and back. The five bytes Windows-1252 leaves
// undefined (0x81, 0x8D, 0x8F, 0x90, 0x9D) map to the C1 control with the
// same value, as in the WHATWG encoding standard, so every byte has a code
// point and every mapping is reversible.
std::uint32_t windows1252ToCodePoint(std::uint8_t byte) noexcept;
std::optional<std::uint8_t> codePointToWindows1252(std::uint32_t codePoint) noexcept;

std::string windows1252ToUtf8(std::string_view text);
// nullopt when the text is not valid UTF-8 or has a character with no
// Windows-1252 byte.
std::optional<std::string> utf8ToWindows1252(std::string_view text);

void appendUtf8(std::string& out, std::uint32_t codePoint);

// Decodes the code point at pos and advances pos. An invalid or truncated
// sequence decodes as its first byte's value (so Windows-1252 text read
// as UTF-8 degrades to Latin-1) and advances by one byte.
std::uint32_t nextCodePoint(std::string_view text, std::size_t& pos) noexcept;

bool isValidUtf8(std::string_view text) noexcept;

}  // namespace bb::core::assets
