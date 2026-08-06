// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#if defined(__linux__)
#include <csignal>
#else
#include <cstdlib>
#endif

/// UVE_API marks a symbol as part of the engine's public ABI surface. This
/// increment builds every module as a static library with no shared-library
/// boundary, so UVE_API expands to nothing today. It exists as the seam a
/// future DLL/.so build will use to add dllexport/visibility attributes
/// without touching every call site that already uses this macro.
#define UVE_API

/// UVE_INLINE marks a function as intended for inlining. Kept as a macro
/// (rather than using `inline` directly at call sites) so a future
/// force-inline variant (e.g. `__forceinline` / `__attribute__((always_inline))`)
/// can be swapped in without editing every declaration.
#define UVE_INLINE inline

/// UVE_DEBUG is 1 in debug builds (NDEBUG not defined) and 0 otherwise.
/// Prefer `#if UVE_DEBUG` over `#if !defined(NDEBUG)` in engine code so the
/// debug/release seam always goes through this one macro.
#if !defined(NDEBUG)
#define UVE_DEBUG 1
#else
#define UVE_DEBUG 0
#endif

#if defined(__linux__)
/// UVE_DEBUG_BREAK() raises a trap signal, stopping execution under a
/// debugger (or terminating the process if none is attached) at the exact
/// call site. Linux implementation: SIGTRAP via raise(). This increment only
/// targets Linux; the #else branch below is a complete, working fallback for
/// platforms without a native trap instruction hook, not a placeholder.
#define UVE_DEBUG_BREAK() (void)::raise(SIGTRAP)
#else
/// Fallback UVE_DEBUG_BREAK() for platforms without a native trap hook wired
/// up yet: terminates the process immediately. Complete and correct today;
/// a future Windows/macOS/mobile increment may add a true debugger-trap
/// implementation (e.g. __debugbreak() on MSVC) behind this same macro name.
#define UVE_DEBUG_BREAK() ::std::abort()
#endif
