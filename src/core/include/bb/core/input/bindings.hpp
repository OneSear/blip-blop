#pragma once

#include <array>
#include <string_view>

#include "bb/core/input/codes.hpp"

namespace bb::core::input {

// What a player can do; one physical input is bound to each action.
enum class Action { Up, Down, Left, Right, Fire, Jump, Special };

constexpr int kActionCount = 7;
constexpr int kPlayerCount = 2;

constexpr std::array<Action, kActionCount> kActions = {
    Action::Up, Action::Down, Action::Left, Action::Right, Action::Fire, Action::Jump, Action::Special};

// "up", "down", "left", "right", "fire", "jump", "special".
std::string_view actionName(Action action);

class Bindings {
public:
    // The defaults of the 2002 game (config.cpp): arrows, Left Ctrl to fire,
    // Left Alt to jump, Space for the special for player 1; Q S D F, Tab,
    // G, H for player 2.
    static Bindings defaults();

    // The pad layout a player gets without binding any pad input (see
    // usesPad()): pad 1 for the first player, pad 2 for the second; the
    // D-pad or the left stick to move, A to jump, X to fire, B for the
    // special (the buttons of a recognised game controller, codes.hpp).
    static Code defaultPadCode(int player, Action action);

    Code get(int player, Action action) const;
    // Whether one of the player's actions is bound to a pad input; the
    // default pad layout then no longer applies to the player.
    bool usesPad(int player) const;
    void set(int player, Action action, Code code);

    bool operator==(const Bindings& other) const { return codes_ == other.codes_; }
    bool operator!=(const Bindings& other) const { return !(*this == other); }

private:
    std::array<std::array<Code, kActionCount>, kPlayerCount> codes_{};
};

}  // namespace bb::core::input
