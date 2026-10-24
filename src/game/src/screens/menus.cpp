// The main menu and the pause menu (menu_main.cpp, menu_game.cpp).

#include "bb/game/menus.hpp"

#include <cstdio>
#include <cstring>
#include <string>

#include "bb/core/input/codes.hpp"
#include "bb/core/input/names.hpp"
#include "bb/game/world.hpp"

namespace bb::game {

namespace {

namespace input = core::input;

// The strings of the menus.
enum Text {
    kResumeText = 800,
    kOptionsText = 801,
    kExitGameText = 805,
    kVsyncText = 806,
    kReturnText = 808,
    kOnText = 809,
    kOffText = 810,
    kPlayer1KeysText = 819,
    kPlayer2KeysText = 820,
    kUpText = 821,  // up, down, left, right, fire, jump, special
    kStartGameText = 830,
    kOnePlayerText = 831,
    kTwoPlayersText = 832,
    kExitText = 833,
    kCancelText = 834,
};

// The aliases of the first player's up, down, left, right and fire.
constexpr int kAliasUp = AliasControls::kFirstPlayer + AliasControls::kUp;
constexpr int kAliasDown = AliasControls::kFirstPlayer + AliasControls::kDown;
constexpr int kAliasLeft = AliasControls::kFirstPlayer + AliasControls::kLeft;
constexpr int kAliasRight = AliasControls::kFirstPlayer + AliasControls::kRight;
constexpr int kAliasFire = AliasControls::kFirstPlayer + AliasControls::kFire;

}  // namespace

//-----------------------------------------------------------------------------
// Menu
//-----------------------------------------------------------------------------

Menu::Menu(Services& services, int pauseOptionsPage, int lineHeight) noexcept
    : services_(services), optionsPage_(pauseOptionsPage), lineHeight_(lineHeight) {}

void Menu::setPage(int page, int items, int focusedItem) {
    page_ = page;
    items_ = items;
    focus_ = focusedItem;
    updateItems();
}

bool Menu::waitFor(MenuWait w) {
    wait_ = w;
    return false;
}

bool Menu::done(int& result, int value) {
    stage_ = 0;
    wait_ = MenuWait::None;
    result = value;
    return true;
}

bool Menu::keyHeld(unsigned int key) const {
    return services_.input->held(key);
}

bool Menu::aliasHeld(int alias) const {
    return services_.input->aliasHeld(alias);
}

void Menu::setItem(int i, const char* text) {
    std::snprintf(text_[static_cast<std::size_t>(i)].data(), text_[0].size(), "%s", text);
}

void Menu::toggleVsync() {
    services_.options->vsync = !services_.options->vsync;
    updateItems();
}

void Menu::optionsItems() {
    const TextSource& t = *services_.texts;
    std::snprintf(text_[0].data(), text_[0].size(), "%s %s", t.text(kVsyncText),
                  t.text(services_.options->vsync ? kOnText : kOffText));
    setItem(1, t.text(kPlayer1KeysText));
    setItem(2, t.text(kPlayer2KeysText));
    setItem(3, t.text(kReturnText));
}

void Menu::keyItems(int firstAlias) {
    const TextSource& t = *services_.texts;
    // The page lists the seven actions in the order of the alias table.
    static const int kItemAlias[7] = {AliasControls::kUp,     AliasControls::kDown, AliasControls::kLeft,
                                      AliasControls::kRight,  AliasControls::kFire, AliasControls::kJump,
                                      AliasControls::kSpecial};
    for (int i = 0; i < 7; ++i) {
        const std::string key =
            redefining_ == i
                ? std::string("...")
                : input::displayName(
                      static_cast<input::Code>(services_.input->alias(firstAlias + kItemAlias[i])),
                      input::Language::English);
        std::snprintf(text_[static_cast<std::size_t>(i)].data(), text_[0].size(), "%s = %s",
                      t.text(kUpText + i), key.c_str());
    }
    setItem(7, t.text(kReturnText));
}

void Menu::setRedefinedKey(unsigned int code) {
    static const int kItemAlias[7] = {AliasControls::kUp,     AliasControls::kDown, AliasControls::kLeft,
                                      AliasControls::kRight,  AliasControls::kFire, AliasControls::kJump,
                                      AliasControls::kSpecial};
    if (redefining_ < 0 || redefining_ > 6) {
        return;
    }
    const int first = page_ == kKeys1Page ? AliasControls::kFirstPlayer : AliasControls::kSecondPlayer;
    services_.input->setAlias(first + kItemAlias[redefining_], code);
}

void Menu::draw() {
    namespace gfx = core::gfx;
    gfx::Pixmap& screen = *services_.screen;
    gfx::Pixmap& backdrop = *services_.backdrop;
    const gfx::BitmapFont& font = *services_.fonts->menu;
    const gfx::BitmapFont& focusedFont = *services_.fonts->menus;

    const int halfHeight = items_ * lineHeight_ / 2;
    int y = 240 - halfHeight;

    // The band: the widest item and 30 pixels on each side, from 400 to 640
    // pixels wide.
    int halfWidth = 0;
    for (int i = 0; i < items_; ++i) {
        const int w = font.width(text_[static_cast<std::size_t>(i)].data());
        if (w > halfWidth) {
            halfWidth = w;
        }
    }
    halfWidth += 60;
    halfWidth /= 2;
    if (halfWidth < 200) {
        halfWidth = 200;
    } else if (halfWidth > 320) {
        halfWidth = 320;
    }

    // Darkened again when the page changes or the band moves by more than
    // 20 pixels.
    const int moved = (320 - halfWidth) - bandLeft_;
    if (page_ != bandPage_ || moved > 20 || moved < -20) {
        gfx::blit(backdrop, 0, 0, screen);
        bandLeft_ = 320 - halfWidth;
        const gfx::Rect band{320 - halfWidth, 220 - halfHeight, 2 * halfWidth, 40 + 2 * halfHeight};
        gfx::darken(backdrop, &band);
        bandPage_ = page_;
    }
    gfx::blit(screen, 0, 0, backdrop);

    for (int i = 0; i < items_; ++i) {
        (focus_ == i ? focusedFont : font)
            .printCentred(screen, 320, y, text_[static_cast<std::size_t>(i)].data());
        y += lineHeight_;
    }
}

//-----------------------------------------------------------------------------
// MainMenu
//-----------------------------------------------------------------------------

namespace {

enum MainStage {
    kStart,
    kRedefineKey,
    kRedefineDone,
    kKeys,
    kReturnKey,
    kReturnAction,
    kSideKeys,
    kSideAction
};

}  // namespace

void MainMenu::start() {
    setPage(kMainPage, 3, 0);
    musicWasOn_ = services_.audio->musicOn;
    soundWasOn_ = services_.audio->soundOn;
}

void MainMenu::updateItems() {
    const TextSource& t = *services_.texts;
    switch (page_) {
        case kMainPage:
            setItem(0, t.text(kStartGameText));
            setItem(1, "OPTIONS");
            setItem(2, t.text(kExitText));
            break;
        case kStartPage:
            setItem(0, t.text(kOnePlayerText));
            setItem(1, t.text(kTwoPlayersText));
            setItem(2, t.text(kReturnText));
            break;
        case kExitPage:
            setItem(0, t.text(kExitText));
            setItem(1, t.text(kCancelText));
            break;
        case kOptionsPage:
            optionsItems();
            break;
        case kKeys1Page:
            keyItems(AliasControls::kFirstPlayer);
            break;
        case kKeys2Page:
            keyItems(AliasControls::kSecondPlayer);
            break;
        default:
            break;
    }
}

bool MainMenu::update(int& result) {
    Input& in = *services_.input;

    for (;;) {
        switch (stage_) {
            case kStart:
                // Two updates of the input, as in 2002 (update() then
                // anyKeyPressed()).
                in.update();
                keyHeld_ = in.anyPressed();
                if (redefining_ != -1) {
                    stage_ = kRedefineKey;
                    return waitFor(MenuWait::Key);
                }
                stage_ = kKeys;
                break;

            case kRedefineKey:
                setRedefinedKey(waitedKey_);
                stage_ = kRedefineDone;
                return waitFor(MenuWait::Release);

            case kRedefineDone:
                redefining_ = -1;
                stage_ = kKeys;
                break;

            case kKeys:
                updateItems();
                if (keyHeld(input::kKeyUp) || aliasHeld(kAliasUp)) {
                    focus_ -= 1;
                    if (focus_ < 0) {
                        focus_ = items_ - 1;
                    }
                    stage_ = kReturnKey;
                    return waitFor(MenuWait::Release);
                }
                if (keyHeld(input::kKeyDown) || aliasHeld(kAliasDown)) {
                    focus_ += 1;
                    focus_ %= items_;
                    stage_ = kReturnKey;
                    return waitFor(MenuWait::Release);
                }
                stage_ = kReturnKey;
                break;

            case kReturnKey:
                if (keyHeld(input::kKeyReturn) || aliasHeld(kAliasFire)) {
                    stage_ = kReturnAction;
                    return waitFor(MenuWait::Release);
                }
                stage_ = kSideKeys;
                break;

            case kReturnAction:
                stage_ = kSideKeys;
                switch (page_) {
                    case kMainPage:
                        switch (focus_) {
                            case 0:
                                setPage(kStartPage, 3, 0);
                                return done(result, kContinue);
                            case 1:
                                setPage(kOptionsPage, 4, 3);
                                return done(result, kContinue);
                            case 2:
                                setPage(kExitPage, 2, 0);
                                return done(result, kContinue);
                            default:
                                break;
                        }
                        break;

                    case kStartPage:
                        switch (focus_) {
                            case 0:
                                return done(result, kStartOnePlayer);
                            case 1:
                                return done(result, kStartTwoPlayers);
                            case 2:
                                setPage(kMainPage, 3, 0);  // and on to the side keys
                                break;
                            default:
                                break;
                        }
                        break;

                    case kExitPage:
                        switch (focus_) {
                            case 0:
                                return done(result, kExit);
                            case 1:
                                setPage(kMainPage, 3, 2);
                                break;
                            default:
                                break;
                        }
                        break;

                    case kOptionsPage:
                        switch (focus_) {
                            case 0:
                                toggleVsync();
                                return done(result, kContinue);
                            case 1:
                                setPage(kKeys1Page, 8, 7);
                                return done(result, kContinue);
                            case 2:
                                setPage(kKeys2Page, 8, 7);
                                return done(result, kContinue);
                            case 3:
                                setPage(kMainPage, 3, 1);
                                return done(result, kContinue);
                            default:
                                break;
                        }
                        break;

                    case kKeys1Page:
                    case kKeys2Page:
                        if (focus_ == 7) {
                            setPage(kOptionsPage, 4, page_ == kKeys1Page ? 1 : 2);
                        } else {
                            redefining_ = focus_;
                            updateItems();
                        }
                        return done(result, kContinue);

                    default:
                        break;
                }
                break;

            case kSideKeys:
                if (keyHeld(input::kKeyRight) || aliasHeld(kAliasRight) || keyHeld(input::kKeyLeft) ||
                    aliasHeld(kAliasLeft)) {
                    stage_ = kSideAction;
                    return waitFor(MenuWait::Release);
                }
                return done(result, kContinue);

            case kSideAction:
                // Right and left do the same: they toggle the vsync option.
                if (page_ == kOptionsPage && focus_ == 0) {
                    toggleVsync();
                }
                return done(result, kContinue);

            default:
                return done(result, kContinue);
        }
    }
}

void MainMenu::stop() {
    AudioOutput& audio = *services_.audio;
    Banks& banks = services_.banks;

    // The sound switched on loads the heroes' sounds (and only them).
    if (audio.soundOn != soundWasOn_) {
        if (audio.soundOn) {
            loadSounds(*services_.assets, "data/bb.sfx", *banks.heroSounds);
        } else {
            banks.heroSounds->clear();
        }
    }

    // The music switched on loads and plays the level's music again.
    if (audio.musicOn != musicWasOn_) {
        if (audio.musicOn) {
            if (!banks.levelMusicFile.empty()) {
                if (!loadMusic(*services_.assets, banks.levelMusicFile, *banks.levelMusic)) {
                    audio.musicOn = false;
                } else if (services_.world->currentMusic >= 0) {
                    banks.levelMusic->play(services_.world->currentMusic);
                }
            }
        } else {
            banks.levelMusic->stopAll();
            banks.levelMusic->clear();
        }
    }

    bandPage_ = -1;
}

//-----------------------------------------------------------------------------
// PauseMenu
//-----------------------------------------------------------------------------

namespace {

enum PauseStage {
    kPauseStart,
    kPauseRedefineKey,
    kPauseRedefineDone,
    kPauseKeys,
    kPauseReturnKey,
    kPauseReturnAction,
    kPauseRightKey,
    kPauseRightAction,
    kPauseLeftKey,
    kPauseLeftAction,
    kPauseEscapeKey,
    kPauseEscapeAction,
};

}  // namespace

void PauseMenu::start() {
    setPage(kMainPage, 3, 0);
}

void PauseMenu::updateItems() {
    const TextSource& t = *services_.texts;
    if (page_ == kMainPage) {
        setItem(0, t.text(kResumeText));
        setItem(1, t.text(kOptionsText));
        setItem(2, t.text(kExitGameText));
    } else if (page_ == optionsPage_) {
        optionsItems();
    } else if (page_ == kKeys1Page) {
        keyItems(AliasControls::kFirstPlayer);
    } else if (page_ == kKeys2Page) {
        keyItems(AliasControls::kSecondPlayer);
    }
}

bool PauseMenu::update(int& result) {
    Input& in = *services_.input;

    for (;;) {
        switch (stage_) {
            case kPauseStart:
                in.update();
                if (redefining_ != -1) {
                    stage_ = kPauseRedefineKey;
                    return waitFor(MenuWait::Key);
                }
                stage_ = kPauseKeys;
                break;

            case kPauseRedefineKey:
                setRedefinedKey(waitedKey_);
                stage_ = kPauseRedefineDone;
                return waitFor(MenuWait::Release);

            case kPauseRedefineDone:
                redefining_ = -1;
                stage_ = kPauseKeys;
                break;

            case kPauseKeys:
                updateItems();
                if (keyHeld(input::kKeyUp) || aliasHeld(kAliasUp)) {
                    focus_ -= 1;
                    if (focus_ < 0) {
                        focus_ = items_ - 1;
                    }
                    stage_ = kPauseReturnKey;
                    return waitFor(MenuWait::Release);
                }
                if (keyHeld(input::kKeyDown) || aliasHeld(kAliasDown)) {
                    focus_ += 1;
                    focus_ %= items_;
                    stage_ = kPauseReturnKey;
                    return waitFor(MenuWait::Release);
                }
                stage_ = kPauseReturnKey;
                break;

            case kPauseReturnKey:
                if (keyHeld(input::kKeyReturn) || aliasHeld(kAliasFire)) {
                    stage_ = kPauseReturnAction;
                    return waitFor(MenuWait::Release);
                }
                stage_ = kPauseRightKey;
                break;

            case kPauseReturnAction:
                stage_ = kPauseRightKey;
                if (page_ == kMainPage) {
                    switch (focus_) {
                        case 0:
                            return done(result, kResume);
                        case 1:
                            setPage(optionsPage_, 4, 3);
                            return done(result, kStay);
                        case 2:
                            return done(result, kLeaveLevel);
                        default:
                            break;
                    }
                } else if (page_ == optionsPage_) {
                    switch (focus_) {
                        case 0:
                            toggleVsync();
                            return done(result, kStay);
                        case 1:
                            setPage(kKeys1Page, 8, 7);
                            return done(result, kStay);
                        case 2:
                            setPage(kKeys2Page, 8, 7);
                            return done(result, kStay);
                        case 3:
                            setPage(kMainPage, 3, 1);
                            return done(result, kStay);
                        default:
                            break;
                    }
                } else if (page_ == kKeys1Page || page_ == kKeys2Page) {
                    if (focus_ == 7) {
                        setPage(optionsPage_, 4, 3);
                    } else {
                        redefining_ = focus_;
                        updateItems();
                    }
                    return done(result, kStay);
                }
                break;

            case kPauseRightKey:
                if (keyHeld(input::kKeyRight) || aliasHeld(kAliasRight)) {
                    stage_ = kPauseRightAction;
                    return waitFor(MenuWait::Release);
                }
                stage_ = kPauseLeftKey;
                break;

            case kPauseRightAction:
                stage_ = kPauseLeftKey;
                // (The language choice of 2002 is commented out.)
                if (page_ == optionsPage_ && focus_ == 0) {
                    toggleVsync();
                    return done(result, kStay);
                }
                break;

            case kPauseLeftKey:
                if (keyHeld(input::kKeyLeft) || aliasHeld(kAliasLeft)) {
                    stage_ = kPauseLeftAction;
                    return waitFor(MenuWait::Release);
                }
                stage_ = kPauseEscapeKey;
                break;

            case kPauseLeftAction:
                stage_ = kPauseEscapeKey;
                if (page_ == optionsPage_ && focus_ == 0) {
                    toggleVsync();
                    return done(result, kStay);
                }
                break;

            case kPauseEscapeKey:
                if (keyHeld(input::kKeyEscape)) {
                    stage_ = kPauseEscapeAction;
                    return waitFor(MenuWait::Release);
                }
                return done(result, kStay);

            case kPauseEscapeAction:
                return done(result, kResume);

            default:
                return done(result, kStay);
        }
    }
}

}  // namespace bb::game
