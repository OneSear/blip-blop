#pragma once

#include <cstdint>

#include "bb/core/gfx/pixmap.hpp"

namespace bb::core::gfx {

// Fingerprint of what the pixmap shows on screen: each pixel's colour
// blended over black by its alpha (presentation draws frames over black),
// hashed with WordHash64 together with the size. Used by the regression
// replays (docs/replays.md); the value must never change for a given image.
std::uint64_t fingerprint(const Pixmap& pixmap);

}  // namespace bb::core::gfx
