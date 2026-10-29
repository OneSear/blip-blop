#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace bb::core {

// The set of physical inputs held down during one frame, expressed in the
// legacy input code space (keyboard key codes and (slot+1)<<10|button
// joystick codes), plus the "window closed" request and whether the window
// has lost the focus (or the application is in the background).
struct InputState {
    std::vector<std::uint32_t> codes;  // sorted, unique
    bool quit = false;
    bool unfocused = false;

    void press(std::uint32_t code);
    bool isPressed(std::uint32_t code) const;
    bool operator==(const InputState& other) const {
        return quit == other.quit && unfocused == other.unfocused && codes == other.codes;
    }
    bool operator!=(const InputState& other) const { return !(*this == other); }
};

// A frame-indexed input recording, used both for recorded play sessions and
// for hand-written test scripts.
//
// Text format (version 1), one statement per line, '#' starts a comment:
//
//   bbreplay 1                  mandatory first statement
//   seed 1                      random seed for the run (default 1)
//   option /cheat               game option applied before start (repeatable)
//   @<frame> [code...] [quit] [unfocused]
//                               input held from <frame> on; codes are numbers
//                               (decimal or 0x hex) or names known to the app
//   end <frame>                 the run stops after this many frames
//
// Changes must be listed in increasing frame order.
class InputTimeline {
public:
    using NameToCode = std::function<std::optional<std::uint32_t>(std::string_view)>;
    using CodeToName = std::function<std::string(std::uint32_t)>;

    static std::optional<InputTimeline> parse(std::string_view text, const NameToCode& resolve,
                                              std::string& error);
    std::string serialize(const CodeToName& name) const;

    // Input in effect at the given frame (empty before the first change).
    const InputState& at(std::uint64_t frame) const;

    // Records the input for a frame; stored only if it differs from the
    // previous state. Frames must be non-decreasing.
    void record(std::uint64_t frame, const InputState& state);

    std::uint32_t seed = 1;
    std::vector<std::string> options;
    std::optional<std::uint64_t> endFrame;

    struct Change {
        std::uint64_t frame;
        InputState state;
    };
    const std::vector<Change>& changes() const { return changes_; }

private:
    std::vector<Change> changes_;
    mutable std::size_t cursor_ = 0;  // speeds up monotonic at() queries
};

}  // namespace bb::core
