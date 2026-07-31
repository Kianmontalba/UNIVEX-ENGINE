//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "uve/debug/log_level_uve.h"

namespace UVE::Debug {

/// A single formatted log record, passed to every ILogSinkUVE::Write() call.
/// `sourceFile` points at a string literal (always the __FILE__ expansion
/// at a UVE_LOG-family call site) and is therefore valid for the lifetime
/// of the program without needing to be copied; `sourceLine` is 0 and
/// `sourceFile` is nullptr for messages that did not originate from a
/// source-location-aware macro.
/// Thread-safety: value type. Safe to copy/move freely; holds no shared
/// mutable state.
struct LogMessageUVE {
    LogLevelUVE level = LogLevelUVE::Trace;
    std::string category;
    std::string message;
    const char* sourceFile = nullptr;
    int sourceLine = 0;
    std::chrono::system_clock::time_point timestamp{};
    std::thread::id threadId{};
};

/// ILogSinkUVE is the destination interface for formatted log records.
/// LoggerUVE fans every accepted message out to every registered sink.
/// Thread-safety: implementations must make Write()/Flush() safe to call
/// concurrently from multiple threads, since LoggerUVE may be logged to
/// from any thread.
class ILogSinkUVE {
public:
    virtual ~ILogSinkUVE() = default;

    /// Records one log message. Must not throw.
    virtual void Write(const LogMessageUVE& message) = 0;

    /// Forces any buffered output to be written out immediately.
    virtual void Flush() = 0;
};

/// Writes log messages as human-readable lines to stdout
/// (Trace/Debug/Info) or stderr (Warning/Error/Fatal).
/// Thread-safety: thread-safe. Writes are serialized by an internal mutex
/// so concurrent log calls never interleave mid-line.
class ConsoleSinkUVE final : public ILogSinkUVE {
public:
    void Write(const LogMessageUVE& message) override;
    void Flush() override;

private:
    std::mutex m_mutex;
};

/// Appends log messages as timestamped text lines to a file on disk.
/// Flushes immediately after any Fatal-level message so a crash never loses
/// the last recorded line; other levels rely on normal stream buffering
/// until an explicit Flush() call. If the file cannot be opened, the sink
/// remains functional but silently discards writes (queryable only via
/// UVE_ASSERT firing in debug builds at construction time) — a deliberate,
/// documented trade-off for this increment rather than introducing an
/// exception-based failure path this early in the engine.
/// Thread-safety: thread-safe. Writes are serialized by an internal mutex.
class FileSinkUVE final : public ILogSinkUVE {
public:
    explicit FileSinkUVE(const std::filesystem::path& filePath);
    ~FileSinkUVE() override;

    FileSinkUVE(const FileSinkUVE&) = delete;
    FileSinkUVE& operator=(const FileSinkUVE&) = delete;

    void Write(const LogMessageUVE& message) override;
    void Flush() override;

private:
    std::mutex m_mutex;
    std::ofstream m_file;
};

/// Captures log messages in memory instead of writing them anywhere
/// external. Exists for unit tests (assert against captured messages
/// instead of parsing stdout/files) and as the foundation for a future
/// in-editor console window that needs recent history in memory.
/// Thread-safety: thread-safe. Writes and reads are serialized by an
/// internal mutex; GetMessagesUVE() returns a snapshot copy so callers
/// never observe the message list being mutated concurrently.
class MemorySinkUVE final : public ILogSinkUVE {
public:
    void Write(const LogMessageUVE& message) override;
    void Flush() override;

    /// Returns a copy of every message captured so far, in write order.
    [[nodiscard]] std::vector<LogMessageUVE> GetMessagesUVE() const;

    /// Removes all captured messages.
    void Clear();

private:
    mutable std::mutex m_mutex;
    std::vector<LogMessageUVE> m_messages;
};

} // namespace UVE::Debug
