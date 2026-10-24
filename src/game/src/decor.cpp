// The scrolling background (scroll.cpp drawScrolling(), grave() and
// clipedBlit() of legacy_assets.cpp).

#include "bb/game/decor.hpp"

#include "bb/core/gfx/sprite.hpp"
#include "bb/game/actor.hpp"
#include "bb/game/world.hpp"

namespace bb::game {

namespace gfx = core::gfx;

Decor::Decor(World& world) : world_(world), ring_(kRingWidth, gfx::kScreenHeight) {}

void Decor::setLevel(const ImageBank* screens, std::vector<int> screenImages) {
    screens_ = screens;
    screenImages_ = std::move(screenImages);
}

void Decor::rewind() noexcept {
    screen_ = 0;
    screenX_ = 0;
    ringTurns_ = 0;
    ringX_ = 0;
}

void Decor::draw(gfx::Pixmap& dst) {
    Camera& camera = world_.camera;
    const int levelWidth = world_.collision.width;
    const int screenCount = world_.collision.screens;

    if (camera.x < 0) {
        camera.x = 0;
    } else if (camera.x > levelWidth - 640) {
        camera.x = levelWidth - 640;
    }

    const int x1 = camera.x % kRingWidth;
    const int x2 = (camera.x + 640) % kRingWidth;
    const int x3 = (camera.x + kRingWidth - 2) % kRingWidth;

    // Fills the ring up to 2 pixels before the camera's left edge, one
    // ring width ahead.
    while (screen_ < (camera.x + kRingWidth - 2) / 640 ||
           (screen_ < screenCount && ringX_ != (x3 + 1) % kRingWidth && ringX_ != x3 % kRingWidth)) {
        const gfx::Rect columns{screenX_, 0, 2, gfx::kScreenHeight};
        gfx::blit(ring_, ringX_, 0, (*screens_)[screenImages_[static_cast<std::size_t>(screen_)]]->pixels(),
                  &columns);

        screenX_ += 2;
        if (screenX_ == 640) {
            screenX_ = 0;
            screen_ += 1;
        }

        ringX_ += 2;
        if (ringX_ == kRingWidth) {
            ringX_ = 0;
            ringTurns_ += 1;
        }
    }

    if (x1 <= kRingWidth - 640) {
        const gfx::Rect part{x1, 0, 640, gfx::kScreenHeight};
        gfx::blit(dst, 0, 0, ring_, &part);
    } else {
        const gfx::Rect first{x1, 0, kRingWidth - x1, gfx::kScreenHeight};
        gfx::blit(dst, 0, 0, ring_, &first);
        const gfx::Rect second{0, 0, x2, gfx::kScreenHeight};
        gfx::blit(dst, kRingWidth - x1, 0, ring_, &second);
    }
}

void Decor::blitClipped(const Image& image, int x, int y, int left, int right) {
    // The vertical bounds are always 0 and 480.
    constexpr int kTop = 0;
    constexpr int kBottom = gfx::kScreenHeight;

    const int width = image.width();
    const int height = image.height();
    int x1 = x - image.spotX();
    int y1 = y - image.spotY();
    const int x2 = x1 + width;
    const int y2 = y1 + height;

    if (x2 < left || x1 > right || y1 > kBottom || y2 < kTop) {
        return;
    }

    int partLeft = 0;
    int partTop = 0;
    if (x1 < left) {
        partLeft = left - x1;
        x1 = left;
    }
    if (y1 < kTop) {
        partTop = kTop - y1;
        y1 = kTop;
    }
    const int partRight = x2 > right ? width - (x2 - right) : width;
    const int partBottom = y2 > kBottom ? height - (y2 - kBottom) : height;

    const gfx::Rect part{partLeft, partTop, partRight - partLeft, partBottom - partTop};
    gfx::blit(ring_, x1, y1, image.pixels(), &part);
}

bool Decor::engrave(int x, int y, const Image* image) {
    World& w = world_;

    if (image == nullptr || x > w.camera.x + kRingWidth - image->width()) {
        return false;
    }

    // Not over a prop or a moving platform.
    Marker box;
    box.x1 = x - image->spotX();
    box.y1 = y - image->spotY();
    box.x2 = box.x1 + image->width();
    box.y2 = box.y1 + image->height();
    box.collides = true;

    for (w.props.rewind(); !w.props.atEnd(); w.props.next()) {
        if (box.hits(*w.props.current())) {
            return false;
        }
    }
    for (w.movingPlatforms.rewind(); !w.movingPlatforms.atEnd(); w.movingPlatforms.next()) {
        if (box.hits(*w.movingPlatforms.current())) {
            return false;
        }
    }

    // The ring holds the end of one turn and the start of the next: the
    // decal goes into the part of the turn it falls in, and wraps.
    const int turn = x / kRingWidth;
    x %= kRingWidth;

    if (turn == ringTurns_ - 1) {
        blitClipped(*image, x, y, ringX_, kRingWidth);
        blitClipped(*image, x - kRingWidth, y, 0, ringX_);
        return true;
    }
    if (turn == ringTurns_) {
        blitClipped(*image, x, y, 0, ringX_);
        blitClipped(*image, x + kRingWidth, y, ringX_, kRingWidth);
        return true;
    }
    return false;
}

}  // namespace bb::game
