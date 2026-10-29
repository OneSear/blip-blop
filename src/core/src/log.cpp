#include "bb/core/log.hpp"

#include <algorithm>
#include <deque>
#include <utility>
#include <vector>

namespace bb::core {

namespace {

struct LogState {
    std::vector<LogSink*> sinks;
    std::deque<std::pair<LogLevel, std::string>> pending;
};

// Never destroyed: objects with static storage may log from their
// destructors, after other statics are gone.
LogState& state() {
    static LogState* s = new LogState;
    return *s;
}

const char* levelName(LogLevel level) {
    switch (level) {
        case LogLevel::Info:
            return "info";
        case LogLevel::Warning:
            return "warning";
        case LogLevel::Error:
            return "error";
    }
    return "?";
}

}  // namespace

void Log::addSink(LogSink* sink) {
    LogState& s = state();
    if (sink == nullptr || std::find(s.sinks.begin(), s.sinks.end(), sink) != s.sinks.end()) {
        return;
    }
    s.sinks.push_back(sink);
    if (s.sinks.size() == 1) {
        for (const auto& [level, line] : s.pending) {
            sink->write(level, line);
        }
        s.pending.clear();
    }
}

void Log::removeSink(LogSink* sink) {
    LogState& s = state();
    s.sinks.erase(std::remove(s.sinks.begin(), s.sinks.end(), sink), s.sinks.end());
}

void Log::write(LogLevel level, std::string_view line) {
    LogState& s = state();
    if (s.sinks.empty()) {
        if (s.pending.size() == kMaxPendingLines) {
            s.pending.pop_front();
        }
        s.pending.emplace_back(level, std::string(line));
        return;
    }
    for (LogSink* sink : s.sinks) {
        sink->write(level, line);
    }
}

void Log::flush() {
    for (LogSink* sink : state().sinks) {
        sink->flush();
    }
}

FileLogSink::FileLogSink(const std::string& path, std::size_t maxLines) : path_(path), maxLines_(maxLines) {
    file_ = std::fopen(path.c_str(), "w");
    if (file_ != nullptr) {
        std::setvbuf(file_, nullptr, _IOFBF, std::size_t{64} * 1024);
    }
}

FileLogSink::~FileLogSink() {
    if (file_ != nullptr) {
        std::fclose(file_);
    }
}

void FileLogSink::write(LogLevel level, std::string_view line) {
    if (file_ == nullptr || lines_ > maxLines_) {
        return;
    }
    ++lines_;
    if (lines_ > maxLines_) {
        std::fputs("Log file is too long.\n", file_);
        std::fflush(file_);
        return;
    }
    if (level != LogLevel::Info) {
        std::fprintf(file_, "%s: ", levelName(level));
    }
    std::fwrite(line.data(), 1, line.size(), file_);
    std::fputc('\n', file_);
    if (level != LogLevel::Info) {
        std::fflush(file_);
    }
}

void FileLogSink::flush() {
    if (file_ != nullptr) {
        std::fflush(file_);
    }
}

ConsoleLogSink::ConsoleLogSink(std::FILE* stream, LogLevel minLevel, std::string prefix)
    : stream_(stream), minLevel_(minLevel), prefix_(std::move(prefix)) {}

void ConsoleLogSink::write(LogLevel level, std::string_view line) {
    if (stream_ == nullptr || static_cast<int>(level) < static_cast<int>(minLevel_)) {
        return;
    }
    std::fprintf(stream_, "%s%s: %.*s\n", prefix_.c_str(), levelName(level), static_cast<int>(line.size()),
                 line.data());
}

void ConsoleLogSink::flush() {
    if (stream_ != nullptr) {
        std::fflush(stream_);
    }
}

}  // namespace bb::core
