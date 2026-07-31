//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "uve/debug/log_level_uve.h"
#include "uve/debug/log_sink_uve.h"

namespace UVE::Debug {

/// ILoggerUVE is the interface for the engine's logging façade. Concrete
/// implementations own zero or more ILogSinkUVE destinations and route
/// accepted messages (level >= GetMinLevel()) to every one of them. Exists
/// so future builds (dedicated server, editor, unit tests) can substitute a
/// different logger implementation without changing any call site that
/// consumes an ILoggerUVE&.
/// Thread-safety: implementations must support LogFormatted() being called
/// concurrently from multiple threads; AddSink()/SetMinLevel() are expected
/// to be called only during single-threaded setup.
class ILoggerUVE {
public:
    virtual ~ILoggerUVE() = default;

    /// Prepares the logger for use. Must be called before LogFormatted() is
    /// expected to produce output. Implementations are not required to
    /// support calling Init() more than once without an intervening
    /// Shutdown().
    virtual void Init(LogLevelUVE minLevel) = 0;

    /// Flushes and releases every sink and returns the logger to an
    /// uninitialized state. Safe to call even if Init() was never called.
    virtual void Shutdown() = 0;

    /// Registers a sink that will receive every future accepted message.
    /// Takes ownership of the sink.
    virtual void AddSink(std::unique_ptr<ILogSinkUVE> sink) = 0;

    /// Sets the minimum level a message must have to be forwarded to sinks.
    virtual void SetMinLevel(LogLevelUVE level) = 0;

    /// Returns the current minimum level.
    [[nodiscard]] virtual LogLevelUVE GetMinLevel() const = 0;

    /// Formats and routes one log message to every registered sink whose
    /// level threshold accepts it. Called by the UVE_LOG-family macros; not
    /// typically called directly. `category` may be empty. `sourceFile` may
    /// be nullptr if the caller has no source-location information.
    virtual void LogFormatted(LogLevelUVE level,
                               std::string_view category,
                               const char* sourceFile,
                               int sourceLine,
                               std::string formattedMessage) = 0;
};

} // namespace UVE::Debug
