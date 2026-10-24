#pragma once

#include "bb/game/actor.hpp"

namespace bb::game {

// A drop of gore thrown by an explosion or a hit (the 2002 GoreGiclure):
// flies with gravity, its image following its speed, and vanishes on the
// ground. Whoever throws it adds it to World::blood or World::gore.
class GoreDrop : public Actor {
public:
    GoreDrop(World& world, int speedX, int speedY);
    void update() override;

private:
    void chooseImage();

    int xSpeed_;
};

}  // namespace bb::game
