// The credits (screens/credits_screen.cpp, the 2002 Game::showCredits()).

#include "bb/game/credits.hpp"

#include "bb/assets/sprites/inter.hpp"
#include "bb/core/assets/asset_source.hpp"

namespace bb::game {

namespace {

constexpr int kCreditsText = 1000;  // the lines of the credits, in order (99)

// Heights of a line, after a title and between two parts; the scrolling
// speed (ticks per pixel).
constexpr int kLineHeight = 20;
constexpr int kTitleGap = 40;
constexpr int kPartGap = 120;
constexpr int kTicksPerPixel = 2;

// Prints the credits from height y; returns the height under the last line.
int drawCreditsText(const core::gfx::BitmapFont& font, core::gfx::Pixmap& screen, const TextSource& texts,
                    int xcred, int y, bool theEnd) {
    font.printCentred(screen, xcred, y + kLineHeight, texts.text(kCreditsText + 0));

    int y1 = y + kLineHeight + kPartGap;

    font.printCentred(screen, xcred, y1, texts.text(kCreditsText + 1));
    font.printCentred(screen, xcred, y1 + kTitleGap + kLineHeight * 0, texts.text(kCreditsText + 2));
    font.printCentred(screen, xcred, y1 + kTitleGap + kLineHeight * 1, texts.text(kCreditsText + 3));

    int y2 = y1 + 2 * kLineHeight + kPartGap + kTitleGap;

    font.printCentred(screen, xcred, y2, texts.text(kCreditsText + 4));
    font.printCentred(screen, xcred, y2 + kLineHeight, texts.text(kCreditsText + 5));
    font.printCentred(screen, xcred, y2 + kTitleGap + kLineHeight, texts.text(kCreditsText + 6));

    int y3 = y2 + 2 * kLineHeight + kPartGap + kTitleGap;

    font.printCentred(screen, xcred, y3, texts.text(kCreditsText + 7));
    font.printCentred(screen, xcred, y3 + kTitleGap + 0 * kLineHeight, texts.text(kCreditsText + 8));
    font.printCentred(screen, xcred, y3 + kTitleGap + 1 * kLineHeight, texts.text(kCreditsText + 9));

    int y4 = y3 + 2 * kLineHeight + kPartGap + kTitleGap;

    font.printCentred(screen, xcred, y4, texts.text(kCreditsText + 10));
    font.printCentred(screen, xcred, y4 + kTitleGap + kLineHeight * 0, texts.text(kCreditsText + 11));
    font.printCentred(screen, xcred, y4 + kTitleGap + kLineHeight * 1, texts.text(kCreditsText + 12));
    font.printCentred(screen, xcred, y4 + kTitleGap + kLineHeight * 2, texts.text(kCreditsText + 13));
    font.printCentred(screen, xcred, y4 + kTitleGap + kLineHeight * 3, texts.text(kCreditsText + 14));
    font.printCentred(screen, xcred, y4 + kTitleGap + kLineHeight * 4, texts.text(kCreditsText + 15));

    int y5 = y4 + 5 * kLineHeight + kPartGap + kTitleGap;

    font.printCentred(screen, xcred, y5, texts.text(kCreditsText + 16));
    font.printCentred(screen, xcred, y5 + kLineHeight, texts.text(kCreditsText + 17));
    font.printCentred(screen, xcred, y5 + kTitleGap + 1 * kLineHeight, texts.text(kCreditsText + 18));
    font.printCentred(screen, xcred, y5 + kTitleGap + 2 * kLineHeight, texts.text(kCreditsText + 19));
    font.printCentred(screen, xcred, y5 + kTitleGap + 3 * kLineHeight, texts.text(kCreditsText + 20));
    font.printCentred(screen, xcred, y5 + kTitleGap + 4 * kLineHeight, texts.text(kCreditsText + 21));

    int y6 = y5 + 5 * kLineHeight + kPartGap + kTitleGap;

    font.printCentred(screen, xcred, y6, texts.text(kCreditsText + 22));
    font.printCentred(screen, xcred, y6 + kTitleGap + 0 * kLineHeight, texts.text(kCreditsText + 23));
    font.printCentred(screen, xcred, y6 + kTitleGap + 1 * kLineHeight, texts.text(kCreditsText + 24));

    int y7 = y6 + 2 * kLineHeight + kPartGap + kTitleGap;

    font.printCentred(screen, xcred, y7, texts.text(kCreditsText + 25));
    font.printCentred(screen, xcred, y7 + kTitleGap + 0 * kLineHeight, texts.text(kCreditsText + 26));
    font.printCentred(screen, xcred, y7 + kTitleGap + 1 * kLineHeight, texts.text(kCreditsText + 27));
    font.printCentred(screen, xcred, y7 + kTitleGap + 2 * kLineHeight, texts.text(kCreditsText + 28));
    font.printCentred(screen, xcred, y7 + kTitleGap + 3 * kLineHeight, texts.text(kCreditsText + 29));
    font.printCentred(screen, xcred, y7 + kTitleGap + 4 * kLineHeight, texts.text(kCreditsText + 30));

    int y8 = y7 + 5 * kLineHeight + kPartGap + kTitleGap;

    font.printCentred(screen, xcred, y8, texts.text(kCreditsText + 31));
    font.printCentred(screen, xcred, y8 + kTitleGap + 0 * kLineHeight, texts.text(kCreditsText + 32));
    font.printCentred(screen, xcred, y8 + kTitleGap + 1 * kLineHeight, texts.text(kCreditsText + 33));
    font.printCentred(screen, xcred, y8 + kTitleGap + 2 * kLineHeight, texts.text(kCreditsText + 34));
    font.printCentred(screen, xcred, y8 + kTitleGap + 3 * kLineHeight, texts.text(kCreditsText + 35));

    int y9 = y8 + 4 * kLineHeight + kPartGap + kTitleGap;
    int y10 = y9;
    int y11 = y10;

    if (theEnd) {
        font.printCentred(screen, xcred, y9, texts.text(kCreditsText + 36));
        font.printCentred(screen, xcred, y9 + kTitleGap + 0 * kLineHeight, texts.text(kCreditsText + 37));
        font.printCentred(screen, xcred, y9 + kTitleGap + 1 * kLineHeight, texts.text(kCreditsText + 38));
        font.printCentred(screen, xcred, y9 + kTitleGap + 2 * kLineHeight, texts.text(kCreditsText + 39));
        font.printCentred(screen, xcred, y9 + kTitleGap + 3 * kLineHeight, texts.text(kCreditsText + 40));
        font.printCentred(screen, xcred, y9 + kTitleGap + 4 * kLineHeight, texts.text(kCreditsText + 41));
        font.printCentred(screen, xcred, y9 + kTitleGap + 5 * kLineHeight, texts.text(kCreditsText + 42));
        font.printCentred(screen, xcred, y9 + kTitleGap + 6 * kLineHeight, texts.text(kCreditsText + 43));
        font.printCentred(screen, xcred, y9 + kTitleGap + 7 * kLineHeight, texts.text(kCreditsText + 44));
        font.printCentred(screen, xcred, y9 + kTitleGap + 8 * kLineHeight, texts.text(kCreditsText + 45));
        font.printCentred(screen, xcred, y9 + kTitleGap + 9 * kLineHeight, texts.text(kCreditsText + 46));
        font.printCentred(screen, xcred, y9 + kTitleGap + 10 * kLineHeight, texts.text(kCreditsText + 47));
        font.printCentred(screen, xcred, y9 + kTitleGap + 11 * kLineHeight, texts.text(kCreditsText + 48));
        font.printCentred(screen, xcred, y9 + kTitleGap + 12 * kLineHeight, texts.text(kCreditsText + 49));
        font.printCentred(screen, xcred, y9 + kTitleGap + 13 * kLineHeight, texts.text(kCreditsText + 50));

        y10 = y9 + 14 * kLineHeight + kPartGap + kTitleGap;

        font.printCentred(screen, xcred, y10, texts.text(kCreditsText + 51));
        font.printCentred(screen, xcred, y10 + kTitleGap + 0 * kLineHeight, texts.text(kCreditsText + 52));
        font.printCentred(screen, xcred, y10 + kTitleGap + 1 * kLineHeight, texts.text(kCreditsText + 53));

        font.printCentred(screen, xcred, y10 + kTitleGap + 3 * kLineHeight, texts.text(kCreditsText + 54));
        font.printCentred(screen, xcred, y10 + kTitleGap + 4 * kLineHeight, texts.text(kCreditsText + 55));

        font.printCentred(screen, xcred, y10 + kTitleGap + 6 * kLineHeight, texts.text(kCreditsText + 56));
        font.printCentred(screen, xcred, y10 + kTitleGap + 7 * kLineHeight, texts.text(kCreditsText + 57));

        font.printCentred(screen, xcred, y10 + kTitleGap + 9 * kLineHeight, texts.text(kCreditsText + 58));
        font.printCentred(screen, xcred, y10 + kTitleGap + 10 * kLineHeight, texts.text(kCreditsText + 59));

        font.printCentred(screen, xcred, y10 + kTitleGap + 12 * kLineHeight, texts.text(kCreditsText + 60));
        font.printCentred(screen, xcred, y10 + kTitleGap + 13 * kLineHeight, texts.text(kCreditsText + 61));

        font.printCentred(screen, xcred, y10 + kTitleGap + 15 * kLineHeight, texts.text(kCreditsText + 62));
        font.printCentred(screen, xcred, y10 + kTitleGap + 16 * kLineHeight, texts.text(kCreditsText + 63));

        font.printCentred(screen, xcred, y10 + kTitleGap + 18 * kLineHeight, texts.text(kCreditsText + 64));
        font.printCentred(screen, xcred, y10 + kTitleGap + 19 * kLineHeight, texts.text(kCreditsText + 65));

        font.printCentred(screen, xcred, y10 + kTitleGap + 21 * kLineHeight, texts.text(kCreditsText + 66));
        font.printCentred(screen, xcred, y10 + kTitleGap + 22 * kLineHeight, texts.text(kCreditsText + 67));

        font.printCentred(screen, xcred, y10 + kTitleGap + 24 * kLineHeight, texts.text(kCreditsText + 68));
        font.printCentred(screen, xcred, y10 + kTitleGap + 25 * kLineHeight, texts.text(kCreditsText + 69));

        font.printCentred(screen, xcred, y10 + kTitleGap + 27 * kLineHeight, texts.text(kCreditsText + 70));
        font.printCentred(screen, xcred, y10 + kTitleGap + 28 * kLineHeight, texts.text(kCreditsText + 71));

        font.printCentred(screen, xcred, y10 + kTitleGap + 30 * kLineHeight, texts.text(kCreditsText + 72));
        font.printCentred(screen, xcred, y10 + kTitleGap + 31 * kLineHeight, texts.text(kCreditsText + 73));

        font.printCentred(screen, xcred, y10 + kTitleGap + 33 * kLineHeight, texts.text(kCreditsText + 74));
        font.printCentred(screen, xcred, y10 + kTitleGap + 34 * kLineHeight, texts.text(kCreditsText + 75));

        font.printCentred(screen, xcred, y10 + kTitleGap + 36 * kLineHeight, texts.text(kCreditsText + 76));
        font.printCentred(screen, xcred, y10 + kTitleGap + 37 * kLineHeight, texts.text(kCreditsText + 77));

        font.printCentred(screen, xcred, y10 + kTitleGap + 39 * kLineHeight, texts.text(kCreditsText + 78));
        font.printCentred(screen, xcred, y10 + kTitleGap + 40 * kLineHeight, texts.text(kCreditsText + 79));

        font.printCentred(screen, xcred, y10 + kTitleGap + 42 * kLineHeight, texts.text(kCreditsText + 80));
        font.printCentred(screen, xcred, y10 + kTitleGap + 43 * kLineHeight, texts.text(kCreditsText + 81));

        font.printCentred(screen, xcred, y10 + kTitleGap + 45 * kLineHeight, texts.text(kCreditsText + 82));
        font.printCentred(screen, xcred, y10 + kTitleGap + 46 * kLineHeight, texts.text(kCreditsText + 83));

        font.printCentred(screen, xcred, y10 + kTitleGap + 48 * kLineHeight, texts.text(kCreditsText + 84));
        font.printCentred(screen, xcred, y10 + kTitleGap + 49 * kLineHeight, texts.text(kCreditsText + 85));

        font.printCentred(screen, xcred, y10 + kTitleGap + 51 * kLineHeight, texts.text(kCreditsText + 86));
        font.printCentred(screen, xcred, y10 + kTitleGap + 52 * kLineHeight, texts.text(kCreditsText + 87));

        font.printCentred(screen, xcred, y10 + kTitleGap + 54 * kLineHeight, texts.text(kCreditsText + 88));
        font.printCentred(screen, xcred, y10 + kTitleGap + 55 * kLineHeight, texts.text(kCreditsText + 89));

        font.printCentred(screen, xcred, y10 + kTitleGap + 57 * kLineHeight, texts.text(kCreditsText + 90));
        font.printCentred(screen, xcred, y10 + kTitleGap + 58 * kLineHeight, texts.text(kCreditsText + 91));

        font.printCentred(screen, xcred, y10 + kTitleGap + 60 * kLineHeight, texts.text(kCreditsText + 92));
        font.printCentred(screen, xcred, y10 + kTitleGap + 61 * kLineHeight, texts.text(kCreditsText + 93));

        font.printCentred(screen, xcred, y10 + kTitleGap + 64 * kLineHeight, texts.text(kCreditsText + 94));
        font.printCentred(screen, xcred, y10 + kTitleGap + 65 * kLineHeight, texts.text(kCreditsText + 95));
        font.printCentred(screen, xcred, y10 + kTitleGap + 66 * kLineHeight, texts.text(kCreditsText + 96));

        y11 = y10 + 72 * kLineHeight + kPartGap + kTitleGap;
    }

    font.printCentred(screen, xcred, y11, texts.text(kCreditsText + 97));
    font.printCentred(screen, xcred, y11 + kLineHeight, texts.text(kCreditsText + 98));

    return y11 + 2 * kLineHeight;
}

}  // namespace

Frame CreditsScreen::update(ScreenStack& stack) {
    Session& session = *services_.session;
    const auto now = [&] { return static_cast<std::uint32_t>(session.nowMicroseconds() / 1000); };

    if (!started_) {
        started_ = true;
        pageStart_ = now();
        if (theEnd_) {
            textX_ = 510;
            loadSprites(*services_.assets, "data/credits.gfx", pages_, false);
        }
        session.note("credits");
        clock_.reset(session.nowMicroseconds());
    }

    // Ends when the text has gone up, or on a key (not in the long
    // version); the 2002 test, in its order.
    if (!(!session.quitRequested() && (!services_.input->anyPressed() || theEnd_) && lastY_ > 0)) {
        if (theEnd_) {
            pages_.clear();
        }
        stack.finish();
        return Frame::Continue;
    }

    // The text moves up one pixel every two 11.8 ms ticks.
    for (int ticks = clock_.advance(session.nowMicroseconds()); ticks > 0; --ticks) {
        subStep_ = (subStep_ + 1) % kTicksPerPixel;
        if (subStep_ == 0) {
            y_ -= 1;
        }
    }

    services_.input->update();

    core::gfx::Pixmap& screen = *services_.screen;
    core::gfx::fill(screen, nullptr, core::gfx::kAlphaMask);
    if (theEnd_) {
        core::gfx::draw(screen, *pages_[page_], 0, 0);
    } else {
        core::gfx::draw(screen, *(*services_.banks.interface)[bb::assets::sprites::inter::kTitle], 0, 0);
    }

    lastY_ = drawCreditsText(*services_.fonts->rpg, screen, *services_.texts, textX_, y_, theEnd_);

    // A new picture every 12.5 seconds.
    if (now() - pageStart_ >= 12500 && theEnd_) {
        page_ += 1;
        page_ %= pages_.size();
        pageStart_ = now();
    }
    return Frame::Present;
}

}  // namespace bb::game
