# UniVex Engine (UVE) — Coding Standards

This document is the quick-reference derived from `docs/MASTER_SPEC.md`. Read this first;
consult the master spec only when this document doesn't cover something.

## Namespace & folder organization

Everything engine-internal lives under `namespace UVE`, organized into sub-namespaces per
subsystem area — **never** flat inside `UVE` directly:

| Namespace        | Folder              | Contents (this increment)                                   |
|-------------------|----------------------|---------------------------------------------------------------|
| `UVE::Platform`   | `engine/platform/`   | `UVE_API`, `UVE_INLINE`, `UVE_DEBUG`, `UVE_DEBUG_BREAK`        |
| `UVE::Debug`      | `engine/debug/`      | `LoggerUVE`, log sinks, `UVE_ASSERT`, logging macros           |
| `UVE::Utilities`  | `engine/utilities/`  | `TimerUVE`                                                    |
| `UVE::Events`     | `engine/events/`     | `EventSystemUVE`, priorities, subscriptions                    |
| `UVE::Memory`     | `engine/memory/`     | `MemoryManagerUVE`, `PoolAllocatorUVE`, `StackAllocatorUVE`, `HeapAllocatorUVE` |
| `UVE::Core`       | `engine/core/`       | `EngineCoreUVE`, config, state, frame stats, version, services |

Future systems (Rendering, Physics, Animation, Audio, ECS, AI, Networking, Editor, ...) become
sibling namespaces/folders (`UVE::Render` in `engine/render/`, etc.) without restructuring
anything listed above.

## Naming conventions

- Classes/structs/enums/functions/macros that are part of the engine-internal C++ API carry a
  `UVE` suffix: `EngineCoreUVE`, `LoggerUVE`, `Vector3UVE`, `LogLevelUVE`, `InitializeEngineUVE()`.
- Interfaces are prefixed `I` and also carry the suffix: `ILoggerUVE`, `ITimerUVE`,
  `IEventSystemUVE`.
- Macros: `UVE_ASSERT(cond)`, `UVE_LOG(level, fmt, ...)`, `UVE_TRACE/INFO/WARNING/ERROR/FATAL`,
  `UVE_API`, `UVE_INLINE`.
- Files: snake_case with an `_uve` suffix — `logger_uve.h`, `engine_core_uve.cpp`.
- User-facing node names (future `Node3D`, `Mesh3D`, etc.) do **not** get the `UVE` suffix —
  not applicable to this increment, noted here for future consistency.

## Required copyright header

Every `.h`/`.hpp`/`.cpp` file starts with this exact block:

```
//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------
```

## Documentation & thread safety

- Every public API gets an `///` doc comment (XML-style, for future auto-doc generation).
- Every class documents its thread-safety contract in its header doc comment — e.g. "not
  thread-safe, owned by the main-loop thread only" or "thread-safe: guarded by an internal
  mutex."

## Error handling

- `UVE_ASSERT(cond)` — debug-time invariant checks. Logs at Fatal and breaks in debug builds;
  compiles to a warning-clean no-op in release builds. Never use for expected runtime
  conditions (bad user input, missing files) — that's `UVE_LOG` + a real error-handling path.
- `UVE_LOG(level, fmt, ...)` / `UVE_TRACE` / `UVE_INFO` / `UVE_WARNING` / `UVE_ERROR` /
  `UVE_FATAL` — runtime logging, always safe to call (silently no-ops if the engine isn't
  initialized or has already shut down).

## Dependency access — services, not globals

Systems should receive `Logger`/`Timer`/`EventSystem` access through `EngineServicesUVE`
(obtained from `EngineCoreUVE::GetServicesUVE()`), not through ad-hoc global pointers. The
**one** intentional exception is the logger's internal active-instance pointer, which exists
solely so the `UVE_LOG`/`UVE_TRACE`/etc. macros work from anywhere without a service reference
in scope. Do not add further global/static state without a documented reason as strong as that
one.

## Allocator boundary

`PoolAllocatorUVE`, `StackAllocatorUVE`, and `HeapAllocatorUVE` (`engine/memory/`) are the
engine's low-level allocation primitives — the intentional, sole place raw aligned-new/
aligned-delete are invoked directly. Everything else builds objects on top of them via
`ConstructUVE<T>()`/`DestroyUVE<T>()` (`allocator_utils_uve.h`), never placement-new or an
explicit destructor call at the call site. Track allocations by requesting an
`IMemoryTrackerUVE*` at construction (e.g. `MemoryManagerUVE::GetDefaultAllocatorUVE()` for the
shared, tracked default) rather than allocating untracked unless there's a specific, documented
reason not to.

## General rules

- Modern C++20. RAII everywhere — no raw `new`/`delete` (outside the allocator boundary above).
- No STL containers in hot paths (not applicable yet — no hot paths exist in this increment).
- No stub functions, no TODO comments, no half-implemented classes. Every method delivered does
  something real and correct.
- Every system must be unit-testable and unit-tested (GoogleTest, see `tests/`).
- Build warnings are treated as errors (`-Werror` with a strict flag set — see
  `cmake/UveCompilerWarnings.cmake`); a warning is a build failure, not a suggestion.
