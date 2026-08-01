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
| `UVE::Math`       | `engine/math/`       | `Vector2UVE`, `Vector3UVE`, `QuaternionUVE`, `Matrix4x4UVE`, `AabbUVE`, `PlaneUVE`, `FrustumUVE`, `RayUVE` |
| `UVE::Asset`      | `engine/asset/`      | `AssetGuidUVE`, `AssetDatabaseUVE`, `AssetManagerUVE`, `AssetImporterUVE`, `HotReloadUVE`, `AssetBundleUVE`, `FileSystemUVE`, the `.uve*` binary envelope |
| `UVE::Scene`      | `engine/scene/`      | `EntityManagerUVE`, `SceneGraphUVE`, `ComponentUVE` + built-ins, `SceneSerializerUVE`, `PrefabSystemUVE` |
| `UVE::Render`     | `engine/render/`     | `IRenderDeviceUVE`, `NullRenderDeviceUVE`, `ICommandBufferUVE`, `RenderSystemUVE`, `CameraSystemUVE`, resource handles/descriptors |
| `UVE::Physics`    | `engine/physics/`    | `CollisionSystemUVE`, `PhysicsSystemUVE`, `PhysicsMaterialUVE`, `RaycastSystemUVE`, `RaycastQueryUVE`/`RaycastHitUVE` |
| `UVE::Input`      | `engine/input/`      | `InputSystemUVE`, `InputActionUVE`, `InputBindingUVE`, `KeyCodeUVE`, `MouseButtonUVE` |
| `UVE::Audio`      | `engine/audio/`      | `IAudioDeviceUVE`, `NullAudioDeviceUVE`, `AudioSystemUVE`, `AudioSourceSystemUVE`, `AudioAttenuationModelUVE` |
| `UVE::Save`       | `engine/save/`       | `SaveGameSystemUVE`, `CheckpointManagerUVE`, `GameStateMetadataUVE`, the `.uvesave` format |
| `UVE::Window`     | `engine/window/`     | `IWindowManagerUVE`, `WindowManagerUVE` (real GLFW3 backend), `NullWindowManagerUVE`, window events, `MonitorInfoUVE` |
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

