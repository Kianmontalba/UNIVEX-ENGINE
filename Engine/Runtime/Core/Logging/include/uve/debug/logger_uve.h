// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "uve/debug/i_logger_uve.h"
#include "uve/debug/log_level_uve.h"
#include "uve/debug/log_sink_uve.h"

namespace UVE::Debug {

/// LoggerUVE is the concrete, engine-standard implementation of ILoggerUVE.
/// Owned by EngineCoreUVE for the lifetime of the engine. While initialized
/// (between Init() and Shutdown()), registers itself as the process-wide
/// "active" logger (see GetActiveInstanceUVE()) so the UVE_LOG/UVE_TRACE/...
/// macros can reach it without a service reference in scope. That static
/// pointer is the single intentional piece of global state in the engine
/// (see docs/CODING_STANDARDS.md) — everything else reaches the logger
/// through EngineServicesUVE.
/// Thread-safety: thread-safe. LogFormatted() and sink registration are
/// serialized by an internal mutex; the min-level threshold and the active-
/// instance pointer use atomics so they can be read cheaply/safely from any
/// thread without taking the sink lock.
class LoggerUVE final : public ILoggerUVE {
public:
    LoggerUVE();
    ~LoggerUVE() override;

    LoggerUVE(const LoggerUVE&) = delete;
    LoggerUVE& operator=(const LoggerUVE&) = delete;

    /// Prepares the logger for use and registers it as the instance
    /// returned by GetActiveInstanceUVE(). Must be called before any
    /// UVE_LOG-family macro is expected to produce output. Not thread-safe
    /// with respect to concurrent Init()/Shutdown() calls on the same
    /// instance; intended to be called once, during single-threaded engine
    /// startup.
    void Init(LogLevelUVE minLevel) override;

    /// Flushes and releases every sink, then clears this instance's
    /// active-instance registration (if it was the active one). After this
    /// call, UVE_LOG-family macros safely no-op until a logger is Init()
    /// again. Not thread-safe with respect to concurrent LogFormatted()
    /// calls in flight on other threads; intended to be called once, during
    /// single-threaded engine shutdown after all other systems have
    /// stopped logging.
    void Shutdown() override;

    void AddSink(std::unique_ptr<ILogSinkUVE> sink) override;
    void SetMinLevel(LogLevelUVE level) override;
    [[nodiscard]] LogLevelUVE GetMinLevel() const override;
    void LogFormatted(LogLevelUVE level,
                       std::string_view category,
                       const char* sourceFile,
                       int sourceLine,
                       std::string formattedMessage) override;

    /// Returns the logger currently registered via Init(), or nullptr if no
    /// logger is initialized (before the first Init() call, or after
    /// Shutdown()). Safe to call from any thread. Used internally by the
    /// UVE_LOG-family macros.
    [[nodiscard]] static LoggerUVE* GetActiveInstanceUVE() noexcept;

private:
    void FlushAllSinksLocked();

    std::mutex m_sinkMutex;
    std::vector<std::unique_ptr<ILogSinkUVE>> m_sinks;
    std::atomic<LogLevelUVE> m_minLevel{LogLevelUVE::Trace};

    static std::atomic<LoggerUVE*> s_activeInstance;
};

} // namespace UVE::Debug
