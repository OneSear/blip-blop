#pragma once

#include <optional>
#include <string>

#include "bb/core/assets/image16.hpp"

namespace bb::tools {

// 16-bit art as RGBA PNG: colours are the game's 8-bit expansion of the
// 16-bit value, and pixels of the layout's key colour get alpha 0.
bool writePng(const std::string& path, const core::assets::Image16& image, std::string& error);

// Reads a PNG written by writePng (or edited since) back into the given
// layout: alpha 0 is the key colour, alpha 255 an opaque colour rounded to
// the layout; other alpha values are refused.
std::optional<core::assets::Image16> readPng(const std::string& path, core::assets::PixelFormat16 layout,
                                             std::string& error);

}  // namespace bb::tools
