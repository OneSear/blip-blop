#pragma once

#include "bb/game/actor.hpp"

namespace bb::game {

// A vehicle a player boards by walking into it (the 2002 Vehicule): while
// nobody rides it, it looks for a free player within 20 pixels; the rider
// leaves when he stops riding (dies, or the vehicle lets him off).
class Vehicle : public Actor {
public:
    explicit Vehicle(World& world) noexcept : Actor(world) {}
    void update() override;

protected:
    virtual bool riderCanFire() const = 0;
    virtual bool riderCanTurn() const = 0;
    virtual void updateRidden() = 0;  // updateUsed()
    virtual void updateEmpty() = 0;   // updateNotUsed()

    // Lets the rider off.
    void dropRider();

    Entity* rider() const { return rider_.get(); }
    bool boardable_ = true;  // can_be_used

private:
    Entity* findRider();  // scanne()

    Handle<> rider_;
};

}  // namespace bb::game
