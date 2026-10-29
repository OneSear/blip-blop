// The settings file (config.cpp of the legacy port).

#include "settings_file.hpp"

#include <optional>
#include <vector>

#include "bb/core/log.hpp"
#include "bb/core/settings.hpp"
#include "bb/core/vfs.hpp"
#include "bb/platform/file_system.hpp"
#include "files.hpp"

namespace bb::app {

namespace {

namespace input = core::input;

// The alias of a player's action: from 0 for the first player, from 10 for
// the second, in the order of input::kActions.
int aliasOf(int player, int action) {
    return (player == 0 ? game::AliasControls::kFirstPlayer : game::AliasControls::kSecondPlayer) + action;
}

void apply(const core::Settings& settings, game::Options& options, game::Input& in) {
    options.vsync = settings.vsync;
    options.fullscreen = settings.fullscreen;
    options.language = settings.language;
    options.integerScaling = settings.integerScaling;
    options.smoothFilter = settings.smoothFilter;
    options.smoothMotion = settings.smoothMotion;
    for (int p = 0; p < input::kPlayerCount; ++p) {
        for (int a = 0; a < input::kActionCount; ++a) {
            in.setAlias(aliasOf(p, a),
                        settings.bindings.get(p, input::kActions[static_cast<std::size_t>(a)]));
        }
    }
}

core::Settings current(const game::Options& options, const game::Input& in) {
    core::Settings settings;
    settings.vsync = options.vsync;
    settings.fullscreen = options.fullscreen;
    settings.language = options.language;
    settings.integerScaling = options.integerScaling;
    settings.smoothFilter = options.smoothFilter;
    settings.smoothMotion = options.smoothMotion;
    for (int p = 0; p < input::kPlayerCount; ++p) {
        for (int a = 0; a < input::kActionCount; ++a) {
            settings.bindings.set(p, input::kActions[static_cast<std::size_t>(a)],
                                  static_cast<input::Code>(in.alias(aliasOf(p, a))));
        }
    }
    return settings;
}

}  // namespace

void applyDefaultSettings(game::Options& options, game::Input& in) {
    core::Settings defaults;
    defaults.vsync = options.vsync;
    apply(defaults, options, in);
}

void loadSettings(const std::string& settingsPath, const platform::FileSystem& fileSystem, const core::Vfs& dataFiles,
                  const std::string& legacyFile, game::Options& options, game::Input& in) {
    applyDefaultSettings(options, in);

    const auto bytes = settingsPath.empty() ? std::nullopt : fileSystem.readAll(settingsPath);
    if (bytes) {
        const std::string text(bytes->begin(), bytes->end());
        std::string error;
        std::vector<std::string> warnings;
        const auto settings = core::Settings::parse(text, error, warnings);
        for (const std::string& w : warnings) {
            core::Log::info(settingsPath + ": " + w);
        }
        if (settings) {
            core::Log::info("Using " + settingsPath + " as settings file.");
            apply(*settings, options, in);
        } else {
            core::Log::info(settingsPath + ": " + error + " - default settings used");
        }
    } else if (const auto legacy = dataFiles.readAll(legacyFile)) {
        std::vector<std::string> notes;
        const auto settings = core::Settings::importLegacyCfg(*legacy, notes);
        for (const std::string& n : notes) {
            core::Log::info(legacyFile + ": " + n);
        }
        if (settings) {
            core::Log::info("Imported " + legacyFile + " (2002 binary configuration).");
            apply(*settings, options, in);
        }
    } else {
        core::Log::info("Cannot find a settings file. Will use default settings.");
    }
}

void saveSettings(const std::string& settingsPath, platform::FileSystem& fileSystem, const game::Options& options,
                  const game::Input& in) {
    if (settingsPath.empty()) {
        core::Log::info("No settings directory: settings not saved.");
        return;
    }
    const std::string text = current(options, in).serialize();
    if (fileSystem.writeAll(settingsPath, text.data(), text.size())) {
        core::Log::info("Saving " + settingsPath + " as settings file.");
    } else {
        core::Log::warning("Cannot save " + settingsPath);
    }
}

}  // namespace bb::app
