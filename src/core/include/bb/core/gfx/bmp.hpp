#pragma once

#include <string>

#include "bb/core/gfx/pixmap.hpp"

namespace bb::core::gfx {

// Writes the pixmap as an uncompressed 24-bit BMP (alpha dropped).
// Returns false when the file cannot be written.
bool writeBmp(const Pixmap& pixmap, const std::string& path);

}  // namespace bb::core::gfx
