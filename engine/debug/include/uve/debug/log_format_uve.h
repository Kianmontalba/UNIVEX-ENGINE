//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

// UVE_HAS_STD_FORMAT is defined by cmake/UveFormatSupport.cmake based on a
// configure-time check of whether the active compiler/standard-library
// combination fully supports std::format. Every UVE::Debug::FormatLogMessageUVE
// call site is written against the same signature either way — nothing
// outside this header needs to know which backend is active.
#if !defined(UVE_HAS_STD_FORMAT)
#error "UVE_HAS_STD_FORMAT must be defined by the build system (see cmake/UveFormatSupport.cmake)"
#endif

#include <string>
#include <utility>

#if UVE_HAS_STD_FORMAT
#include <format>
#else
#include <fmt/format.h>
#endif

namespace UVE::Debug {

#if UVE_HAS_STD_FORMAT

/// Formats `formatString` with `args` using std::format, with compile-time
/// argument/placeholder validation. Identical signature and behavior to the
/// {fmt}-backed overload compiled when UVE_HAS_STD_FORMAT is 0.
template <typename... TArgs>
[[nodiscard]] std::string FormatLogMessageUVE(std::format_string<TArgs...> formatString, TArgs&&... args) {
    return std::format(formatString, std::forward<TArgs>(args)...);
}

#else

/// Formats `formatString` with `args` using the {fmt} library (fetched
/// automatically because std::format was not available on this
/// compiler/standard-library combination), with compile-time
/// argument/placeholder validation. Identical signature and behavior to the
/// std::format-backed overload compiled when UVE_HAS_STD_FORMAT is 1.
template <typename... TArgs>
[[nodiscard]] std::string FormatLogMessageUVE(fmt::format_string<TArgs...> formatString, TArgs&&... args) {
    return fmt::format(formatString, std::forward<TArgs>(args)...);
}

#endif

} // namespace UVE::Debug
