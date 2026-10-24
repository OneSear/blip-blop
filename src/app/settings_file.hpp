#pragma once

#include <string>

#include "bb/game/services.hpp"

namespace bb::core {
class Vfs;
}

namespace bb::app {

// The settings (vsync, the players' keys) as text in the user's preferences
// directory (bb::core::Settings); the 2002 binary bb.cfg is only imported,
// once, when there is no settings file yet (migration.md DEC-12).

// The built-in settings; the vsync option is kept (set_default_config()).
void applyDefaultSettings(game::Options& options, game::Input& input);
// The settings of settingsPath, else those imported from legacyFile (a data
// file), else the defaults (load_BB3_config()).
void loadSettings(const std::string& settingsPath, const core::Vfs& dataFiles, const std::string& legacyFile,
                  game::Options& options, game::Input& input);
// Writes the settings to settingsPath (nothing without one).
void saveSettings(const std::string& settingsPath, const game::Options& options, const game::Input& input);

}  // namespace bb::app
