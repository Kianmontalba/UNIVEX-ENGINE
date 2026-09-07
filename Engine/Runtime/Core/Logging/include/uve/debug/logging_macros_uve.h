// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <string_view>
#include <utility>

#include "uve/debug/log_format_uve.h"
#include "uve/debug/log_level_uve.h"
#include "uve/debug/logger_uve.h"
#include "uve/platform/platform_uve.h"

namespace UVE::Debug::Detail {

/// Formats `formatString`/`args` and routes the result to the currently
/// active LoggerUVE (see LoggerUVE::GetActiveInstanceUVE()), tagged with the
/// given severity level and source location. Safely does nothing if no
/// logger is currently active (before engine Init() or after Shutdown()).
/// Not intended to be called directly — use the UVE_LOG-family macros below.
/// Thread-safety: safe to call from any thread; delegates entirely to
/// LoggerUVE, which is itself thread-safe.
template <typename... TArgs>
UVE_INLINE void LogToActiveInstanceUVE(
    LogLevelUVE level,
    const char* sourceFile,
    int sourceLine,
#if UVE_HAS_STD_FORMAT
    std::format_string<TArgs...> formatString,
#else
    fmt::format_string<TArgs...> formatString,
#endif
    TArgs&&... args) {
    LoggerUVE* const activeLogger = LoggerUVE::GetActiveInstanceUVE();
    if (activeLogger == nullptr) {
        return;
    }
    activeLogger->LogFormatted(level, std::string_view{}, sourceFile, sourceLine,
                                FormatLogMessageUVE(formatString, std::forward<TArgs>(args)...));
}

} // namespace UVE::Debug::Detail

/// Logs a formatted message at an explicit LogLevelUVE. `fmt` is a
/// std::format/{fmt}-style format string (compile-time validated against
/// `...`); prefer the UVE_TRACE/INFO/WARNING/ERROR/FATAL convenience macros
/// below unless the level is only known at runtime as a variable.
#define UVE_LOG(level, fmt, ...) \
    ::UVE::Debug::Detail::LogToActiveInstanceUVE((level), __FILE__, __LINE__, fmt __VA_OPT__(, ) __VA_ARGS__)

/// Logs a formatted message at LogLevelUVE::Trace.
#define UVE_TRACE(fmt, ...) UVE_LOG(::UVE::Debug::LogLevelUVE::Trace, fmt __VA_OPT__(, ) __VA_ARGS__)
/// Logs a formatted message at LogLevelUVE::Info.
#define UVE_INFO(fmt, ...) UVE_LOG(::UVE::Debug::LogLevelUVE::Info, fmt __VA_OPT__(, ) __VA_ARGS__)
/// Logs a formatted message at LogLevelUVE::Warning.
#define UVE_WARNING(fmt, ...) UVE_LOG(::UVE::Debug::LogLevelUVE::Warning, fmt __VA_OPT__(, ) __VA_ARGS__)
/// Logs a formatted message at LogLevelUVE::Error.
#define UVE_ERROR(fmt, ...) UVE_LOG(::UVE::Debug::LogLevelUVE::Error, fmt __VA_OPT__(, ) __VA_ARGS__)
/// Logs a formatted message at LogLevelUVE::Fatal. Fatal messages are
/// guaranteed to be flushed to every sink before LogFormatted() returns.
#define UVE_FATAL(fmt, ...) UVE_LOG(::UVE::Debug::LogLevelUVE::Fatal, fmt __VA_OPT__(, ) __VA_ARGS__)
