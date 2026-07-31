//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <string_view>

namespace UVE::Debug {

/// Severity level of a log message, from least to most severe. Values are
/// ordered so `level >= threshold` comparisons work as expected.
enum class LogLevelUVE : std::uint8_t {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warning = 3,
    Error = 4,
    Fatal = 5,
};

/// Returns the fixed-width display name for a log level (e.g. "WARNING").
/// Returns "UNKNOWN" for a value outside the declared enumerators (only
/// reachable via an invalid static_cast from an integer, never from normal
/// engine use).
[[nodiscard]] constexpr std::string_view ToStringUVE(LogLevelUVE level) noexcept {
    switch (level) {
        case LogLevelUVE::Trace:
            return "TRACE";
        case LogLevelUVE::Debug:
            return "DEBUG";
        case LogLevelUVE::Info:
            return "INFO";
        case LogLevelUVE::Warning:
            return "WARNING";
        case LogLevelUVE::Error:
            return "ERROR";
        case LogLevelUVE::Fatal:
            return "FATAL";
    }
    return "UNKNOWN";
}

} // namespace UVE::Debug
