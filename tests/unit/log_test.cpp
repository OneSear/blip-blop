#include <doctest.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "bb/core/log.hpp"

using bb::core::Log;
using bb::core::LogLevel;

namespace {

struct MemorySink final : bb::core::LogSink {
    std::vector<std::pair<LogLevel, std::string>> lines;
    int flushes = 0;
    void write(LogLevel level, std::string_view line) override { lines.emplace_back(level, std::string(line)); }
    void flush() override { ++flushes; }
};

std::string readAll(const std::string& path) {
    std::ifstream in(path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

}  // namespace

TEST_CASE("log lines reach every sink") {
    MemorySink a;
    MemorySink b;
    {
        const bb::core::ScopedLogSink ra(a);
        const bb::core::ScopedLogSink rb(b);
        Log::info("one");
        Log::warning("two");
        Log::error("three");
        Log::flush();
    }
    REQUIRE(a.lines.size() == 3);
    CHECK(a.lines[0] == std::make_pair(LogLevel::Info, std::string("one")));
    CHECK(a.lines[1] == std::make_pair(LogLevel::Warning, std::string("two")));
    CHECK(a.lines[2] == std::make_pair(LogLevel::Error, std::string("three")));
    CHECK(b.lines == a.lines);
    CHECK(a.flushes == 2);  // Log::flush() and the scoped registration's end

    Log::info("after");  // no sink: kept for the next one
    MemorySink c;
    const bb::core::ScopedLogSink rc(c);
    CHECK(a.lines.size() == 3);
    REQUIRE(c.lines.size() == 1);
    CHECK(c.lines[0].second == "after");
}

TEST_CASE("lines logged before any sink are kept up to a limit") {
    for (std::size_t i = 0; i < Log::kMaxPendingLines + 5; ++i) {
        Log::info(std::to_string(i));
    }
    MemorySink sink;
    const bb::core::ScopedLogSink registration(sink);
    REQUIRE(sink.lines.size() == Log::kMaxPendingLines);
    CHECK(sink.lines.front().second == "5");  // the oldest are dropped
    CHECK(sink.lines.back().second == std::to_string(Log::kMaxPendingLines + 4));
}

TEST_CASE("the file sink writes levels, flushes and stops at its limit") {
    const std::string path = "bb_log_test.log";
    {
        bb::core::FileLogSink file(path, 3);
        REQUIRE(file.isOpen());
        file.write(LogLevel::Info, "info line");
        file.write(LogLevel::Warning, "careful");
        // A warning is flushed at once: it is on disk before the sink closes.
        CHECK(readAll(path) == "info line\nwarning: careful\n");
        file.write(LogLevel::Error, "broken");
        file.write(LogLevel::Info, "dropped");
        file.write(LogLevel::Info, "dropped too");
    }
    CHECK(readAll(path) == "info line\nwarning: careful\nerror: broken\nLog file is too long.\n");
    std::remove(path.c_str());
}

TEST_CASE("the console sink filters by level") {
    const std::string path = "bb_console_test.log";
    std::FILE* stream = std::fopen(path.c_str(), "w");
    REQUIRE(stream != nullptr);
    bb::core::ConsoleLogSink console(stream, LogLevel::Warning, "blipblop: ");
    console.write(LogLevel::Info, "quiet");
    console.write(LogLevel::Warning, "shown");
    console.write(LogLevel::Error, "shown too");
    std::fclose(stream);
    CHECK(readAll(path) == "blipblop: warning: shown\nblipblop: error: shown too\n");
    std::remove(path.c_str());
}
