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
| `UVE::Math`       | `engine/math/`       | `Vector3UVE`, `QuaternionUVE`, `Matrix4x4UVE`, `AabbUVE`, `PlaneUVE`, `FrustumUVE` |
| `UVE::Asset`      | `engine/asset/`      | `AssetGuidUVE`, `AssetDatabaseUVE`, `AssetManagerUVE`, `AssetImporterUVE`, `HotReloadUVE`, `AssetBundleUVE`, `FileSystemUVE`, the `.uve*` binary envelope |
| `UVE::Scene`      | `engine/scene/`      | `EntityManagerUVE`, `SceneGraphUVE`, `ComponentUVE` + built-ins, `SceneSerializerUVE`, `PrefabSystemUVE` |
| `UVE::Render`     | `engine/render/`     | `IRenderDeviceUVE`, `NullRenderDeviceUVE`, `ICommandBufferUVE`, `RenderSystemUVE`, `CameraSystemUVE`, resource handles/descriptors |
| `UVE::Core`       | `engine/core/`       | `EngineCoreUVE`, config, state, frame stats, version, services |

Future systems (Physics, Animation, Audio, AI, Networking, Editor, ...) become sibling
namespaces/folders without restructuring anything listed above — exactly how `UVE::Render` in
`engine/render/` was added.

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

Every `.uve*` asset file on disk (`.uvescene`, `.uveprefab`, `.uvebundle`, and any future binary
asset format such as `.uvemodel`/`.uvetex`) shares one binary header, written/read as individual
fixed-width field writes — **never** a single `.write()`/`.read()` of a struct, since C++ struct
padding and alignment are implementation-defined and unsafe to persist:

```
magic:              char[4] = "UVE\0"
version:            uint32   (envelope schema version, currently 1)
assetType:          uint32   (Asset::AssetKindUVE — one shared numbering across every .uve* kind)
compressionMethod:  uint32   (0 = None — the only value implemented so far)
payloadLength:      uint64   (byte length of the payload that follows)
payload:            payloadLength bytes
```

