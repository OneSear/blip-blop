#include "png_io.hpp"

#include <cstdint>
#include <vector>

#include <stb_image.h>
#include <stb_image_write.h>

namespace bb::tools {

using core::assets::colorKey16;
using core::assets::fromPixel;
using core::assets::Image16;
using core::assets::PixelFormat16;
using core::assets::toPixel;

bool writePng(const std::string& path, const Image16& image, std::string& error) {
    const std::uint16_t key = colorKey16(image.format);
    std::vector<std::uint8_t> rgba;
    rgba.reserve(image.pixels.size() * 4);
    for (const std::uint16_t value : image.pixels) {
        const core::gfx::Pixel p = toPixel(value, image.format);
        rgba.push_back(core::gfx::red(p));
        rgba.push_back(core::gfx::green(p));
        rgba.push_back(core::gfx::blue(p));
        rgba.push_back(value == key ? 0 : 255);
    }
    if (image.width == 0 || image.height == 0) {
        error = path + ": empty image";
        return false;
    }
    if (stbi_write_png(path.c_str(), image.width, image.height, 4, rgba.data(), image.width * 4) == 0) {
        error = "cannot write " + path;
        return false;
    }
    return true;
}

std::optional<Image16> readPng(const std::string& path, PixelFormat16 layout, std::string& error) {
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (data == nullptr) {
        error = path + ": " + stbi_failure_reason();
        return std::nullopt;
    }
    Image16 image;
    image.width = width;
    image.height = height;
    image.format = layout;
    const std::size_t count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    image.pixels.resize(count);
    const std::uint16_t key = colorKey16(layout);
    bool ok = true;
    for (std::size_t i = 0; i < count; ++i) {
        const stbi_uc* p = data + 4 * i;
        if (p[3] == 0) {
            image.pixels[i] = key;
        } else if (p[3] == 255) {
            image.pixels[i] = fromPixel(core::gfx::makePixel(p[0], p[1], p[2]), layout);
        } else {
            error = path + ": partly transparent pixel " +
                    std::to_string(i % static_cast<std::size_t>(width)) + "," +
                    std::to_string(i / static_cast<std::size_t>(width)) + " (the art has none)";
            ok = false;
            break;
        }
    }
    stbi_image_free(data);
    if (!ok) {
        return std::nullopt;
    }
    return image;
}

}  // namespace bb::tools
