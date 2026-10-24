// The head-up display (screens/hud.cpp, the 2002 Game::drawAll()).

#include "bb/game/hud.hpp"

#include <cstdio>

#include "bb/assets/sprites/bb.hpp"
#include "bb/assets/sprites/misc.hpp"

namespace bb::game {

namespace {

namespace img = bb::assets::sprites::misc;
namespace common = bb::assets::sprites::bb;

constexpr int kTimeText = 828;   // "TIME"
constexpr int kBonusText = 856;  // "Bonus"
constexpr int kStageText = 857;  // "Stage"

struct Layout {
    bool rightSide;
    int lifeX;   // life points, at y = 20
    int scoreX;  // score, aligned towards the centre
    int edgeX;   // lives or "Bonus Stage", aligned to the edge
    int livesX;
    int weaponX;      // weapon picture, at y = 50
    int ammoX;        // ammunition, centred, at y = 43
    int cowBombX;     // first cow bomb, at y = 65
    int cowBombStep;  // towards the centre
};

constexpr Layout kPlayer1 = {false, 20, 190, 4, 5, 135, 135, 90, 23};
constexpr Layout kPlayer2 = {true, 560, 450, 635, 635, 510, 505, 520, -23};

// Text from x, or ending at x on the right side.
void printFromEdge(const core::gfx::BitmapFont& font, core::gfx::Pixmap& screen, bool rightSide, int x, int y,
                   const char* text) {
    if (rightSide) {
        font.printRight(screen, x, y, text);
    } else {
        font.print(screen, x, y, text);
    }
}

void drawLifePoints(const World& world, core::gfx::Pixmap& screen, int x, int y, int lifePoints) {
    // Five segments: the ones left (green), then the ones lost (grey), each
    // from the last image of its run.
    static const int dx[] = {3, 0, 13, 44, 31};
    static const int dy[] = {0, 21, 48, 21, 0};
    const ImageBank& players = *world.media.players;

    for (int i = 0; i < lifePoints; ++i) {
        core::gfx::draw(screen, *players[common::kLifeSegment + 4 - i], x + dx[i], y + dy[i]);
    }
    for (int i = lifePoints; i < 5; ++i) {
        core::gfx::draw(screen, *players[common::kLifeSegmentLost + 4 - i], x + dx[i], y + dy[i]);
    }
}

int weaponImage(Weapon weapon) {
    switch (weapon) {
        case Weapon::Rifle:
            return img::kWeaponRifle;
        case Weapon::Smg:
            return img::kWeaponSmg;
        case Weapon::Flamethrower:
            return img::kWeaponFlamethrower;
        case Weapon::Shotgun:
            return img::kWeaponShotgun;
        case Weapon::Laser:
            return img::kWeaponLaser;
    }
    return img::kWeaponRifle;
}

void drawPlayer(const World& world, const Player& player, const Layout& layout, bool bonusLevel, bool blink) {
    const Media& media = world.media;
    const Fonts& fonts = *media.fonts;
    core::gfx::Pixmap& screen = media.canvas->screen();
    char buffer[20];

    drawLifePoints(world, screen, layout.lifeX, 20, player.hitPoints);

    // The score towards the centre: right-aligned on the left side.
    std::snprintf(buffer, sizeof(buffer), "%07d", player.score);
    const core::gfx::BitmapFont& scoreFont = player.hero == Hero::Blip ? *fonts.scoreBlip : *fonts.scoreBlop;
    printFromEdge(scoreFont, screen, !layout.rightSide, layout.scoreX, 20, buffer);

    if (bonusLevel) {
        printFromEdge(*fonts.rpg, screen, layout.rightSide, layout.edgeX, 5, media.texts->text(kBonusText));
        printFromEdge(*fonts.rpg, screen, layout.rightSide, layout.edgeX, 20, media.texts->text(kStageText));
    } else {
        std::snprintf(buffer, sizeof(buffer), "%d", player.lives);
        printFromEdge(*fonts.cool, screen, layout.rightSide, layout.livesX, 5, buffer);
    }

    core::gfx::draw(screen, *(*media.misc)[weaponImage(player.weapon)], layout.weaponX, 50);

    if (player.weapon == Weapon::Rifle) {
        fonts.ammo->printCentred(screen, layout.ammoX, 43, "*");
    } else {
        std::snprintf(buffer, sizeof(buffer), "%d", player.ammo);
        const core::gfx::BitmapFont& ammoFont = player.firing && blink ? *fonts.ammoUsed : *fonts.ammo;
        ammoFont.printCentred(screen, layout.ammoX, 43, buffer);
    }

    for (int i = 0, x = layout.cowBombX; i < player.cowBombs; ++i, x += layout.cowBombStep) {
        core::gfx::draw(screen, *(*media.misc)[img::kHudCowBomb], x, 65);
    }
}

}  // namespace

void Hud::draw(const World& world, const Player* player1, const Player* player2, bool bonusLevel,
               bool blink) {
    if (player1 != nullptr && player1->lives > 0) {
        drawPlayer(world, *player1, kPlayer1, bonusLevel, blink);
    }
    if (player2 != nullptr && player2->lives > 0) {
        drawPlayer(world, *player2, kPlayer2, bonusLevel, blink);
    }

    const int timer = world.flags[Flag::Timer];
    if (timer > 0) {
        const Media& media = world.media;
        core::gfx::Pixmap& screen = media.canvas->screen();
        char buffer[12];  // any int
        media.fonts->cool->printCentred(screen, 320, 20, media.texts->text(kTimeText));
        std::snprintf(buffer, sizeof(buffer), "%d", timer);
        media.fonts->cool->printCentred(screen, 320, 50, buffer);
    }
}

}  // namespace bb::game
