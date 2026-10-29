#pragma once

// The sprite banks of a source tree as SDL textures, loaded when first
// drawn (bbedit).

#include <map>
#include <string>
#include <utility>
#include <vector>

struct SDL_Renderer;
struct SDL_Texture;

namespace bb::editor {

struct Picture {
    SDL_Texture* texture = nullptr;
    int width = 0;
    int height = 0;
    int hotspotX = 0;
    int hotspotY = 0;
};

class Textures {
public:
    Textures(SDL_Renderer* renderer, std::string sourceDir)
        : renderer_(renderer), sourceDir_(std::move(sourceDir)) {}
    ~Textures();
    Textures(const Textures&) = delete;
    Textures& operator=(const Textures&) = delete;

    // The images of a bank by its game name ("snufenn.gfx"); empty when it
    // cannot be read (the error is kept, see error()).
    const std::vector<Picture>& bank(const std::string& name);
    // One image; null when there is no such image.
    const Picture* picture(const std::string& bank, int index);
    const std::string& error(const std::string& bank) const;

    // Forgets a bank (its files changed).
    void reload(const std::string& name);

private:
    SDL_Renderer* renderer_;
    std::string sourceDir_;
    std::map<std::string, std::vector<Picture>> banks_;
    std::map<std::string, std::string> errors_;
};

}  // namespace bb::editor
