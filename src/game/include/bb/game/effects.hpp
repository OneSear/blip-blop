#pragma once

#include <vector>

#include "bb/game/actor.hpp"

namespace bb::game {

// A blood spray that follows the entity it comes from for 4 steps, and
// stops when that entity dies (the 2002 Giclure).
class BloodSpray : public Actor {
public:
    BloodSpray(World& world, Entity& owner, int sprayDirection, int offsetX, int offsetY);
    void update() override;

private:
    Handle<> owner_;
    int offsetX_;
    int offsetY_;
};

// Sprays blood from owner in one of 8 directions, offset from its position
// (into World::blood).
void spawnBlood(World& world, Entity& owner, int direction, int offsetX, int offsetY);

// A bubble of the underwater levels: rises swaying until it leaves the
// screen or meets a wall (Bulle).
class Bubble : public Actor {
public:
    explicit Bubble(World& world) noexcept : Actor(world) {}
    void update() override;

    int centreX = 0;    // xbase
    int phase = 0;      // phi, degrees
    int phaseStep = 0;  // dphi
};

// Gives off a bubble from source now and then (Game::creeBulle()).
void emitBubble(World& world, Entity& source);

// A falling raindrop (MeteoPluie).
class Raindrop : public Actor {
public:
    explicit Raindrop(World& world) noexcept : Actor(world) {}
    void update() override;
};

// A snowflake drifting left as it falls (MeteoNeige).
class Snowflake : public Actor {
public:
    explicit Snowflake(World& world) noexcept : Actor(world) {}
    void update() override;

    int centreX = 0;  // xbase
    int phase = 0;    // phi, degrees
    int sway = 0;     // xwide
};

// The rain or snow of a level (World::weather): two pools of particles,
// reused in turn without being reset, kept at the weather's intensity in
// World::weatherParticles (Game::updateMeteo()).
class Precipitation {
public:
    explicit Precipitation(World& world);
    Precipitation(const Precipitation&) = delete;
    Precipitation& operator=(const Precipitation&) = delete;

    // Moves the particles, then adds new ones up to the intensity.
    void update();

private:
    World& world_;
    std::vector<Raindrop> drops_;
    std::vector<Snowflake> flakes_;
    int nextDrop_ = 0;
    int nextFlake_ = 0;
};

}  // namespace bb::game
