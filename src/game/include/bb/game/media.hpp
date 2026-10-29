#pragma once

#include "bb/core/gfx/font.hpp"
#include "bb/core/gfx/sprite.hpp"

namespace bb::game {

// An image of a sprite bank: its size and its hotspot (the point placed at
// the entity's position), and a bank of them indexed as in the 2002 code
// (the index is not checked).
using Image = core::gfx::Sprite;
using ImageBank = core::gfx::SpriteBank;

// A sound bank.
class SoundPlayer {
public:
    static constexpr int kLoop = 1;  // SOUND_LOOP of the 2002 code

    virtual ~SoundPlayer() = default;
    virtual void play(int sound, int flags = 0) = 0;
    virtual void stop(int sound) = 0;
};

// A music bank: one music plays at a time.
class MusicPlayer {
public:
    virtual ~MusicPlayer() = default;
    virtual void play(int music) = 0;
    virtual void stop(int music) = 0;
};

// The strings of the game by id (the 2002 txt_data and gameText()).
class TextSource {
public:
    virtual ~TextSource() = default;
    // The string `id`; empty when there is none.
    virtual const char* text(int id) const = 0;
};

// The fonts the game draws with (fnt_* of the 2002 code).
struct Fonts {
    const core::gfx::BitmapFont* cool = nullptr;  // banners, timer, loading
    const core::gfx::BitmapFont* rpg = nullptr;   // dialogues, prompts
    const core::gfx::BitmapFont* menu = nullptr;
    const core::gfx::BitmapFont* menus = nullptr;
    const core::gfx::BitmapFont* scoreBlip = nullptr;    // scorei
    const core::gfx::BitmapFont* scoreBlop = nullptr;    // scoreo
    const core::gfx::BitmapFont* scoreBleach = nullptr;  // scorep (four-player mode)
    const core::gfx::BitmapFont* scoreBlitz = nullptr;   // scoreg
    const core::gfx::BitmapFont* ammo = nullptr;
    const core::gfx::BitmapFont* ammoUsed = nullptr;  // while firing

    // The score font of hero `hero` (a Hero value: 0 Blip to 3 Blitz).
    const core::gfx::BitmapFont* score(int hero) const noexcept {
        const core::gfx::BitmapFont* fonts[] = {scoreBlip, scoreBlop, scoreBleach, scoreBlitz};
        return hero >= 0 && hero < 4 ? fonts[hero] : scoreBlip;
    }
};

// Where the game draws: the back buffer, in screen coordinates.
class Canvas {
public:
    Canvas() = default;
    Canvas(core::gfx::Pixmap* screen, const Fonts* fonts, const TextSource* texts) noexcept
        : screen_(screen), fonts_(fonts), texts_(texts) {}

    core::gfx::Pixmap& screen() const noexcept { return *screen_; }

    // Draws image with its hotspot at (x, y); a null image draws nothing.
    void draw(const Image* image, int x, int y) const {
        if (image != nullptr) {
            core::gfx::draw(*screen_, *image, x, y);
        }
    }
    // Copies the rectangle [left, right) x [top, bottom) of image with its
    // top-left corner at (x, y). (The 2002 code asked for an opaque or a
    // keyed copy; the SDL port drew both with the colour key, and so does
    // this.)
    void drawPart(const Image* image, int left, int top, int right, int bottom, int x, int y) const {
        const core::gfx::Rect part{left, top, right - left, bottom - top};
        core::gfx::blit(*screen_, x, y, image->pixels(), &part);
    }
    // Draws string `text` of the string table centred on x, in the banner
    // font.
    void drawText(int x, int y, int text) const {
        fonts_->cool->printCentred(*screen_, x, y, texts_->text(text));
    }

private:
    core::gfx::Pixmap* screen_ = nullptr;
    const Fonts* fonts_ = nullptr;
    const TextSource* texts_ = nullptr;
};

// What the level's background takes from its entities (bb::game::Decor).
class LevelEffects {
public:
    virtual ~LevelEffects() = default;
    // Engraves image into the scrolling decor at level position (x, y)
    // (corpses, blood on walls); false when it could not (the 2002 grave()).
    virtual bool engrave(int x, int y, const Image* image) = 0;
};

}  // namespace bb::game
