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
| `UVE::Threading`  | `engine/threading/`  | `ThreadPoolUVE`, `JobCounterUVE`, `JobUVE`                       |
| `UVE::CommandLine`| `engine/commandline/`| `CommandLineUVE` — startup argument parsing                    |
| `UVE::Config`     | `engine/config/`     | `ConfigManagerUVE` — JSON-based `.uvesettings` key-value store  |
| `UVE::Math`       | `engine/math/`       | `Vector3UVE`, `QuaternionUVE`                                   |
| `UVE::Asset`      | `engine/asset/`      | `AssetGuidUVE`, `AssetDatabaseUVE` — minimal GUID<->path registry |
| `UVE::Scene`      | `engine/scene/`      | `EntityManagerUVE`, `SceneGraphUVE`, `ComponentUVE` + built-ins, `SceneSerializerUVE`, `PrefabSystemUVE` |
| `UVE::Core`       | `engine/core/`       | `EngineCoreUVE`, config, state, frame stats, version, services |

Future systems (Rendering, Physics, Animation, Audio, AI, Networking, Editor, ...) become
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

`ConfigManagerUVE` (`engine/config/`) is the first module with a third-party data-format
dependency (`nlohmann::json`, pulled in via `FetchContent`). It is confined behind a PIMPL
(`ConfigManagerUVE::ImplUVE`, defined only in `config_manager_uve.cpp`) so no header outside
`engine/config/src/` ever includes the JSON library, and `uve_config` links it `PRIVATE` — copy
this pattern for any future module that pulls in a similar third-party dependency.

`EntityManagerUVE` (`engine/scene/`) identifies component types via `std::type_index(typeid(T))`
rather than a new global type-id registry — reusing the exact type-erasure pattern
`IEventSystemUVE` already established (`SubscribeErased`/`PublishErased` taking
`std::type_index`). Copy this pattern for any future system that needs to dispatch by C++ type
at runtime, instead of inventing a parallel dense-integer type-id scheme.

`AssetDatabaseUVE` (`engine/asset/`) and `SceneSerializerUVE` (`engine/scene/src/`) follow
`ConfigManagerUVE`'s exact PIMPL-confinement pattern for `nlohmann::json` — the JSON library
never appears in a public header, only inside each type's own `.cpp` (`AssetDatabaseUVE::ImplUVE`,
or free functions local to `scene_serializer_uve.cpp`), and the owning `CMakeLists.txt` links
`nlohmann_json::nlohmann_json` `PRIVATE`. Copy this pattern for any future module that needs the
JSON library internally.

## The universal `.uve*` binary envelope

Every `.uve*` asset file on disk (`.uvescene`, `.uveprefab`, and any future binary asset format
such as `.uvemodel`/`.uvetex`) shares one binary header, written/read as individual fixed-width
field writes — **never** a single `.write()`/`.read()` of a struct, since C++ struct padding and
alignment are implementation-defined and unsafe to persist:

```
magic:              char[4] = "UVE\0"
version:            uint32   (payload schema version for this asset type)
assetType:          uint32   (meaning is asset-type-specific, e.g. SceneAssetTypeUVE)
compressionMethod:  uint32   (0 = None — the only value implemented so far)
payloadLength:      uint64   (byte length of the payload that follows)
payload:            payloadLength bytes
```

Files are always opened with `std::ios::binary`. A malformed header (bad magic, truncated file,
unsupported version/asset-type/compression method) or a corrupt payload must never crash — log a
detailed `UVE_ERROR` (path + reason) and return a failure value, mirroring `ConfigManagerUVE`'s
error-handling contract. `SceneSerializerUVE` (`engine/scene/src/scene_serializer_uve.cpp`) is
the reference implementation of this envelope; copy its header read/write helpers for any future
`.uve*` binary format instead of reinventing them.

## Allocator boundary

`PoolAllocatorUVE`, `StackAllocatorUVE`, and `HeapAllocatorUVE` (`engine/memory/`) are the
engine's low-level allocation primitives — the intentional, sole place raw aligned-new/
aligned-delete are invoked directly. Everything else builds objects on top of them via
`ConstructUVE<T>()`/`DestroyUVE<T>()` (`allocator_utils_uve.h`), never placement-new or an
explicit destructor call at the call site. Track allocations by requesting an
`IMemoryTrackerUVE*` at construction (e.g. `MemoryManagerUVE::GetDefaultAllocatorUVE()` for the
shared, tracked default) rather than allocating untracked unless there's a specific, documented
reason not to.

`ChunkUVE` (`engine/scene/src/`, never exposed under `engine/scene/include/`) is a second,
narrowly-scoped exception: it type-erasedly placement-constructs/moves/destroys arbitrary
component types inside byte buffers it allocates via `IAllocatorUVE` — the raw buffer itself
still always comes from an allocator, never `new[]`/`delete[]`. This is required because a
chunk is built generically across component types only known at runtime (via
`std::type_index`), which a compile-time-templated `ConstructUVE<T>()` call site can't express;
it is the type-erased analog of what `ConstructUVE`/`DestroyUVE` already do for the
single-known-`T` case, not a loophole for ad hoc placement-new elsewhere.

## Real OS threads

`ThreadPoolUVE` (`engine/threading/`) is the first module that spawns actual `std::thread`s, so
its `CMakeLists.txt` is the first to call `find_package(Threads REQUIRED)` and link
`Threads::Threads` explicitly — copy that pattern for any future module that spawns real OS
threads rather than relying on implicit linkage. The established locking idiom applies here too:
`mutable std::mutex m_mutex;` plus `const std::lock_guard<std::mutex> lock(m_mutex);` at the top
of a critical section; use `std::atomic<T>` only for counters/flags meant to be read or updated
without taking that lock (e.g. debug/query stats).

## General rules

- Modern C++20. RAII everywhere — no raw `new`/`delete` (outside the allocator boundary above).
- No STL containers in hot paths (not applicable yet — no hot paths exist in this increment).
- No stub functions, no TODO comments, no half-implemented classes. Every method delivered does
  something real and correct.
- Every system must be unit-testable and unit-tested (GoogleTest, see `tests/`).
- Build warnings are treated as errors (`-Werror` with a strict flag set — see
  `cmake/UveCompilerWarnings.cmake`); a warning is a build failure, not a suggestion.
