// The stork the players ride at the start of finalboss (vehicule_cigogne.cpp).

#include "finalboss/finalboss_world.hpp"

namespace bb::game::finalboss {

namespace {

constexpr int kSpeed = 8;

// Whether the next stork to let its rider off is the first: one value for
// all storks, set again by each new one (2002 file static, kept in
// World::shared(): it decides where the rider lands, which the
// fingerprints see).
struct FirstDrop {
    bool value = true;
};

}  // namespace

Stork::Stork(World& world) : Vehicle(world) {
    image = enemyImage(img::kStork);
    world.shared<FirstDrop>().value = true;
}

void Stork::updateRidden() {
    boardable_ = false;
    x += kSpeed;

    Entity& player = *rider();
    player.asRider()->faceRight();
    player.x = x;
    player.y = y - 20;

    if (x >= world().marker("finalboss.storkDropX")) {
        bool& firstDrop = world().shared<FirstDrop>().value;
        player.y = firstDrop ? y - 80 : y - 40;
        dropRider();
        firstDrop = false;
    }
}

void Stork::updateEmpty() {
    x += kSpeed;
}

}  // namespace bb::game::finalboss
