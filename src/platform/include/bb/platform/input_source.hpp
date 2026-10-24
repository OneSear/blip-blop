#pragma once

#include <cstdint>
#include <string>

#include "bb/core/input/codes.hpp"

namespace bb::platform {

struct InputEvent {
    enum class Type {
        KeyDown,  // code: key code (bb::core::input)
        KeyUp,
        PadButtonDown,  // pad: slot, code: button index
        PadButtonUp,
        PadAxis,     // pad: slot, axis: index, value: -32768..32767
        PadHat,      // pad: slot, value: kHat* bits
        PadAdded,    // pad: slot
        PadRemoved,  // pad: slot
        Quit,        // the window was closed or the system asks to quit
        FocusLost,   // the window lost the focus or the application is suspended
        FocusGained,
    };

    static constexpr int kHatUp = 1;
    static constexpr int kHatRight = 2;
    static constexpr int kHatDown = 4;
    static constexpr int kHatLeft = 8;

    Type type = Type::Quit;
    core::input::Code code = 0;
    int pad = -1;
    int axis = 0;
    int value = 0;
};

// Keyboard, joysticks and window events. Joysticks are identified by a
// stable slot (0, 1, ...): a reconnected pad gets the first free slot.
class InputSource {
public:
    virtual ~InputSource() = default;

    // Next pending event; false when there is none.
    virtual bool poll(InputEvent& event) = 0;

    // Number of slots ever used (some may be empty after a disconnection).
    virtual int padSlots() const = 0;
    virtual bool padConnected(int slot) const = 0;
    virtual std::string padName(int slot) const = 0;
    // Whether the pad is a recognised game controller, whose buttons have
    // the numbers of codes.hpp (kPadButtonA...); other pads report their
    // own numbers.
    virtual bool padIsController(int slot) const = 0;
};

}  // namespace bb::platform
