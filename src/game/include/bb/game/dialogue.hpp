#pragma once

#include <cstdint>
#include <sstream>
#include <string>

#include "bb/game/services.hpp"

namespace bb::game {

// Plays a scene of the level's dialogue script (.rpg, docs/formats/rpg.md;
// the 2002 RPGPlayer) over the level: two boxes, the top one and the
// bottom one, each with a face and a text, shown for a time or until a
// key is pressed.
class DialoguePlayer {
public:
    explicit DialoguePlayer(Services& services) noexcept : services_(services) {}

    // The level's script (attachFile()).
    void setScript(const char* name) { script_ = name; }
    // Starts scene n; false if the script or the scene is missing.
    bool begin(int scene);
    // One frame over the level already drawn; false when the scene is
    // over. escaped: Escape was pressed - nothing was drawn, and the scene
    // is to be skipped (skip()) once no key is held.
    bool draw(bool& escaped);
    // Runs the rest of the scene without showing it (its flags still set).
    void skip();
    void end();

private:
    static constexpr int kLineLength = 200;
    enum Box { kTop = 0, kBottom = 1 };
    enum Owner { kPlayer = 0, kEnemy = 1 };  // ID_JOUEUR, ID_ENNEMI

    bool update();  // updateScene(): up to the next shown box
    bool readLine();
    void error(const char* what);
    std::uint32_t now() const {
        return static_cast<std::uint32_t>(services_.session->nowMicroseconds() / 1000);
    }

    Services& services_;
    std::string script_;
    std::istringstream text_;
    char line_[kLineLength] = {};
    char value_[kLineLength] = {};

    const ImageBank* faces_[2] = {nullptr, nullptr};
    int image_[2] = {0, 0};
    int textId_[2] = {0, 0};
    int owner_[2] = {0, 0};
    int box_ = kTop;  // focus
    std::uint32_t waitUntil_ = 0;
    std::uint32_t shownAt_ = 0;
    bool keyReleased_ = false;
    int playerCount_ = 0;
    int player_ = 0;                  // whose faces the player's box shows (cur_joueur)
    int heroFirstImage_[2] = {0, 0};  // Blip 0, Blop 6
};

}  // namespace bb::game
