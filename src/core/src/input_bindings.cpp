#include "bb/core/input/bindings.hpp"

namespace bb::core::input {

std::string_view actionName(Action action) {
    switch (action) {
        case Action::Up:
            return "up";
        case Action::Down:
            return "down";
        case Action::Left:
            return "left";
        case Action::Right:
            return "right";
        case Action::Fire:
            return "fire";
        case Action::Jump:
            return "jump";
        case Action::Special:
            return "special";
    }
    return "";
}

Bindings Bindings::defaults() {
    Bindings b;
    b.set(0, Action::Up, kKeyUp);
    b.set(0, Action::Down, kKeyDown);
    b.set(0, Action::Left, kKeyLeft);
    b.set(0, Action::Right, kKeyRight);
    b.set(0, Action::Fire, kKeyLeftCtrl);
    b.set(0, Action::Jump, kKeyLeftAlt);
    b.set(0, Action::Special, kKeySpace);
    b.set(1, Action::Up, kKeyQ);
    b.set(1, Action::Down, kKeyS);
    b.set(1, Action::Left, kKeyD);
    b.set(1, Action::Right, kKeyF);
    b.set(1, Action::Fire, kKeyTab);
    b.set(1, Action::Jump, kKeyG);
    b.set(1, Action::Special, kKeyH);
    return b;
}

Code Bindings::defaultPadCode(int player, Action action) {
    switch (action) {
        case Action::Up:
            return padCode(player, kPadUp);
        case Action::Down:
            return padCode(player, kPadDown);
        case Action::Left:
            return padCode(player, kPadLeft);
        case Action::Right:
            return padCode(player, kPadRight);
        case Action::Fire:
            return padCode(player, kPadButtonX);
        case Action::Jump:
            return padCode(player, kPadButtonA);
        case Action::Special:
            return padCode(player, kPadButtonB);
    }
    return 0;
}

bool Bindings::usesPad(int player) const {
    for (const Code code : codes_.at(static_cast<std::size_t>(player))) {
        if (padSlot(code) >= 0) {
            return true;
        }
    }
    return false;
}

Code Bindings::get(int player, Action action) const {
    return codes_.at(static_cast<std::size_t>(player)).at(static_cast<std::size_t>(action));
}

void Bindings::set(int player, Action action, Code code) {
    codes_.at(static_cast<std::size_t>(player)).at(static_cast<std::size_t>(action)) = code;
}

}  // namespace bb::core::input
