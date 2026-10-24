// The cutscene player (cine_player.cpp, screens/cutscene_screen.cpp).

#include "bb/game/cutscene.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "bb/core/assets/asset_source.hpp"
#include "bb/core/input/codes.hpp"
#include "bb/core/log.hpp"

namespace bb::game {

namespace {

// atoi() of 2002, without its undefined behaviour on overflow.
int toInt(const char* text) {
    return static_cast<int>(std::strtol(text, nullptr, 10));
}

}  // namespace

CutscenePlayer::CutscenePlayer(Services& services) : services_(services), music_(*services.audio) {}

void CutscenePlayer::reset() {
    // Only these fields: the others carry over from the previous scene, as
    // in 2002 (the same player plays the intro again in the attract loop).
    for (Object& o : objects_) {
        o.shown = false;
        o.moveTicks = 0;
        o.frames = 0;
        o.xWarp = false;
        o.yWarp = false;
    }

    lineNumber_ = 0;
    framesToDraw_ = 0;
    finished_ = false;
    clipLeft_ = 0;
    clipTop_ = 0;
    clipRight_ = 640;
    clipBottom_ = 480;
    whiteBack_[0] = whiteBack_[1] = false;
    whiteClip_[0] = whiteClip_[1] = false;
}

void CutscenePlayer::loadSprites(const char* name) {
    bb::game::loadSprites(*services_.assets, name, sprites_);
}

bool CutscenePlayer::begin(const char* name) {
    skipped_ = false;
    rendering_ = false;
    reset();

    std::string error;
    const auto text = services_.assets->script(name, error);
    if (!text) {
        core::Log::warning(std::string("CINEPlayer -> Cannot open file ") + name);
        return false;
    }

    script_.clear();
    script_.str(*text);

    char note[256];
    std::snprintf(note, sizeof(note), "cutscene %s", name);
    services_.session->note(note);
    return true;
}

// The 2002 loop, one frame per call: run the commands up to the next
// renderscene, then draw its frames one by one; Escape skips the scene
// (and so does a quit request, which the 2002 loop ignored until the end).
Frame CutscenePlayer::step() {
    for (;;) {
        if (rendering_) {
            if (framesToDraw_ > 0 && !finished_) {
                services_.input->update();
                renderFrame();

                if (services_.input->held(core::input::kKeyEscape) || services_.session->quitRequested()) {
                    finished_ = true;
                    skipped_ = true;
                }
                return Frame::Present;
            }
            rendering_ = false;
        }

        if (finished_) {
            music_.clear();
            sprites_.clear();
            script_.str(std::string());
            services_.session->note(skipped_ ? "cutscene skipped" : "cutscene ends");
            return Frame::Continue;
        }

        runCommands();
        clock_.reset(services_.session->nowMicroseconds());
        rendering_ = true;
    }
}

void CutscenePlayer::update() {
    // (The 2002 code also computed a volume and an alpha fade, which
    // nothing used: their drawing was commented out.)
    for (Object& o : objects_) {
        if (!o.shown) {
            continue;
        }
        if (o.moveTicks > 0) {
            o.x += o.dx;
            o.y += o.dy;
            o.moveTicks -= 1;
        }
        if (o.frames > 0) {
            o.frameWait += 1;
            o.frameWait %= o.frameDelay;
            o.image = sprites_[o.animation[o.frame]];

            if (o.frameWait == 0) {
                o.frame += 1;
                if (o.frame >= o.frames) {
                    if (o.loops) {
                        o.frame = 0;
                        o.image = sprites_[o.animation[0]];
                    } else {
                        o.frames = 0;
                    }
                }
            }
        }
        if (o.xWarp && o.x == o.xWarpFrom) {
            o.x = o.xWarpTo;
        }
        if (o.yWarp && o.y == o.yWarpFrom) {
            o.y = o.yWarpTo;
        }
    }
    framesToDraw_ -= 1;
}

// The updates the time since the last frame allows (one per 11.8 ms tick),
// then the drawing.
void CutscenePlayer::renderFrame() {
    for (int ticks = clock_.advance(services_.session->nowMicroseconds()); ticks > 0; --ticks) {
        update();
    }
    draw();
}

bool CutscenePlayer::validObject(int n) {
    if (n < 0 || n >= kObjects) {
        error("numéro d'objet invalide");
        return false;
    }
    return true;
}

void CutscenePlayer::hide(Object& o) {
    o.shown = false;
    o.dx = 0;
    o.dy = 0;
    o.frames = 0;
    o.xWarp = false;
    o.yWarp = false;
}

void CutscenePlayer::runCommands() {
    bool toDraw = false;
    const auto is = [this](const char* name) { return std::strcmp(command_, name) == 0; };
    const int* a = intArgument_;

    while (!finished_ && !toDraw && readCommand()) {
        if (is("endscene")) {
            sprites_.clear();
            finished_ = true;
        } else if (is("loadgfx")) {
            sprites_.clear();
            if (!bb::game::loadSprites(*services_.assets, stringArgument_[0], sprites_)) {
                error("ne peut pas charger le fichier GFX");
            }
        } else if (is("loadmbk")) {
            music_.clear();
            if (!loadMusic(*services_.assets, stringArgument_[0], music_)) {
                error("ne peut pas charger la MBK");
            }
        } else if (is("playmusic")) {
            music_.play(a[0]);
        } else if (is("stopmusic")) {
            music_.stopAll();
        } else if (is("setpos")) {
            if (validObject(a[0])) {
                objects_[a[0]].x = a[1];
                objects_[a[0]].y = a[2];
            }
        } else if (is("setpic")) {
            if (validObject(a[0])) {
                if (a[1] < 0 || a[1] >= sprites_.size()) {
                    error("numéro d'image invalide");
                } else {
                    Object& o = objects_[a[0]];
                    o.kind = Object::Sprite;
                    o.image = sprites_[a[1]];
                    o.shown = true;
                }
            }
        } else if (is("setmove")) {
            if (validObject(a[0])) {
                Object& o = objects_[a[0]];
                o.dx = a[1];
                o.dy = a[2];
                o.moveTicks = a[3];
            }
        } else if (is("settext")) {
            if (validObject(a[0])) {
                Object& o = objects_[a[0]];
                o.kind = Object::Text;
                o.text = a[1];
                o.shown = true;
            }
        } else if (is("reset")) {
            if (validObject(a[0])) {
                hide(objects_[a[0]]);
            }
        } else if (is("resetall")) {
            for (Object& o : objects_) {
                hide(o);
            }
        } else if (is("setclip")) {
            clipLeft_ = a[0];
            clipTop_ = a[1];
            clipRight_ = a[2];
            clipBottom_ = a[3];
        } else if (is("noclip")) {
            clipLeft_ = 0;
            clipTop_ = 0;
            clipRight_ = 640;
            clipBottom_ = 480;
        } else if (is("setanim")) {
            if (validObject(a[0])) {
                Object& o = objects_[a[0]];
                o.frameDelay = a[1];
                o.loops = a[2] != 0;
                o.kind = Object::Sprite;
                o.image = sprites_[a[3]];
                o.frameWait = 0;
                o.frame = 0;
                o.frames = argumentCount_ - 3;
                for (int i = 3; i < argumentCount_; ++i) {
                    o.animation[i - 3] = a[i];
                }
            }
        } else if (is("setxwarp")) {
            if (validObject(a[0])) {
                Object& o = objects_[a[0]];
                o.xWarp = true;
                o.xWarpFrom = a[1];
                o.xWarpTo = a[2];
            }
        } else if (is("setywarp")) {
            if (validObject(a[0])) {
                Object& o = objects_[a[0]];
                o.yWarp = true;
                o.yWarpFrom = a[1];
                o.yWarpTo = a[2];
            }
        } else if (is("stopanim")) {
            if (validObject(a[0])) {
                objects_[a[0]].frames = 0;
            }
        } else if (is("setbackcolor") || is("setclipcolor")) {
            // A layer's colour: 0 black, anything else white (the white
            // flashes of intro.cin and end.cin; D-95 fixed: the 2014 port
            // always drew black).
            if (a[0] < 0 || a[0] > 1) {
                error("mauvais numéro de calque");
            } else {
                (is("setbackcolor") ? whiteBack_ : whiteClip_)[a[0]] = a[1] != 0;
            }
        } else if (is("setalpha") || is("setdeltaalpha") || is("setvolume") || is("setdeltavolume")) {
            // Fades that nothing used.
        } else if (is("renderscene")) {
            framesToDraw_ = a[0];
            toDraw = true;
        } else {
            syntaxError();
        }
    }
}

void CutscenePlayer::draw() {
    namespace gfx = core::gfx;
    gfx::Pixmap& screen = *services_.screen;
    const gfx::Pixel white = gfx::makePixel(255, 255, 255);
    const gfx::Pixel black = gfx::kAlphaMask;

    gfx::fill(screen, nullptr, whiteBack_[0] ? white : black);

    for (const Object& o : objects_) {
        if (o.shown && o.kind == Object::Sprite && o.image != nullptr) {
            gfx::draw(screen, *o.image, o.x, o.y);
        }
    }

    // The clip colour outside the clip rectangle.
    const gfx::Pixel clip = whiteClip_[0] ? white : black;
    const gfx::Rect left{0, 0, clipLeft_, 480};
    const gfx::Rect right{clipRight_, 0, 640 - clipRight_, 480};
    const gfx::Rect top{0, 0, 640, clipTop_};
    const gfx::Rect bottom{0, clipBottom_, 640, 480 - clipBottom_};
    gfx::fill(screen, &left, clip);
    gfx::fill(screen, &right, clip);
    gfx::fill(screen, &top, clip);
    gfx::fill(screen, &bottom, clip);

    for (const Object& o : objects_) {
        if (o.shown && o.kind == Object::Text) {
            services_.fonts->rpg->printCentred(screen, o.x, o.y, services_.texts->text(o.text));
        }
    }
}

// The next command of the script: lowercase, without spaces or quotes,
// "name(arg, arg, ...)". False at the end of the script or on a syntax
// error (which ends the scene).
bool CutscenePlayer::readCommand() {
    static const char separators[] = ",)\n";

    if (script_.eof()) {
        return false;
    }

    // Skips comments and empty lines.
    script_.getline(line_, kLineLength);
    lineNumber_ += 1;
    while (!script_.eof() && (line_[0] == ';' || line_[0] == '\0')) {
        script_.getline(line_, kLineLength);
        lineNumber_ += 1;
    }
    if (script_.eof() && std::strlen(line_) == 0) {
        return false;
    }

    int i = 0;
    int j = 0;
    char c = '\0';
    do {
        c = line_[i++];
        if (c == ';') {
            cleaned_[j] = '\0';
        } else if (c != ' ' && c != '"') {
            cleaned_[j++] = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    } while (c != '\0' && c != ';');
    cleaned_[j] = '\0';

    char* open = std::strchr(cleaned_, '(');
    if (open == nullptr) {
        return syntaxError();
    }
    open[0] = '\0';
    std::strcpy(command_, cleaned_);
    if (command_[0] == '\0') {
        return syntaxError();
    }

    argumentCount_ = 0;
    for (char* arg = std::strtok(open + 1, separators); arg != nullptr;
         arg = std::strtok(nullptr, separators)) {
        std::strcpy(stringArgument_[argumentCount_], arg);
        intArgument_[argumentCount_] = toInt(arg);
        argumentCount_ += 1;
    }
    return true;
}

bool CutscenePlayer::syntaxError() {
    core::Log::warning("CINEPlayer -> Syntax error line " + std::to_string(lineNumber_));
    finished_ = true;
    return false;
}

void CutscenePlayer::error(const char* what) {
    core::Log::warning("CINEPlayer -> Error line " + std::to_string(lineNumber_) + " : " + what);
    finished_ = true;
}

//-----------------------------------------------------------------------------
// CutsceneScreen
//-----------------------------------------------------------------------------

Frame CutsceneScreen::update(ScreenStack& stack) {
    const Frame frame = player_.step();
    if (frame == Frame::Continue) {
        stack.finish();
    }
    return frame;
}

}  // namespace bb::game
