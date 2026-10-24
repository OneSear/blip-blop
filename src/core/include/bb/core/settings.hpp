#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "bb/core/input/bindings.hpp"
#include "bb/core/input/names.hpp"

namespace bb::core {

// Player settings, saved as a versioned text file (migration.md 3.13):
//
//   blipblop-settings 1
//   vsync on
//   fullscreen off
//   language english
//   p1.fire key:left_ctrl
//   p2.jump pad1:button3
//   ...
//
// Unknown statements are reported and skipped, so newer files still load.
struct Settings {
    bool vsync = true;
    bool fullscreen = false;
    input::Language language = input::Language::English;
    input::Bindings bindings = input::Bindings::defaults();

    std::string serialize() const;

    // nullopt (and error) when the header is missing or a value is invalid;
    // warnings lists the statements that were skipped.
    static std::optional<Settings> parse(std::string_view text, std::string& error,
                                         std::vector<std::string>& warnings);

    // Imports the 2002 binary bb.cfg (61 bytes: vsync byte, language int32,
    // 14 int32 input codes P1 up/down/left/right/fire/jump/special then P2).
    // A player whose codes are not all known inputs gets the default
    // bindings: the shipped file binds player 2 to DirectInput scan codes
    // (D-67). The language is English: the game forced it (D-25).
    static std::optional<Settings> importLegacyCfg(const std::vector<std::uint8_t>& bytes,
                                                   std::vector<std::string>& notes);
};

}  // namespace bb::core