`AssetDatabaseUVE` (`engine/asset/`), `SceneSerializerUVE` (`engine/scene/src/`), and
`SaveGameSystemUVE` (`engine/save/src/`) follow `ConfigManagerUVE`'s exact PIMPL-confinement
pattern for `nlohmann::json` — the JSON library never appears in a public header, only inside each
type's own `.cpp` (`AssetDatabaseUVE::ImplUVE`, or free functions local to
`scene_serializer_uve.cpp`/`save_game_system_uve.cpp`), and the owning `CMakeLists.txt` links
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
(`AssetBundleUVE`), raw file bytes for `Blob` (`AssetManagerUVE`'s reference loadable type), or a
**fixed** two-section, length-prefixed metadata-then-world layout for `Save`
(`SaveGameSystemUVE`) — deliberately distinct in shape from `Bundle`'s variable-length,
name-indexed entry table, since a save file always has exactly two sections, never an arbitrary
named set. See "Save/Load (`engine/save/`)" below for the full `.uvesave` layout. Files are always
opened with `std::ios::binary`. A malformed header (bad magic, truncated file,
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

## Physics (`engine/physics/`)

`engine/physics` links `uve_scene` (it needs `IEntityManagerUVE`, `ISceneGraphUVE`, and the
built-in components); `engine/scene` must never link back to `uve_physics` — that would be a
circular module dependency this codebase has never had (compare: `engine/render`/`engine/asset`
depend on `engine/scene`, never the reverse). This has a concrete consequence for component
design: physics-related data that needs to live on an ECS component (`RigidBodyComponentUVE`'s
`velocity`/`drag`/`gravityScale`, `ColliderComponentUVE`'s `friction`/`restitution`/`density`) is
always stored as **plain fields** (`float`, `Math::Vector3UVE`, ...) directly on the
`UVE::Scene`-namespaced component — never as a `UVE::Physics`-namespaced type embedded by value,
since that would force `engine/scene` to include an `engine/physics` header.

When a `UVE::Physics` type is still useful for bundling that data outside the ECS (an internal
combine/read helper, a query result a caller wants handed back to them), the resolving pattern is:
a small, **derived, read-only value type that is constructed on demand from the component's plain
fields and never written back to the ECS**. `PhysicsMaterialUVE` (Increment 16) is the concrete
example — `ColliderComponentUVE.friction`/`.restitution`/`.density` are the actual stored data;
`Physics::PhysicsMaterialUVE` (built via `MaterialOfUVE()`) is a transient snapshot used by
`PhysicsSystemUVE`'s resolution math and returned to raycast callers via `RaycastHitUVE::material`
— never the field type on `ColliderComponentUVE` itself. Reuse this shape for any future type
(in `engine/render`, `engine/audio`, ...) that wants to summarize `engine/scene` data without
`engine/scene` depending back on it.

Iteration logic shared by more than one system in the same module (`CollisionSystemUVE` and
`RaycastSystemUVE` both need "every entity's world-space AABB, cached once") belongs in a small
internal helper under `include/uve/<module>/detail/` (see
`Physics::Detail::BuildColliderWorldAabbCacheUVE`) — extracted the moment a second caller needs
the same loop, not left duplicated. Anything under a `detail/` path is an implementation detail,
not a stable public contract: it may be replaced wholesale (e.g. once a real BVH broad-phase
lands) without that being a breaking change for anything outside the module that owns it.

## Save/Load (`engine/save/`)

`SaveGameSystemUVE` persists an explicit root-entity list (the same "explicit roots" contract
`Scene::ISceneSerializerUVE` already exposes) to a slot-based `.uvesave` file, backed by
`AssetKindUVE::Save`. `engine/save` depends on `engine/scene` and `engine/asset`; the dependency
never runs the other way — `engine/scene`/`engine/asset` must never gain a dependency on
`uve_save`, matching every other module's one-way dependency discipline in this codebase (compare:
`engine/physics`/`engine/render` depend on `engine/scene`, never the reverse).

**`.uvesave` payload layout** — a fixed two-section structure, written inside the standard `.uve*`
envelope with `assetType = AssetKindUVE::Save`:

```
metadataJsonLength: uint32
metadataJson:       metadataJsonLength bytes (UTF-8 JSON — GameStateMetadataUVE)
worldJsonLength:    uint64
worldJson:          worldJsonLength bytes (UTF-8 JSON — the embedded Scene payload)
```

Unlike `Bundle`'s variable-length, name-indexed entry table, this shape is intentionally fixed —
always exactly a metadata section followed by a world section, never an arbitrary named set.

**The scratch-file bounce**: `ISceneSerializerUVE::SaveUVE()`/`LoadUVE()` only take/produce a real
`std::filesystem::path` — there is no in-memory byte-buffer entry point, and its per-component
JSON table is deliberately private to `scene_serializer_uve.cpp` (not part of its public
contract). Rather than growing `ISceneSerializerUVE`'s API surface for one caller,
`SaveGameSystemUVE::SaveUVE()` calls `SceneSerializerUVE::SaveUVE()` against a scratch path, reads
the raw bytes back via `Asset::ReadUveFileUVE()`, deletes the scratch file, and embeds those bytes
verbatim as the world section of its own payload; `LoadUVE()` does the reverse (write the embedded
world bytes to a scratch path, call `SceneSerializerUVE::LoadUVE()`, delete the scratch file).
Copy this pattern — write to scratch, read the bytes back, delete the scratch file — for any
future caller that needs to embed a file-based service's output inside another format without
widening that service's public contract.

**Payload layer isolation**: `BuildSavePayloadUVE()`/`SplitSavePayloadUVE()`
(`save_game_system_uve.cpp`) are the *only* functions that touch the raw metadata+world byte
layout above; every other function in the module (slot path resolution, the scratch-file bounce,
the atomic write) treats the payload as an opaque `std::vector<std::byte>`. This is a deliberate
seam: a future compression or encryption increment can wrap `BuildSavePayloadUVE()`'s output
(and, symmetrically, `SplitSavePayloadUVE()`'s input) without touching `SaveGameSystemUVE`'s
public API or any other internal function. `GameStateMetadataUVE::payloadSchemaVersion` is a
similar forward-looking hook — a dedicated, documented field a future migration system can key
off of, distinct from the four `engineVersion*` fields, without this increment implementing any
actual migration logic.

**Atomic save writes**: `SaveUVE()` writes to a `.uvesave.tmp` staging path in the save directory
via the existing (non-atomic) `WriteUveFileUVE()`, then `std::filesystem::rename()`s it over the
final slot path only after a fully successful write — removing the `.tmp` file on any failure
path. No half-written `.uvesave` file is ever visible at a slot's real path, and a failed save
never leaves a stray temp file behind. This wraps `WriteUveFileUVE()` rather than modifying it —
`Asset::uve_file_envelope_uve.*` itself stays non-atomic for every other caller.

**Reserved auto-save slot**: `kAutoSaveSlotIndexUVE = -1` is deliberately outside
`[0, kSaveSlotCountUVE)`, so `CheckpointManagerUVE`'s writes can never collide with, or be
enumerated alongside, a player's own numbered saves (`ListUsedSlotsUVE()` never returns it).
Auto-save and manual checkpoint currently share this one reserved slot — this is temporary,
foundation-only architecture, flagged with an explicit
`TODO(Increment 19+): Future versions may separate autosave and manual checkpoint into independent
reserved slots.` doc comment on `ICheckpointManagerUVE`. Do not remove that TODO without actually
splitting the slots.

**Explicitly deferred** (`docs/MASTER_SPEC.md` Part 17 items with no buildable path yet, same
"Foundations increment" discipline as every prior module): LZ4 compression
(`compressionMethod` stays `0 = None` — no LZ4 library exists in the repo), AES-256 encryption (no
crypto library exists), cloud sync hooks (not even a Null-backend stub — unlike
`NullAudioDeviceUVE`'s recorded gain/position state, a `NullCloudSyncUVE` would have no
interesting behavior to bookkeep), screenshot thumbnails (needs a real render-to-texture/readback
path, no GPU backend exists), save migration/versioning logic (no old format exists yet to migrate
from — `payloadSchemaVersion` above is the hook a future increment keys off of), and Player
State/Inventory/Quest Progress sections (no gameplay component types exist anywhere in the repo —
"world state" is exactly the generic ECS/component data `SceneSerializerUVE` already round-trips).

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

**`NullRenderDeviceUVE`** performs zero real GPU work: it bookkeeps every resource handle and
validates call correctness (paired render passes, no draw calls outside a pass, no binding to an
unknown handle), and its command buffer is a "spy" that records the exact call sequence a real
backend would have received, for tests to assert against
(`NullRenderDeviceUVE::GetLastSubmittedCommandsUVE()`). Used whenever `EngineConfigUVE::headlessUVE`
is true (or `--headless` is passed) — no display, GPU device node, or graphics SDK is required.

**`GlRenderDeviceUVE`** (`engine/render/`, Increment 20) is the first *real* backend: a genuine
OpenGL 4.6 Core implementation of `IRenderDeviceUVE`, used whenever a real window exists. Unlike
`NullCommandBufferUVE`'s record-then-never-replay spy pattern, `GlCommandBufferUVE` issues actual
GL calls immediately as each method executes — there is no secondary "submit" replay step in
OpenGL's immediate execution model, so `GlRenderDeviceUVE::SubmitUVE()` does no replay work at
all, just asserts the command buffer's dynamic type and releases it. A future Vulkan/Metal/D3D12
backend is still future work; nothing above `IRenderDeviceUVE` needs to change when one arrives —
`GlRenderDeviceUVE` already proves the interface is renderer-agnostic in practice, not just in
theory.

**`IRenderDeviceUVE::PresentUVE()`** is a distinct, explicit step after `SubmitUVE()`, mirroring a
real Vulkan/D3D12 swapchain present call rather than folding presentation into submission.
`GlRenderDeviceUVE::PresentUVE()` delegates to the window manager's `SwapBuffersUVE()`;
`NullRenderDeviceUVE::PresentUVE()` is a bookkeeping no-op with a `GetPresentCallCountUVE()` test
hook — every backend must implement the complete RHI contract, no exceptions.

**`RenderPassDescUVE::colorAttachment == kInvalidTextureHandleUVE`** means "render into the
backend's default framebuffer" (the window's swapchain image) — this reuses the exact
sentinel-handle convention the struct already established for `depthAttachment` ("no depth
attachment") rather than growing the interface with a separate `GetSwapchainColorTargetUVE()`
accessor. `NullRenderDeviceUVE` ignores it like every other field it doesn't act on.

**`PipelineDescUVE::vertexStride`** is the byte distance between consecutive vertices, required by
`glVertexAttribPointer`; `VertexAttributeUVE::offset` alone was insufficient (a real RHI gap
found and fixed during Increment 20, not present in the original design).

**GL symbol confinement**: no public header under `engine/render/include/` or
`engine/window/include/` ever names a GLFW or raw OpenGL type. `IWindowManagerUVE::
GetNativeWindowHandleUVE()` returns type-erased `void*` (matching `ChunkUVE`/`IAssetManagerUVE::
RegisterLoaderUVE<T>`'s established type-erasure precedent), which `GlRenderDeviceUVE.cpp`
`reinterpret_cast`s back to `GLFWwindow*` internally — it already needs the GLFW header itself for
`glfwGetProcAddress`, so this leaks nothing new into `engine/render`'s public surface. Modern GL
entry points (`glCreateShader`, `glGenVertexArrays`, `glBufferData`, ...) are loaded at runtime via
a **hand-rolled minimal function-pointer loader** (`engine/render/src/gl_functions_uve.h/.cpp`,
module-private — never in `include/`): a plain struct of `PFNGL*PROC` members populated via
`glfwGetProcAddress`, chosen over pulling in GLEW/glad because this backend only ever needs the
~30 functions Increment 20 actually uses — the same "only build what the current consumer needs"
discipline as `Matrix4x4UVE`'s minimal API. Legacy GL 1.1 functions (`glViewport`, `glClear`,
`glClearColor`, `glReadPixels`) need no loading — the system's `<GL/gl.h>` already declares them.

**`WindowManagerUVE`/`NullWindowManagerUVE`** extend the `NullRenderDeviceUVE`/`NullAudioDeviceUVE`
Null-backend pairing convention to windowing: `EngineServicesUVE`'s "every service is a live,
non-null reference" invariant needs zero headless-mode exceptions because `NullWindowManagerUVE`
satisfies `IWindowManagerUVE` exactly as faithfully as `WindowManagerUVE`'s real GLFW3 backend
does, just without touching any OS window. `WindowManagerUVE` owns the *entire* GL context
lifecycle — `glfwInit`/`glfwCreateWindow`/`glfwMakeContextCurrent`/`glfwDestroyWindow`/
`glfwTerminate` — never `GlRenderDeviceUVE`, which only ever loads function pointers and issues GL
calls against a context it assumes is already current. `EngineCoreUVE::Shutdown()` must therefore
reset `m_renderDevice` strictly before `m_windowManager` — every GL object dies while the context
is still valid, and the context/GLFW itself is only torn down after.

**The demo triangle** (`EngineCoreUVE::SetupDemoTriangleUVE()`/`RenderDemoTriangleUVE()`) is
explicitly temporary scaffold proving `CreateBuffer → CreateShader → CreatePipeline → Bind/Draw →
Present` end-to-end with hardcoded vertex data and inline GLSL — it deliberately bypasses
`Renderer3DUVE`/`MeshRendererUVE`/asset loading/the ECS entirely and must never grow into a real
content path. The intended long-term rendering roadmap is: `NullRenderDeviceUVE →
GlRenderDeviceUVE (Increment 20) → Renderer3DUVE scene integration → Materials → PBR →
RenderGraph → Editor viewport` — the demo triangle is only the first visible milestone on that
path, not the final architecture, and should be deleted (not extended) once a real
scene-to-window bridge exists.

### Windowing/OpenGL environment prerequisites

`engine/window/CMakeLists.txt` fetches GLFW3 via `FetchContent` (mirroring `nlohmann_json`'s
precedent in `engine/config/CMakeLists.txt` — git-based fetches work even where raw HTTPS to
GitHub doesn't). `engine/render/CMakeLists.txt` is the first `engine/render` consumer of
`find_package(OpenGL REQUIRED)`, mirroring `engine/threading`'s `find_package(Threads REQUIRED)`
precedent (see "Real OS threads" below) — the first module to require a system-provided library
calls `find_package` for it directly. Building GLFW3's own CMake-driven X11 backend, and
resolving this codebase's own `#include <GL/gl.h>`, additionally requires the system's X11/OpenGL
*development* headers already installed (`libgl1-mesa-dev`, `libglfw3-dev`, `libxrandr-dev`,
`libxinerama-dev`, `libxcursor-dev`, `libxi-dev`, `libxkbcommon-dev` on Ubuntu/Debian) — an
environment prerequisite, not something `FetchContent` can provide.

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
