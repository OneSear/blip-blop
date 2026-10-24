#pragma once

#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>

namespace bb::core {

enum class LogLevel { Info, Warning, Error };

// Where log lines go: a file, the console, a test buffer.
class LogSink {
public:
    virtual ~LogSink() = default;

    // One line, without its end-of-line character.
    virtual void write(LogLevel level, std::string_view line) = 0;
    virtual void flush() {}
};

// The program log (migration.md 3.13). Lines go to every registered sink;
// lines logged while no sink is registered are kept (up to
// kMaxPendingLines) and handed to the first sink added. Not thread-safe:
// the game logs from its main thread only.
class Log {
public:
    static constexpr std::size_t kMaxPendingLines = 1000;

    // The sink must stay alive until it is removed.
    static void addSink(LogSink* sink);
    static void removeSink(LogSink* sink);

    static void write(LogLevel level, std::string_view line);
    static void info(std::string_view line) { write(LogLevel::Info, line); }
    static void warning(std::string_view line) { write(LogLevel::Warning, line); }
    static void error(std::string_view line) { write(LogLevel::Error, line); }

    static void flush();
};

// Registers a sink for the lifetime of the object.
class ScopedLogSink {
public:
    explicit ScopedLogSink(LogSink& sink) : sink_(sink) { Log::addSink(&sink_); }
    ~ScopedLogSink() {
        sink_.flush();
        Log::removeSink(&sink_);
    }
    ScopedLogSink(const ScopedLogSink&) = delete;
    ScopedLogSink& operator=(const ScopedLogSink&) = delete;

private:
    LogSink& sink_;
};

// A text file, written through a large buffer. Warnings and errors are
// flushed at once, so that they survive a crash; info lines reach the disk
// when the buffer fills, on flush() and when the sink is destroyed. After
// maxLines lines, one last line says the log is too long and the rest is
// dropped (the 2002 log stopped after 50,000 messages the same way).
class FileLogSink final : public LogSink {
public:
    static constexpr std::size_t kDefaultMaxLines = 50000;

    explicit FileLogSink(const std::string& path, std::size_t maxLines = kDefaultMaxLines);
    ~FileLogSink() override;
    FileLogSink(const FileLogSink&) = delete;
    FileLogSink& operator=(const FileLogSink&) = delete;

    bool isOpen() const { return file_ != nullptr; }
    const std::string& path() const { return path_; }

    void write(LogLevel level, std::string_view line) override;
    void flush() override;

private:
    std::string path_;
    std::FILE* file_ = nullptr;
    std::size_t lines_ = 0;
    std::size_t maxLines_;
};

// A console stream (stderr): only lines of at least minLevel, prefixed
// with the program name and the level.
class ConsoleLogSink final : public LogSink {
public:
    ConsoleLogSink(std::FILE* stream, LogLevel minLevel, std::string prefix);

    void write(LogLevel level, std::string_view line) override;
    void flush() override;

private:
    std::FILE* stream_;
    LogLevel minLevel_;
    std::string prefix_;
};

}  // namespace bb::core