The payload's own shape is asset-kind-specific and entirely up to the caller — UTF-8 JSON text
for `Scene`/`Prefab` (`SceneSerializerUVE`), a binary directory-plus-blob table for `Bundle`
(`AssetBundleUVE`), or raw file bytes for `Blob` (`AssetManagerUVE`'s reference loadable type).
Files are always opened with `std::ios::binary`. A malformed header (bad magic, truncated file,
unsupported version/compression method) or a corrupt payload must never crash — log a detailed
`UVE_ERROR` (path + reason) and return a failure value, mirroring `ConfigManagerUVE`'s
error-handling contract. `WriteUveFileUVE`/`ReadUveFileUVE`
(`engine/asset/include/uve/asset/uve_file_envelope_uve.h`,
`engine/asset/src/uve_file_envelope_uve.cpp`) are the one shared implementation of this envelope
— every consumer (`SceneSerializerUVE`, `AssetBundleUVE`, and any future binary asset format)
calls them rather than reimplementing header read/write logic.

## Deferred garbage collection for ref-counted resources

`AssetManagerUVE` (`engine/asset/`) is the engine's first ref-counted resource system:
`AssetHandleUVE<T>` releasing its reference only decrements a count — the actual unload (running
the registered destroy function) happens in `CollectGarbageUVE()`, called once per frame from
`EngineCoreUVE::Update()` (the same per-frame slot as `SceneGraphUVE::UpdateUVE()`/
`IEventSystemUVE::DispatchQueuedUVE()`). This avoids load/unload thrashing when a refcount briefly
bounces to zero and back within a single frame. Copy this deferred-collection pattern for any
future ref-counted resource system instead of unloading synchronously the instant a refcount
reaches zero.

## Matrix convention (`Matrix4x4UVE`)

`Matrix4x4UVE` (`engine/math/`) is row-major storage (`m[row][col]`) under a **column-vector**
convention: a point transforms as `M * [x, y, z, 1]^T`, and composing `lhs * rhs` means "apply
`rhs` first, then `lhs`" — so a view-projection matrix is `PerspectiveUVE(...) *
ViewFromPositionAndRotationUVE(...)`. `PerspectiveUVE` targets Vulkan's `[0, 1]` depth range but
Y-up NDC (matching this engine's Y-up convention everywhere else, e.g.
`WorldTransformComponentUVE`) rather than Vulkan's native Y-down NDC — reconciling that (negating
a row, or a negative-height viewport) is deferred to whichever future increment implements a real
Vulkan backend; it has no effect on any CPU-side math today. There is deliberately no generic 4x4
`InverseUVE()` — the one place an inverse is conceptually needed (world-to-view) has a cheaper
closed form via `ViewFromPositionAndRotationUVE`, since a camera is never non-uniformly scaled in
practice. Follow the same "only what the current consumer needs, not a general-purpose library"
discipline `Vector3UVE`/`QuaternionUVE` already established for any future addition to this
matrix type.

`AabbUVE`/`PlaneUVE`/`FrustumUVE` (also `engine/math/`) are the minimal culling primitives Part
7.2's `CameraSystemUVE`/`MeshRendererUVE` need: an axis-aligned box, a `normal . point + distance
= 0` plane, and 6 planes extracted from a view-projection matrix via the standard Gribb-Hartmann
technique. They live in `engine/math`, not `engine/render`, specifically so a future
`PhysicsSystemUVE` broad-phase can reuse them without depending on rendering at all.

## Virtual paths (`IFileSystemUVE`)

`FileSystemUVE` (`engine/asset/`) resolves a **virtual path** — a forward-slash-separated
string with no leading slash (e.g. `"textures/rock.uvetex"`) — against zero or more mounts (a
real directory, or a `.uvebundle` archive), searched in descending-priority order. Mount
prefixes use the same shape (`""` = root). A virtual path matches a mount iff it equals the
prefix or starts with `prefix + "/"` — matching is always on whole path segments, never a naive
substring check (a mount at `"tex"` must never match `"textures/rock.uvetex"`). This lets a
loose directory mount transparently override (or be overridden by) a bundle mount at the same or
a covering prefix, purely by priority — the classic PAK-file/mod-override pattern. Copy this
convention (forward-slash-only strings, segment-boundary prefix matching, descending-priority
mount search) for any future code that resolves paths against more than one possible source.
`AssetBundleEntryUVE::virtualName` (`engine/asset/include/uve/asset/i_asset_bundle_uve.h`) is the
bridge between `AssetBundleUVE`'s GUID-centric world (every entry always has a GUID) and
`FileSystemUVE`'s path-centric one (a bundle-backed mount looks entries up by `virtualName`,
falling back to the GUID's hex string if an entry has none — so older code that never sets
`virtualName` keeps working unchanged).

`FileSystemUVE` is deliberately standalone this increment — `AssetDatabaseUVE`,
`uve_file_envelope_uve`, `AssetImporterUVE`, and `SceneSerializerUVE` are all unaffected and still
take a raw `std::filesystem::path` directly; only `AssetBundleUVE` gained the
`HasEntryUVE()`/`ReadEntryUVE()` primitives `FileSystemUVE`'s bundle-backed mounts need. Wider
adoption (e.g. routing `WriteUveFileUVE`/`ReadUveFileUVE` through the VFS) is future-increment
work once a concrete consumer needs it.

## The render hardware interface (`engine/render/`)

`IRenderDeviceUVE` is a **modern explicit RHI** (pipeline state objects, explicit render passes,
resources created/destroyed by handle) mirroring Vulkan/D3D12/Metal directly, paired with a
**retained** `ICommandBufferUVE`: a caller records a sequence of calls into it, then submits it as
a batch via `IRenderDeviceUVE::SubmitUVE()`, rather than issuing draw calls immediately as the
scene is walked. Both were explicit architecture decisions (not spec-derived — the master spec's
own intro flags "immediate or retained command buffers?" as a question to ask, not guess).

**Resource handles are small wrapper structs, not bare integer aliases** —
`BufferHandleUVE`/`TextureHandleUVE`/`ShaderHandleUVE`/`PipelineHandleUVE` each wrap a single
`std::uint32_t value`, exactly like `AssetGuidUVE`. This differs from `IFileSystemUVE`'s
`MountHandleUVE` (a bare alias) because the RHI has four distinct resource kinds where mixing one
up for another is a real risk; `MountHandleUVE` only ever has one kind of handle in play. Follow
whichever precedent fits: a bare alias when there's only one handle kind in a system, a wrapper
struct when there are several that must never be confused.

**`NullRenderDeviceUVE` is the only backend this codebase can build and test today** — there is no
display server, GPU device node, or graphics SDK (Vulkan/GL headers, GLFW3/SDL3, a shader
compiler) available in this environment (confirmed the same way `WindowManagerUVE`'s infeasibility
was confirmed for Increment 8). It performs zero real GPU work: it bookkeeps every resource
handle and validates call correctness (paired render passes, no draw calls outside a pass, no
binding to an unknown handle), and its command buffer is a "spy" that records the exact call
sequence a real backend would have received, for tests to assert against
(`NullRenderDeviceUVE::GetLastSubmittedCommandsUVE()`). A real Vulkan/Metal/D3D12 backend is
future work once this environment has the SDK headers, GPU, and windowing it currently lacks —
nothing above `IRenderDeviceUVE` needs to change when one arrives.

**Module-private vs. public concrete classes**: `NullRenderDeviceUVE` is public
(`engine/render/include/`) because `EngineCoreUVE` constructs it directly, matching
`FileSystemUVE`/`AssetBundleUVE`'s precedent for concrete classes `EngineCoreUVE` names by type.
`NullCommandBufferUVE`, by contrast, is module-private (`engine/render/src/`, never in
`include/`) because nothing outside `engine/render/src/null_render_device_uve.cpp` ever names it
— callers only ever see the `ICommandBufferUVE&` returned from `CreateCommandBufferUVE()`,
matching `ArchetypeUVE`/`ChunkUVE`'s precedent for types that are genuinely internal to one
translation unit. `RecordedCommandUVE` (the tagged-union type describing one recorded call) is
public even though `NullCommandBufferUVE` isn't, because test code needs to name it to assert
against `GetLastSubmittedCommandsUVE()`'s result — a data type consumed externally is public even
when the class that produces it stays private, the same way `Debug::LogMessageUVE` is public
while `MemorySinkUVE`'s own internals aren't inspected directly.

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

`IAssetManagerUVE::RegisterLoaderUVE<T>()` (`engine/asset/include/uve/asset/i_asset_manager_uve.h`)
is a third, similarly narrow exception: it wraps the caller's typed loader in a closure that does
a plain `new T()`/`delete static_cast<T*>(ptr)` so a loaded asset's lifetime can be tracked
type-erased (`void*` + a destroy closure) inside `AssetManagerUVE`'s GUID-keyed record map, the
same way `ChunkUVE` type-erases component storage for the ECS. Not routed through
`IAllocatorUVE`/`ConstructUVE` because asset objects are individually heap-allocated,
long-lived, and never packed into a shared buffer the way components/pool slots are — there is
no allocator-boundary benefit to gain here, only the type-erasure need `ChunkUVE` already
established the precedent for.

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
