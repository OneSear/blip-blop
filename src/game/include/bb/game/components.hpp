#pragma once

namespace bb::game {

// The parts entities are composed of (migration.md DEC-08): the 2002
// Sprite and Personnage kept these as loose fields and repeated the code
// that runs them in every class.

// An animation clock: a step that advances every `delay` ticks
// (etape and ss_etape).
class AnimationPlayer {
public:
    int frame = 0;     // the step (etape)
    int subFrame = 0;  // ticks within the step (ss_etape)

    // Counts one tick of a step `delay` ticks long; true when it moved
    // to the next step.
    bool tick(int delay) noexcept {
        subFrame += 1;
        subFrame %= delay;
        if (subFrame == 0) {
            frame += 1;
            return true;
        }
        return false;
    }

    // Same, back to step 0 after `steps` steps.
    void cycle(int delay, int steps) noexcept {
        if (tick(delay)) {
            frame %= steps;
        }
    }

    // Same, holding step `last` once there.
    void tickUpTo(int delay, int last) noexcept {
        subFrame += 1;
        subFrame %= delay;
        if (subFrame == 0 && frame < last) {
            frame += 1;
        }
    }

    // Plays `table` (`steps` entries, one every `delay` ticks) in a loop
    // and returns the current entry (anime()).
    int loop(const int* table, int steps, int delay) noexcept {
        tick(delay);
        if (frame < 0) {
            frame = 0;
        }
        frame %= steps;
        return table[frame];
    }

    // Back to the first tick of the first step.
    void restart() noexcept {
        frame = 0;
        subFrame = 0;
    }
};

// The box an entity is hit and hits with, in level coordinates (corners
// inclusive); it counts only when active.
class Hitbox {
public:
    int x1 = -1000;
    int y1 = -1000;
    int x2 = -2000;
    int y2 = -2000;
    bool active = false;

    // Sets the corners and activates it.
    void set(int left, int top, int right, int bottom) noexcept {
        x1 = left;
        y1 = top;
        x2 = right;
        y2 = bottom;
        active = true;
    }

    // Box overlap of two active hitboxes (the 2002 Sprite::collision).
    bool overlaps(const Hitbox& other) const noexcept {
        if (!active || !other.active) {
            return false;
        }
        return !(other.x2 < x1 || other.x1 > x2 || other.y2 < y1 || other.y1 > y2);
    }
};

// Hit points (pv).
class Health {
public:
    int points = 0;

    // Takes up to `damage` points and returns how many were taken: all of
    // them when the damage is smaller than the points left, else what was
    // left (then none is).
    int take(int damage) noexcept {
        if (damage < points) {
            points -= damage;
            return damage;
        }
        const int taken = points;
        points = 0;
        return taken;
    }

    // Adds `amount` points, up to `most`.
    void heal(int amount, int most) noexcept {
        points += amount;
        if (points > most) {
            points = most;
        }
    }

    // Whether points are left.
    bool left() const noexcept { return points > 0; }
};

}  // namespace bb::game
