// Blip'n Blop: parses the options, opens the SDL 2 platform and runs the
// application on it.

#include <cstdio>
#include <memory>
#include <string>

#include "app.hpp"
#include "bb/core/log.hpp"
#include "bb/platform/entry.hpp"
#include "bb/platform/sdl2/sdl2_platform.hpp"
#include "files.hpp"
#include "session.hpp"

int bbMain(int argc, char* argv[]) {
    // The run's state: everything the program keeps lives here, in the
    // objects of this function (no global objects).
    bb::app::Session session;
    if (!session.init(argc, const_cast<const char**>(argv))) {
        return 2;
    }

    bb::platform::sdl2::Options options;
    options.headless = session.headless();

    std::string error;
    std::unique_ptr<bb::platform::Platform> platform = bb::platform::sdl2::createPlatform(options, error);
    if (!platform) {
        std::fprintf(stderr, "blipblop: cannot initialise SDL: %s\n", error.c_str());
        return 1;
    }
    bb::platform::FileSystem& fileSystem = platform->files();
    bb::app::GameFiles files(fileSystem);
    if (!session.load(fileSystem, files)) {
        return 2;
    }

    // Log: everything to a file, warnings and errors also to stderr.
    // Deterministic runs (tests, replays) keep the log in the working
    // directory, next to their other outputs.
    std::string logPath = session.logPath();
    if (logPath.empty()) {
        const std::string preferences = fileSystem.preferencesDirectory();
        logPath =
            (session.deterministic() || preferences.empty() ? std::string() : preferences) + "BlipBlop.log";
    }
    bb::core::FileLogSink logFile(logPath);
    if (!logFile.isOpen()) {
        std::fprintf(stderr, "blipblop: cannot write the log file '%s'\n", logPath.c_str());
    }
    bb::core::ConsoleLogSink console(stderr, bb::core::LogLevel::Warning, "blipblop: ");
    const bb::core::ScopedLogSink fileRegistration(logFile);
    const bb::core::ScopedLogSink consoleRegistration(console);

    files.locateGameData(fileSystem.baseDirectory());

    session.start(platform->clock(), platform->system(), fileSystem);

    // The platform outlives the application.
    bb::app::App app(*platform, session, files);
    return app.run();
}
