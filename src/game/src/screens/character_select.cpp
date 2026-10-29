// The character choice (screens/character_select_screen.cpp, the 2002
// Game::selectPlayer()).

#include "bb/assets/sprites/inter.hpp"
#include "bb/core/input/codes.hpp"
#include "bb/game/screens.hpp"
#include "bb/game/world.hpp"

namespace bb::game {

namespace {

namespace img = bb::assets::sprites::inter;
namespace input = core::input;

constexpr int kLoadingText = 851;  // "LOADING"

}  // namespace

void drawLoading(Services& services) {
    services.fonts->cool->printCentred(*services.screen, 320, 205, services.texts->text(kLoadingText));
}

Frame CharacterSelectScreen::update(ScreenStack& stack) {
    Input& in = *services_.input;
    World& w = *services_.world;
    core::gfx::Pixmap& screen = *services_.screen;
    const ImageBank& images = *services_.banks.interface;
    const auto draw = [&](int image, int x, int y) { core::gfx::draw(screen, *images[image], x, y); };

    if (!started_) {
        started_ = true;
        services_.session->note("character select");
    }

    if (services_.session->quitRequested()) {
        character_ = 0;
        stack.finish();
        return Frame::Continue;
    }

    in.update();

    // The arrows blink with the world's phase (every 10 frames here).
    blinkTick_ += 1;
    blinkTick_ %= 10;
    if (blinkTick_ == 0) {
        w.blink = !w.blink;
    }

    background1X_ -= 20;
    background2X_ -= 20;
    if (background1X_ == -640) {
        background1X_ = 640;
    }
    if (background2X_ == -640) {
        background2X_ = 640;
    }
    draw(img::kStarfield, background1X_, 0);
    draw(img::kStarfield + 1, background2X_, 0);

    // The title in English (the language is forced, D-25).
    if (titleY_ < 50) {
        titleY_ += 10;
    }
    draw(img::kSelectTitle, 320, titleY_);

    const auto right = [&] { return in.held(input::kKeyRight) || in.aliasHeld(AliasControls::kRight); };
    const auto left = [&] { return in.held(input::kKeyLeft) || in.aliasHeld(AliasControls::kLeft); };
    const auto choose = [&] { return in.held(input::kKeyReturn) || in.aliasHeld(AliasControls::kFire); };

    switch (step_) {
        case Step::BlipAppears:
            if (x_ < 240) {
                x_ += 20;
            } else if (right()) {
                step_ = Step::BlipLeaves;
            } else if (w.blink) {
                draw(img::kArrowRight, 620, 257);
            }
            if (nameX_ > 500) {
                nameX_ -= 20;
            }
            draw(img::kSelectBlip, x_, y_);
            draw(img::kNameBlip, nameX_, nameY_);

            if (x_ >= 240 && choose()) {
                drawLoading(services_);
                character_ = 0;
                stack.finish();
                return Frame::Present;
            }
            break;

        case Step::BlipLeaves:
            if (x_ > -300) {
                x_ -= 20;
            }
            if (nameX_ < 1040) {
                nameX_ += 20;
            }
            draw(img::kSelectBlip, x_, y_);
            draw(img::kNameBlip, nameX_, nameY_);

            if (x_ <= -300) {
                step_ = Step::BlopAppears;
                x_ = 930;
                nameX_ = -500;
                y_ = 320;
            }
            break;

        case Step::BlopAppears:
            if (x_ > 310) {
                x_ -= 20;
            } else if (left()) {
                step_ = Step::BlopLeaves;
            } else if (w.blink) {
                draw(img::kArrowLeft, 20, 254);
            }
            if (nameX_ < 120) {
                nameX_ += 20;
            }
            draw(img::kSelectBlop, x_, y_);
            draw(img::kNameBlop, nameX_, nameY_);

            if (x_ <= 310 && choose()) {
                drawLoading(services_);
                character_ = 1;
                stack.finish();
                return Frame::Present;
            }
            break;

        case Step::BlopLeaves:
            if (x_ < 930) {
                x_ += 20;
            }
            if (nameX_ > -500) {
                nameX_ -= 20;
            }
            draw(img::kSelectBlop, x_, y_);
            draw(img::kNameBlop, nameX_, nameY_);

            if (x_ >= 930) {
                step_ = Step::BlipAppears;
                x_ = -300;
                nameX_ = 1040;
                y_ = 255;
            }
            break;
    }
    return Frame::Present;
}

}  // namespace bb::game
