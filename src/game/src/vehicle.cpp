// The vehicle base (vehicule.cpp).

#include "bb/game/vehicle.hpp"

namespace bb::game {

void Vehicle::update() {
    Entity* current = rider_.get();
    if (current == nullptr) {
        if (boardable_) {
            current = findRider();
            rider_ = current;
        }
        if (current != nullptr) {
            current->asRider()->board(riderCanFire(), riderCanTurn());
        } else {
            updateEmpty();
            dieWhenOffscreen();
        }
        return;
    }

    if (!current->asRider()->riding() || current->dead) {
        rider_.reset();
        updateEmpty();
        return;
    }
    updateRidden();
}

void Vehicle::dropRider() {
    rider_.get()->asRider()->leave();
    rider_.reset();
}

Entity* Vehicle::findRider() {
    // The players list's own cursor, as in 2002.
    EntityList<Entity>& players = world().players;
    for (players.rewind(); !players.atEnd(); players.next()) {
        Entity* player = players.current();
        const int dx = player->x - x;
        const int dyPlayer = player->y - y;
        if (dx > -20 && dx < 20 && dyPlayer > -20 && dyPlayer < 20 && player->asRider()->canBoard()) {
            return player;
        }
    }
    return nullptr;
}

}  // namespace bb::game
