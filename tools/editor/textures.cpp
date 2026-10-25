#include "textures.hpp"

#include <SDL.h>

#include <filesystem>
#include <vector>

#include "bb/core/assets/image16.hpp"
#include "source_assets.hpp"

namespace bb::editor {

namespace {

// The game's colours (bb::core::assets::toPixel, ABGR8888), the bank's key
// colour transparent as the game draws sprites.
SDL_Texture* makeTexture(SDL_Renderer* renderer, const core::assets::Image16& image) {
    if (image.width <= 0 || image.height <= 0) {
        return nullptr;
    }
    std::vector<core::gfx::Pixel> pixels(image.pixels.size());
    const std::uint16_t key = core::assets::colorKey16(image.format);
    for (std::size_t i = 0; i < pixels.size(); ++i) {
        pixels[i] = image.pixels[i] == key ? 0u : core::assets::toPixel(image.pixels[i], image.format);
    }
    SDL_Texture* texture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, image.width, image.height);
    if (texture == nullptr) {
        return nullptr;
    }
    SDL_UpdateTexture(texture, nullptr, pixels.data(), image.width * static_cast<int>(sizeof(core::gfx::Pixel)));
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    return texture;
}

}  // namespace

Textures::~Textures() {
    for (auto& [name, pictures] : banks_) {
        for (Picture& p : pictures) {
            if (p.texture != nullptr) {
                SDL_DestroyTexture(p.texture);
            }
        }
    }
}

const std::vector<Picture>& Textures::bank(const std::string& name) {
    const auto found = banks_.find(name);
    if (found != banks_.end()) {
        return found->second;
    }
    std::vector<Picture>& pictures = banks_[name];
    if (name.empty()) {
        return pictures;
    }
    const std::string dir = (std::filesystem::u8path(sourceDir_) / "sprites" / tools::stemOf(name)).u8string();
    std::string error;
    const auto sprites = tools::readSpriteBank(dir, error);
    if (!sprites) {
        errors_[name] = error;
        return pictures;
    }
    for (const auto& sprite : sprites->images) {
        Picture p;
        p.texture = makeTexture(renderer_, sprite.image);
        p.width = sprite.image.width;
        p.height = sprite.image.height;
        p.hotspotX = sprite.hotspotX;
        p.hotspotY = sprite.hotspotY;
        pictures.push_back(p);
    }
    return pictures;
}

const Picture* Textures::picture(const std::string& bankName, int index) {
    const auto& pictures = bank(bankName);
    if (index < 0 || index >= static_cast<int>(pictures.size()) || pictures[static_cast<std::size_t>(index)].texture == nullptr) {
        return nullptr;
    }
    return &pictures[static_cast<std::size_t>(index)];
}

const std::string& Textures::error(const std::string& bankName) const {
    static const std::string none;
    const auto it = errors_.find(bankName);
    return it == errors_.end() ? none : it->second;
}

void Textures::reload(const std::string& name) {
    const auto it = banks_.find(name);
    if (it != banks_.end()) {
        for (Picture& p : it->second) {
            if (p.texture != nullptr) {
                SDL_DestroyTexture(p.texture);
            }
        }
        banks_.erase(it);
    }
    errors_.erase(name);
}

}  // namespace bb::editor
