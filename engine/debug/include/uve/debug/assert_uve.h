//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include "uve/debug/logging_macros_uve.h"
#include "uve/platform/platform_uve.h"

/// UVE_ASSERT(cond) checks a debug-time invariant — a condition that
/// indicates a programming error if false, never an expected runtime
/// condition from external input (use UVE_ERROR + a real error-handling
/// path for those instead).
///
/// Debug builds (UVE_DEBUG == 1): if `cond` is false, logs a Fatal message
/// naming the failed expression and call site via UVE_FATAL (which is
/// guaranteed to flush every sink before returning), then calls
/// UVE_DEBUG_BREAK() to halt execution at the failure point.
///
/// Release builds (UVE_DEBUG == 0): expands to a warning-clean no-op that
/// evaluates only the *type* of `cond` (via sizeof), never its value, so
/// asserted expressions are never executed in release builds and call sites
/// never need `#if UVE_DEBUG` guards to avoid an unused-value warning.
#if UVE_DEBUG
#define UVE_ASSERT(cond)                                  \
    do {                                                  \
        if (!(cond)) {                                    \
            UVE_FATAL("Assertion failed: {}", #cond);      \
            UVE_DEBUG_BREAK();                             \
        }                                                  \
    } while (false)
#else
#define UVE_ASSERT(cond) \
    do {                 \
        (void)sizeof(cond); \
    } while (false)
#endif
