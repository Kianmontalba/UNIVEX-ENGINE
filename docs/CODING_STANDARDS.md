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
| `UVE::Render`     | `engine/render/`     | `IRenderDeviceUVE`, `NullRenderDeviceUVE`, `GlRenderDeviceUVE`, `ICommandBufferUVE`, `RenderSystemUVE`, `CameraSystemUVE`, resource handles/descriptors |
| `UVE::Render::Shader` | `engine/render/shader/` | `IShaderManagerUVE`, `ShaderManagerUVE`, `ShaderSourceUVE`, `ShaderProgramUVE`, built-in `.glsl` shaders |
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

## Copyright headers

Use this short header in every source file outside `engine/core`:

```
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
```

Files in `engine/core` use the complete proprietary notice. Keep it undecorated: no separator lines or triple-slash comments.

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
error-handling contract. `EncodeUveFileEnvelopeUVE`/`DecodeUveFileEnvelopeUVE` are the authoritative
in-memory byte layout and validation seam; `WriteUveFileUVE`/`ReadUveFileUVE`
(`engine/asset/include/uve/asset/uve_file_envelope_uve.h`,
`engine/asset/src/uve_file_envelope_uve.cpp`) delegate to it for filesystem persistence. Every
consumer (`SceneSerializerUVE`, `AssetBundleUVE`, and any future binary asset format) must reuse
this envelope contract rather than reimplementing header read/write logic.

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

**The save-game scratch-file bounce remains intentionally local.** `ISceneSerializerUVE` now also
exposes `CaptureUVE()`/`RestoreUVE()` for editor-owned in-memory subtree history, while keeping its
per-component JSON table private to `scene_serializer_uve.cpp`. `SaveGameSystemUVE` continues to
call `SceneSerializerUVE::SaveUVE()` against a scratch path, read the raw bytes via
`Asset::ReadUveFileUVE()`, delete the scratch file, and embed those bytes verbatim as the world
section; `LoadUVE()` performs the reverse. That retained path preserves the established save-file
format and keeps save-game embedding independent from editor command snapshots. Future callers
must choose the smallest fitting boundary: `CaptureUVE()`/`RestoreUVE()` for transient in-memory
scene-subtree replay, or an explicitly managed scratch file when embedding an already-file-shaped
format.

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

**Increment 21 RHI extensions** (all following the same real-in-Gl/no-op-in-Null pattern as every
prior RHI addition): `IRenderDeviceUVE::GetPipelineUniformsUVE()` reflects a linked pipeline's
active uniforms (name/type/location/array size) after link — `GlRenderDeviceUVE` populates this via
`glGetActiveUniform`/`glGetUniformLocation` right after a successful `CreatePipelineUVE()`/
`CreatePipelineFromBinaryUVE()`; `NullRenderDeviceUVE` returns an empty list (there is nothing to
reflect against). `IRenderDeviceUVE::GetPipelineBinaryUVE()`/`CreatePipelineFromBinaryUVE()` are
the program-binary cache's only GL-facing surface — `NullRenderDeviceUVE`'s versions return
`false`/a synthetic always-valid handle respectively, so `ShaderManagerUVE` never needs a
Null-specific branch. `ICommandBufferUVE::SetUniformFloatUVE`/`SetUniformIntUVE`/
`SetUniformBoolUVE`/`SetUniformVector3UVE`/`SetUniformMatrix4x4UVE` set one named uniform on the
currently-bound pipeline — real `glUniform*` calls in `GlCommandBufferUVE` (looked up against the
bound pipeline's reflected uniform table; an unknown name logs a low-severity warning rather than
asserting, since a `ShaderProgramUVE` caller may legitimately set a uniform a particular built
variant of a shader doesn't declare), recorded into `NullCommandBufferUVE`'s command-sequence spy
otherwise. `CreateShaderUVE`/`CreatePipelineUVE` also gained a trailing `std::string* outInfoLog =
nullptr` parameter (matching `CreateBufferUVE`'s existing optional-out-parameter precedent) so
`ShaderManagerUVE` can capture the raw compile/link log without `GlRenderDeviceUVE` duplicating
its own info-log-fetching logic anywhere else.

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
explicitly temporary scaffold proving `CreateBuffer → CreateProgram (via ShaderManagerUVE) →
Bind/SetUniforms/Draw → Present` end-to-end — it deliberately bypasses `Renderer3DUVE`/
`MeshRendererUVE`/the asset pipeline/the ECS entirely and must never grow into a real content
path. Since Increment 21 its shader program is loaded through `ShaderManagerUVE::CreateProgramUVE()`
from the `basic_3d.glsl` built-in (no more inline GLSL string literals in `EngineCoreUVE` itself);
compilation is asynchronous, so `RenderDemoTriangleUVE()` always clears the framebuffer but only
issues the draw call once `ShaderProgramUVE::IsValidUVE()` is true. The intended long-term
rendering roadmap is: `NullRenderDeviceUVE → GlRenderDeviceUVE (Increment 20) → ShaderManagerUVE
(Increment 21) → Renderer3DUVE scene integration → Materials → PBR → RenderGraph → Editor
viewport` — the demo triangle is only the first visible milestone on that path, not the final
architecture, and should be deleted (not extended) once a real scene-to-window bridge exists.

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

## Shader loading and hot-reload (`engine/render/shader/`, Increment 21)

`ShaderManagerUVE` is **one backend-agnostic implementation**, not a `GlShaderManagerUVE`/
`NullShaderManagerUVE` pair — it is built entirely on top of `IRenderDeviceUVE`
(`CreateShaderUVE`/`CreatePipelineUVE`/the uniform-reflection and binary-cache RHI extensions
below), so it works identically against `NullRenderDeviceUVE` (headless) and `GlRenderDeviceUVE`
(real GL) with zero duplicated compile/link logic. This directly extends this codebase's
established "grow the RHI for a genuine gap, don't fork the consumer" precedent from Increment 20
(`PresentUVE`, `vertexStride`, the `colorAttachment` sentinel): the new capabilities this increment
needed (uniform reflection, uniform setting, a program-binary cache) all became new
`IRenderDeviceUVE`/`ICommandBufferUVE` methods (real in `GlRenderDeviceUVE`/`GlCommandBufferUVE`,
no-op/empty/false in the Null backends) rather than a second shader-manager hierarchy.

**Threading model**: `CreateSourceUVE()`/`CreateProgramUVE()` submit file I/O + `#include`
resolution + macro preprocessing to a background `IThreadPoolUVE` worker (mirroring
`AssetManagerUVE`'s own "background job → mutex-guarded record → `QueueEvent`" pattern) and return
immediately with a not-yet-`IsReadyUVE()` `ShaderSourceUVE`/`ShaderProgramUVE`. The actual GL
compile/link only ever happens inside `ShaderManagerUVE::UpdateUVE()`, called once per frame from
`EngineCoreUVE::Update()` on the main thread — `GlRenderDeviceUVE` has no shared GL context (only
`WindowManagerUVE`'s constructor ever calls `glfwMakeContextCurrent`), so real compilation is
inherently main-thread-only; background work is confined to everything that doesn't touch GL.

**Ownership**: `ShaderSourceUVE`/`ShaderProgramUVE` have private constructors (only
`ShaderManagerUVE` may build one, via `friend`) and are always handed out as
`std::shared_ptr<...>` whose custom deleter captures only `IRenderDeviceUVE&` — not the manager
itself — so an object's lifetime never depends on `ShaderManagerUVE` outliving it. A hot-reload
swap mutates the *same* object's fields in place (never replaces the `shared_ptr`), so every
holder observes the reload automatically with no re-lookup.

**The `#include`/preprocessor** (`Detail::PreprocessShaderSourceUVE`, module-private,
`engine/render/shader/src/shader_preprocessor_uve.h/.cpp`) is a deliberately simplified, hand-rolled
line-based pass — not a full C preprocessor: absolute virtual `#include "path"` paths only (no
relative resolution, no `<...>` form), object-like `#define`/`#undef`/`#ifdef`/`#ifndef`/`#else`/
`#endif` only (no function-like macros, no line continuation), token-boundary-aware macro
substitution via manual character scanning. An `#include` cycle is detected via a visited-file
stack and fails cleanly (never infinite-loops); a file already expanded once this compile is
silently skipped on a second `#include` (a once-per-compile include-guard convenience, since GLSL
itself has no `#pragma once`). Every recursive `#include` is wrapped in GL-native `#line`
directives so both the driver's own compile errors and `Detail::ParseGlInfoLogUVE()`
(`shader_diagnostics_parser_uve.h/.cpp`) map back to the *originally authored* file/line, never the
flattened blob actually hitting `glCompileShader`. Both modules are pure text/data logic with zero
GL or threading involved, so both are independently unit-tested directly (see
`tests/render/shader/shader_preprocessor_uve_tests.cpp`/`shader_diagnostics_parser_uve_tests.cpp`,
which reach these module-private headers via a test-only `target_include_directories` entry in
`tests/CMakeLists.txt` — the only test files in this codebase that `#include` anything from an
`engine/*/src/` directory, since these two are explicitly documented as unit-testable in isolation).

**Built-in shaders** (`engine/render/shader/built_in/*.glsl`) each hold *both* stages in one
physical file, split via `#ifdef VERTEX_SHADER`/`#ifdef FRAGMENT_SHADER` — `ShaderManagerUVE::
CreateProgramUVE()` compiles the same resolved source twice, injecting the matching stage macro
each time, reusing the same conditional-compilation mechanism `UVE_DEBUG`/`UVE_MOBILE` already
need rather than inventing a second file-splitting convention. Every built-in also has a byte-
identical embedded `std::string_view` fallback in `built_in_shaders_uve.cpp` (generated
programmatically from the physical `.glsl` files to guarantee parity by construction) —
`ShaderSourceCompileDescUVE`/`ShaderProgramDescUVE` always carry both a `virtualFilePath` and an
`embeddedFallbackSourceCode`; `ShaderManagerUVE` tries the virtual file first (enabling hot-reload
tracking) and transparently falls back to the embedded string when `IFileSystemUVE::HasFileUVE()`
is false, so the engine still renders correctly even launched from a working directory where the
source tree isn't reachable. `tests/render/shader/shader_manager_uve_tests.cpp`'s
`BuiltInShaderParityUVETest` enforces this byte-parity by reading the physical files directly
(assumes the process's working directory is the repository root, matching every other relative
default path in this codebase — `EngineConfigUVE::shaderSourceRealDirectoryUVE`,
`assetDatabaseFilePath`, `logFilePath`, ...).

**Hot-reload is its own independent mtime-polling loop**, not a reuse of `IHotReloadUVE` —
`IHotReloadUVE` is hard-keyed to one `AssetGuidUVE` ↔ one registered loader with no
dependency-graph concept, so it cannot express "an `#include`d fragment changed, therefore every
program that includes it must reload." `ShaderManagerUVE` tracks each program's full `#include`
dependency closure itself and copies `HotReloadUVE`'s *pattern* (its own accumulator, its own
default poll interval, `std::error_code`-based `last_write_time` calls) independently. A failed
hot-reload recompile leaves the last-known-good program running untouched — the new
shader/pipeline is only destroyed-and-swapped-in after the replacement compiles and links
successfully, so a typo in a live-edited `.glsl` file never blanks the screen.

**The on-disk program-binary cache** (`Detail::ReadCacheEntryUVE`/`WriteCacheEntryUVE`,
`shader_binary_cache_uve.h/.cpp`) is its own tiny binary format — 8-byte magic + format version +
GL binary format + payload, at `<shaderCachePath>/<platform>/<contentHash-as-hex>.uveshadercache`
— deliberately **not** the engine's `.uve*` asset envelope, since this is pure derived data
(reproducible from source), never a shippable asset. A driver-rejected binary (e.g. after a driver
update, or `GL_PROGRAM_BINARY_LENGTH <= 0` as some Mesa/llvmpipe configurations legitimately
report) is treated as an ordinary cache miss — logged at `WARNING`, never `ERROR` — falling back to
a normal from-source compile; a cache *write* failure is equally non-fatal (also `WARNING`), since
the worst case is just a slower next startup, never a broken one.

## Materials (`Renderer3DUVE`, Increment 22)

`MaterialAssetUVE` (Increment 12) has always had a full PBR-shaped field set — `albedoColor`,
`albedoTexture`, `normalTexture`, `metallic`, `roughness`, `aoTexture`, `emissiveColor`,
`vertexShader`, `fragmentShader`, `isTransparent` — but until this increment `Renderer3DUVE` only
ever consumed the two shader GUIDs and `isTransparent`; every other field sat loaded in memory and
untouched. This increment wires the rest through to the GPU. **Two scope-defining decisions,
worth restating for anyone extending this later:**

1. **Unlit textured output only, as of Increment 22.** At the time this section was written, the
   engine had no lighting system at all. `Renderer3DUVE` rendered `albedoColor * albedoTexture`
   (white 1×1 fallback when unset), modulated by AO, with emissive added — but
   `metallic`/`roughness`/`normalTexture` were only *captured, uploaded, bound, and pushed as
   uniforms*, never consumed by any lighting equation. **Increment 23 (see the Lighting section
   below) adds a single directional light + flat ambient Lambertian term** —
   `metallic`/`roughness`/`normalTexture` remain visually inert even after Increment 23; consuming
   them is explicit future PBR work.
2. **Increment 34 routes materials through `ShaderManagerUVE`.** `MaterialAssetUVE` keeps its
   existing independent `vertexShader` and `fragmentShader` GUIDs; there is no `.uvemat` format
   migration. `ShaderProgramStagesDescUVE` gives `ShaderManagerUVE` two source descriptors and
   links them as one manager-owned `ShaderProgramUVE`, so material programs receive the same async
   preprocessing, diagnostics, program-binary cache, include dependency tracking, and hot reload
   lifecycle as built-ins. Existing `.uveshader` assets are decoded envelopes rather than raw GLSL
   files, so their loaded source text is passed as each stage's embedded fallback; root asset reload
   events invalidate the referencing material cache entry, while virtual `#include` dependencies
   remain manager-tracked.

**Increment 34 managed material lifecycle.** `ResolveMaterialGpuResourcesUVE()` submits one
separate-stage manager request after both shader assets and all referenced textures are ready, then
caches the shared `ShaderProgramUVE` with its source GUIDs and resolved texture handles. It never
calls `CreateShaderUVE()`/`CreatePipelineUVE()` directly and never destroys the linked pipeline;
that remains `ShaderManagerUVE` ownership. The first frame after a request safely skips a material
until `IsValidUVE()` becomes true, then later frames render automatically. A material, vertex-shader,
or fragment-shader `AssetReloadedEventUVE` removes only matching material cache entries; a texture
reload still clears the full material cache because texture dependencies are intentionally not
stored per entry. Command-order tests must assert named uniforms rather than insertion positions,
since `ShaderProgramUVE` applies its pending uniform cache without an ordering guarantee.

**Increment 33 activates tangent-space normal mapping in the canonical lit shader.** `MeshVertexUVE`
now carries a runtime-derived normalized tangent plus handedness; the serialized `.uvemodel` payload
remains position/normal/UV compatible, and `GenerateMeshTangentsUVE()` derives its TBN input from
indexed triangles after loading. `Renderer3DUVE` repeats this deterministic derivation into its
one-time GPU-upload copy for custom runtime mesh loaders that bypass the standard asset loader. The
canonical vertex layout adds `TANGENT` as a `Float4`, and the fragment shader orthonormalizes the
TBN basis before decoding `uNormalTexture` from `[0,1]` to `[-1,1]`. Degenerate UVs and direct
legacy draws without a tangent stream receive a deterministic orthogonal fallback rather than
passing an invalid basis to lighting.

**`Render::ToRenderTextureFormatUVE(Asset::TextureFormatUVE)`** (module-private, anonymous
namespace in `renderer_3d_uve.cpp`) bridges `Asset::TextureFormatUVE` (the CPU-side loadable-asset
enum) to `Render::TextureFormatUVE` (the RHI's own, deliberately separate enum — see
`Asset::TextureFormatUVE`'s own doc comment for why engine/asset can't just reuse
`Render::TextureFormatUVE` directly). An exhaustive 2-case switch with a
`UVE_ASSERT(false && "Unhandled ...")` fallback, matching `shader_data_type_uve.cpp`'s existing
idiom — kept module-private (not exported) since `Renderer3DUVE` is its only consumer, mirroring
how `MeshAssetUVE → BufferDescUVE` conversion is already inlined in the same file rather than
factored into public API.

**Two 1×1 fallback textures** (`ImplUVE::fallbackWhiteTexture`/`fallbackNormalTexture`), created
once per `Renderer3DUVE` instance alongside its offscreen color/depth render targets, and
destroyed alongside them too. A material's unset (`kInvalidAssetGuidUVE`) `albedoTexture`/
`aoTexture` resolves to the white texture (`sample * albedoColor == albedoColor`,
`sample.r == 1.0` i.e. no occlusion); unset `normalTexture` resolves to a flat tangent-space "up"
normal (`{128,128,255}` → decodes to `(0,0,1)`) — a neutral TBN-space normal that preserves the
geometric normal under Increment 33's canonical normal-map decode. This avoids any shader-side
branching on "does this material have a texture" — every material always has three real, bound
textures.

**Texture GPU-upload has its own cache** (`ImplUVE::textureCache`, keyed by the texture's own
`AssetGuidUVE`, independent of `materialCache`) so two materials sharing an albedo texture upload
it once. `ResolveTextureGpuHandleUVE()` follows the *exact* same async-readiness contract every
other cache resolver in this file already established (mesh/material/shader): unset GUID → the
fallback immediately, cache hit → the cached handle, still-loading → `std::nullopt` (caller aborts
this material's resolution for the frame without caching anything, retried next frame — this is
why `ResolveMaterialGpuResourcesUVE()` only builds and caches `MaterialGpuResourcesUVE` once
*both* shaders *and* all three textures are ready), and — new to this increment — a genuinely
*failed* load (`AssetHandleUVE::HasFailedUVE()`) resolves to the fallback permanently (logged
once), since retrying a load that will never succeed would spin forever.

**Texture-to-sampler binding is pure convention, no new RHI.** `ICommandBufferUVE::BindTextureUVE`
binds a texture at a raw integer slot; `SetUniformIntUVE` (already existing since Increment 21) is
reused to tell the shader which slot a `sampler2D` uniform should read from — a GLSL sampler
uniform is just an int uniform holding a texture unit index. `Renderer3DUVE` uses three fixed
slots: albedo=0, normal=1, ao=2 (`kAlbedoTextureSlotUVE`/`kNormalTextureSlotUVE`/
`kAoTextureSlotUVE`), and a fixed uniform-name convention every material shader is expected to
declare: `uModel`, `uViewProjection`, `uViewPosition` (the rendering camera's world position, see
the Lighting section's specular formula, Increment 24), `uAlbedoColor`, `uMetallic`, `uRoughness`,
`uEmissiveColor`, `uAlbedoTexture`, `uNormalTexture`, `uAOTexture`. A shader that doesn't declare
one of these is a safe no-op per `SetUniform*UVE`'s own documented contract (unknown/optimized-out
name), not an error.

**A real, pre-existing gap this increment also had to close**: `RecordItemsUVE()` never pushed
`worldMatrix`/`viewProjection` to the shader at all before this increment — `Renderer3DUVE`'s real
scene-rendering path had never actually transformed a mesh correctly end-to-end, independent of
materials/textures. Fixed alongside the material work since nothing about texture binding would
have been meaningfully testable without it.

**Cache eviction on texture reload is deliberately coarse.** `MaterialGpuResourcesUVE` doesn't
track which texture GUIDs it resolved from (only resolved `TextureHandleUVE`s), so
`OnAssetReloadedUVE()` cannot cheaply tell which cached materials referenced a specific reloaded
texture. It therefore destroys+evicts that one `textureCache` entry and clears `materialCache`.
The latter only releases `shared_ptr<ShaderProgramUVE>` references; `ShaderManagerUVE` retains sole
ownership of linked pipelines and temporary stage handles, so renderer tests must not depend on its
internal retirement timing. Material and shader-asset reloads are more precise: source GUIDs kept in
each cache entry identify the affected material program. Both paths rebuild lazily and
non-blockingly on later draws, matching the codebase's preference for simple, obviously-correct
invalidation over fine-grained texture dependency graphs.

**Testing stays within `Renderer3DUVE`'s existing Null-backend-only convention.** Its test fixture
registers a `ShaderAssetUVE` loader that returns fixed garbage source
(`"void main() { }"`) — this has always worked because `NullRenderDeviceUVE::CreateShaderUVE()`
never actually compiles/validates source content, so no real GLSL needs to be authored for these
tests; the `uModel`/`uAlbedoTexture`/etc. names above are simply the string literals
`Renderer3DUVE`'s C++ passes to `SetUniform*UVE()`, asserted against via
`NullRenderDeviceUVE::GetLastSubmittedCommandsUVE()`'s command-spy — `Renderer3DUVE` has never had
a live-GL visual test (its color target is an offscreen texture, not the window's default
framebuffer, so reading it back would need an FBO-bound `glReadPixels` and — since the RHI
deliberately never exposes a raw GL texture/FBO id outside `gl_render_device_uve.cpp`, per this
document's own GL-symbol-confinement rule — no straightforward way to do that from outside the
render module without adding new, unplanned RHI surface).

## Lighting (`LightSystemUVE`, Increment 23)

**Scope, deliberately narrow, as of Increment 23.** At most one active directional light, no
point/spot lights, no light culling, no shadows, no specular/Blinn-Phong — diffuse-only
Lambertian (N·L) plus a flat ambient term. `metallic`/`roughness` (bound as uniforms since
Increment 22) stayed visually inert; consuming them was future PBR work. `LightSystemUVE` is the
spec's Part 7.2 entry ("Light culling, IBL") with culling/IBL still explicitly deferred.
**Increment 24 (see the Specular / Metallic-Workflow PBR section below) adds the specular term** —
`metallic`/`roughness` are no longer inert; point/spot lights, culling, and shadows remain future
work.

**`LightComponentUVE` needed no changes.** It already carried `color`/`intensity` (an
Increment-5-era placeholder) and was already fully wired through `SceneSerializerUVE`. A
directional light's *direction* is deliberately not a component field — it's derived from the
light entity's `WorldTransformComponentUVE::worldRotation` via `RotateVectorUVE(rotation,
{0,0,-1})`, the same "forward" convention `CameraSystemUVE`'s view matrix and `EngineCoreUVE`'s
audio-listener code already use. Rotate the light entity (e.g. via its `TransformComponentUVE`) to
aim it.

**`ILightSystemUVE`/`LightSystemUVE`** (`engine/render/`) is stateless, mirroring
`ICameraSystemUVE`/`IMeshRendererUVE`. `ExtractActiveLightUVE(Scene::IEntityManagerUVE&)` — note:
non-`const`, unlike `ICameraSystemUVE`'s methods, because it goes through
`IEntityManagerUVE::ForEachUVE`, which has no `const` overload (same reason
`IMeshRendererUVE::ExtractRenderQueueUVE` takes a non-`const` reference) — returns the first
matching entity `ForEachUVE` encounters, or a default `DirectionalLightDataUVE{}` (`intensity ==
0.0F`, the deliberate "no light" sentinel — no `std::optional`, no shader branching, matching
Increment 22's fallback-texture philosophy). Which entity "wins" when multiple lights exist is
unspecified this v1; deterministic only when all light entities share one archetype (within a
single archetype, iteration order is chunk/row creation order; across different archetypes it's
`std::unordered_map`-hash-dependent).

**`Renderer3DUVE` wiring.** Gains an injected `ILightSystemUVE&` and a plain `Math::Vector3UVE
ambientColor` constructor parameter (from `EngineConfigUVE::ambientColor`, following the exact
`gravity` precedent — no "scene settings" component/system exists in this engine, so an
engine-config constant is the only existing whole-scene-tunable pattern). `RenderFrameUVE()`
computes `ExtractActiveLightUVE()` once per frame (like `viewProjection`) and bundles it with
`viewProjection`/`ambientColor` into a module-private `FrameUniformsUVE` struct — mirroring
`PipelineDescUVE`/`RenderPassDescUVE`'s own precedent for grouping related descriptor data, rather
than growing `RecordItemsUVE`'s parameter list to five positional parameters.

**Four new fixed uniform names**, added to the Materials section's convention list: `uLightDirection`,
`uLightColor`, `uLightIntensity`, `uAmbientColor` — pushed unconditionally every item, right after
`uViewProjection`. A shader that declares none of them is an unaffected no-op per `SetUniform*UVE`'s
existing contract. Expected GLSL usage (not implemented this increment — see below):
```glsl
vec3 diffuse = albedo * uLightColor * uLightIntensity * max(dot(normalize(N), normalize(-uLightDirection)), 0.0);
vec3 ambient = albedo * uAmbientColor;
vec3 color = ambient + diffuse + emissive;
```

**Out of scope, deliberately.** The four built-in `.glsl` files (`engine/render/shader/built_in/`)
are untouched — they're `ShaderManagerUVE`'s demo-triangle scaffold only, already stale since
Increment 22 (`uColor`/`uTexture` naming, no lighting math, no `aNormal`), and updating them is
unrelated pre-existing staleness, not this increment's job. `MeshVertexUVE` already uploads normals
(`NORMAL` attribute in `MeshVertexLayoutUVE()`, since before this increment) — no vertex-layout
change was needed. No real GLSL was authored for tests, matching Increment 22:
`Renderer3DUVE`'s tests use `NullRenderDeviceUVE`, which never compiles/validates shader source.
**Known simplification for whoever eventually authors a real lit shader**: `Renderer3DUVE` does not
compute or push a normal matrix (inverse-transpose of the model matrix) this increment — correct
world-space normal transform under non-uniform scale needs one; using `uModel` directly to
transform normals is only correct under uniform scale. Deferred, not forgotten.

## Specular / Metallic-Workflow PBR (Increment 24)

**Makes `metallic`/`roughness` finally meaningful.** Increment 23 deliberately left them inert
(diffuse-only Lambertian + ambient). This increment documents a metallic-workflow specular term
(Fresnel-Schlick `F0 = mix(0.04, albedo, metallic)` + a roughness-driven distribution) and threads
the one piece of data that formula needs but wasn't available yet: the rendering camera's world
position (the view vector `V`, and the half-vector `H = normalize(V + L)`). `N`, `L`, `albedo`,
`metallic`, `roughness` were already available.

**Still zero C++-side lighting math**, matching Increment 23's own precedent exactly: this
codebase has never computed N·L, Fresnel, or any other lighting term in C++ — every formula here
is a documented expected-GLSL-usage comment, never compiled or executed by any test
(`NullRenderDeviceUVE` never validates shader source; the material test fixture's shader loader
returns literal garbage `"void main() { }"`). The only actual code change is plumbing one new
`Math::Vector3UVE` uniform through.

**`ICameraSystemUVE` gains `GetWorldPositionUVE(entityManager, cameraEntity)`** — a thin wrapper
returning `WorldTransformComponentUVE::worldPosition`, added specifically so `Renderer3DUVE`
keeps funneling every camera-related read through `ICameraSystemUVE` (it never reads
`WorldTransformComponentUVE` directly for the camera; adding a direct `GetComponentUVE` call
there instead would have broken that boundary). Same missing-component contract as
`ComputeViewMatrixUVE`: no graceful fallback, asserts in debug builds.

**`Renderer3DUVE` wiring.** No constructor signature change (no new dependency — `ICameraSystemUVE`
was already injected). `FrameUniformsUVE` gains `Math::Vector3UVE viewPosition`, computed once per
frame in `RenderFrameUVE()` alongside `viewProjection`/the light data, and pushed as a new
`uViewPosition` uniform in `RecordItemsUVE()` — grouped with the other frame-constant pushes
(`uModel`, `uViewProjection`, `uLightDirection`, `uLightColor`, `uLightIntensity`,
`uAmbientColor`, `uViewPosition`, then the material-uniform group). Per-item command count rises
from 20 to 21 (22 → 23 total per render, including Begin/EndRenderPass).

**Expected GLSL usage** (documented only, not implemented/compiled this increment, same as
Increment 23's diffuse formula):
```glsl
// Simplified metallic-workflow specular (no geometry/visibility term — a real Smith G term
// is deferred, not forgotten; see the known-simplification note below).
vec3 N = normalize(vNormal);
vec3 L = normalize(-uLightDirection);
vec3 V = normalize(uViewPosition - vWorldPos);
vec3 H = normalize(V + L);

vec3 F0 = mix(vec3(0.04), albedo, uMetallic);
float NdotH = max(dot(N, H), 0.0);
float roughness2 = uRoughness * uRoughness;
float denom = NdotH * NdotH * (roughness2 - 1.0) + 1.0;
float D = roughness2 / (3.14159265 * denom * denom);
vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(V, H), 0.0), 5.0);

float NdotL = max(dot(N, L), 0.0);
vec3 diffuse = albedo * (1.0 - uMetallic) * uLightColor * uLightIntensity * NdotL;
vec3 specular = D * F * uLightColor * uLightIntensity * NdotL; // NdotL stands in for the
                                                                // omitted geometry term
vec3 ambient = albedo * uAmbientColor;
vec3 color = ambient + diffuse + specular + emissive;
```
`vWorldPos` is a vertex-shader-interpolated varying, not a uniform — out of scope to define since
no real shader is authored this increment, exactly like Increment 23.

**Increment 35 upgrades canonical direct lighting to a GGX/Smith microfacet BRDF.** The physical
`lit_shadowed_3d.glsl` file and its byte-identical embedded fallback now evaluate Trowbridge-Reitz
GGX distribution, Schlick-GGX/Smith visibility, and Schlick Fresnel for every active direct light.
No material asset or renderer-uniform contract changes: `uMetallic`, `uRoughness`, albedo, normal,
AO, emissive, view position, and the existing light records are reused. With `F0 = mix(0.04,
albedo, metallic)`, the direct term is `specular = D*G*F / max(4*NdotV*NdotL, epsilon)` and the
energy split is `diffuse = (1-F)*(1-metallic)*albedo/PI`. Roughness remains clamped to `[0.04,1]`
and every normalization/BRDF denominator has an epsilon guard. Directional cascaded PCF/blended
shadow visibility multiplies only the completed direct contribution; AO-modulated flat ambient and
emissive stay outside the direct-light loop.

**Still deliberately deferred:** image-based lighting, irradiance/prefiltered environment maps,
BRDF lookup integration, HDR/tone mapping, normal-matrix support for non-uniform model scaling, and
any material-format change. Point, spot, multi-light, normal-map, and directional-shadow inputs are
already supported by the same direct path and are retained by this increment.

**Known simplification retained:** the normal-matrix gap from Increment 23 remains
unaddressed; transforming normals with `mat3(uModel)` is not correct under non-uniform scale. GGX
visibility is no longer deferred, but normal-matrix support belongs to a separate mesh-transform
increment.

## Point/Spot Lights + Multi-Light (Increment 25)

**Replaces the single-directional-light limit with up to `kMaxLightsUVE = 4` simultaneous lights of
any mix of types.** Increments 23-24 deliberately scoped down to one active directional light —
both increments' own doc comments flagged point/spot lights and multi-light support as the
anticipated next step. This increment is that step: it extends `LightComponentUVE` with a light
type plus type-specific falloff data, and reworks `ILightSystemUVE`/`Renderer3DUVE` to gather and
push a fixed-size array of lights per frame instead of just one.

**Two scope decisions, confirmed before implementation, not inferred:**
- **Fixed max of 4 simultaneous lights** (`Render::kMaxLightsUVE`), a compile-time constant, not
  `EngineConfigUVE`-driven — no request for runtime configurability, matching the existing
  fixed-texture-slot-constant precedent from Increment 22.
- **Selection policy when more than 4 lights exist in a scene: first-N-encountered, not
  closest-to-camera.** This is a direct extension of the v1 "arbitrary/unspecified, deterministic
  only within one archetype" precedent — it requires zero new C++ math (no distance computation or
  sorting), keeping this increment's C++ surface limited to data plumbing, exactly like
  Increments 23-24's own "no C++ lighting math" discipline. A closest-N-to-camera policy is
  explicitly deferred, not implemented — no "bounded-N-with-sort-policy" precedent exists anywhere
  else in this engine (the closest analog, `RaycastSystemUVE::RaycastUVE()`, selects exactly one
  closest hit, not N).

**`Scene::LightComponentUVE` extended, field order deliberately preserving backward compatibility:**
```cpp
enum class LightTypeUVE : std::uint8_t { Directional = 0, Point = 1, Spot = 2 };

struct LightComponentUVE final {
    // color/intensity stay first (their original Increment-5 position) so every existing 2-arg
    // aggregate-init call site (LightComponentUVE{color, intensity}) keeps compiling unmodified
    Math::Vector3UVE color{1.0F, 1.0F, 1.0F};
    float intensity = 1.0F;
    LightTypeUVE type = LightTypeUVE::Directional;
    float range = 10.0F;            // Point/Spot only; ignored for Directional
    float spotAngleDegrees = 45.0F; // Spot only (cone half-angle); ignored otherwise
};
```
The approved plan originally put `type` first; before touching any code, `grep -rn
"LightComponentUVE{"` turned up 8 existing 2-arg positional aggregate-init call sites across the
codebase (including one test file not even in the task list). Reordering to keep `color`/
`intensity` first — appending the 3 new fields after — was the fix, applied before any build was
attempted. **When adding fields to any existing aggregate-init'd component struct, grep for
existing positional-init call sites first and append new fields after the original ones**, unless
every call site is being updated to designated/named initialization in the same change.

Position/direction stay derived from the entity's `WorldTransformComponentUVE`
(`worldPosition`/`worldRotation` via `RotateVectorUVE(rotation, {0,0,-1})`) — not stored on the
component, preserving the Increment 23 convention.

**`SceneSerializerUVE` backward compatibility** follows the established `ColliderComponentUVE`/
`AudioSourceComponentUVE` pattern exactly: `color`/`intensity` (the original fields) stay
`json.at(...)` (required); `type`/`range`/`spotAngleDegrees` (the new fields) use
`json.value("fieldName", defaultValue)` with defaults matching the struct's in-class defaults, so
old saves without those keys silently get normal defaults. `type` serializes via
`static_cast<std::uint8_t>` to/from JSON (matching the `AudioSourceComponentUVE::attenuationCurve`
precedent), never as a string.

**`DirectionalLightDataUVE` renamed to `LightDataUVE`** — "Directional" stopped being accurate once
Point/Spot exist. Always a full record per slot (every field populated regardless of type),
preserving the established "no shader-side branching on optional data" philosophy from the
fallback-texture/zero-intensity-sentinel precedents: a Directional slot's irrelevant `position`/
`range`/`spotAngleDegrees` are simply unused by the (documented, not implemented) GLSL type-branch.
```cpp
constexpr std::size_t kMaxLightsUVE = 4;
struct LightDataUVE {
    Scene::LightTypeUVE type = Scene::LightTypeUVE::Directional;
    Math::Vector3UVE position{};                    // Point/Spot: world position
    Math::Vector3UVE direction{0.0F, 0.0F, -1.0F};   // Directional/Spot
    Math::Vector3UVE color{1.0F, 1.0F, 1.0F};
    float intensity = 0.0F;   // 0.0F sentinel: this slot is empty — extends the v1 "no light"
                               // sentinel to a per-slot basis, still zero shader branching needed
    float range = 10.0F;
    float spotAngleDegrees = 45.0F;
};
using LightListUVE = std::array<LightDataUVE, kMaxLightsUVE>;
```

**`ILightSystemUVE::ExtractActiveLightsUVE`** (renamed from `ExtractActiveLightUVE`) returns a
`LightListUVE` instead of a single `LightDataUVE`. `LightSystemUVE`'s implementation walks
`ForEachUVE<WorldTransformComponentUVE, LightComponentUVE>` filling the array in encounter order,
skipping further writes once `kMaxLightsUVE` slots are filled (a simple count guard — first-N-
encountered, no sorting, no distance math). Trailing unfilled slots keep the default `LightDataUVE{}`
sentinel (`intensity == 0.0F`).

**`Renderer3DUVE` wiring.** `FrameUniformsUVE::light` (single) becomes `FrameUniformsUVE::lights`
(`LightListUVE`). `RecordItemsUVE()`'s old 3-uniform light block becomes a `for` loop over
`kMaxLightsUVE` slots, pushing GLSL array-of-struct-named uniforms — `"uLights[i].type"`,
`.position`, `.direction`, `.color`, `.intensity`, `.range`, `.spotAngleDegrees` (7 calls × 4 slots
= 28 total, replacing the old 3). This works with zero RHI changes: `GlCommandBufferUVE`'s uniform
lookup (`FindUniformUVE`) is a plain string-keyed `std::unordered_map` lookup built once at
pipeline-link time — an array-of-struct name like `"uLights[0].color"` is just another string key,
identical in cost and mechanism to a flat name. Chosen over flat indexed names (`uLightColor0`) as
the more idiomatic, scalable convention for real multi-light shader authors. Per-render command
count for the single-mesh/no-light test scenario rises from 23 to 48 (Begin/EndRenderPass
unaffected); see `RenderFrameUVE_VisibleMesh_RecordsExpectedCommandSequence` in
`renderer_3d_uve_tests.cpp` for the exact indexed layout.

**Expected GLSL usage** (documented only, not implemented/compiled this increment, same as every
prior lighting increment):
```glsl
struct LightUVE {
    int type;             // 0 = Directional, 1 = Point, 2 = Spot
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float range;
    float spotAngleDegrees;
};
uniform LightUVE uLights[4];

vec3 lighting = ambient;
for (int i = 0; i < 4; ++i) {
    if (uLights[i].intensity <= 0.0) { continue; } // empty-slot sentinel, same idiom as v1

    vec3 L;
    float attenuation = 1.0;
    if (uLights[i].type == 0) { // Directional
        L = normalize(-uLights[i].direction);
    } else { // Point or Spot
        vec3 toLight = uLights[i].position - vWorldPos;
        float dist = length(toLight);
        L = toLight / dist;
        attenuation = 1.0 / max(dist * dist, 0.0001); // inverse-square falloff
        if (uLights[i].type == 2) { // Spot: hard cone cutoff, no soft edge
            float cosAngle = dot(-L, normalize(uLights[i].direction));
            if (cosAngle < cos(radians(uLights[i].spotAngleDegrees))) { attenuation = 0.0; }
        }
    }

    // ...N·L diffuse + metallic-workflow specular from Increment 24, each term multiplied by
    // uLights[i].color * uLights[i].intensity * attenuation, accumulated into `lighting`
}
```

**Out of scope, deliberately**: light culling/prioritization beyond first-N-encountered, shadows,
IBL, soft/smoothstep spot cone edges (hard cutoff only), real GLSL authoring or live-GL visual
proof (same GL-symbol-confinement rationale as Increments 22-24), `ShaderManagerUVE`↔materials
bridging, configurable `kMaxLightsUVE`, any new `Vector3UVE`/math helpers — `range`/
`spotAngleDegrees` are stored/passed through as plain data, never computed on in C++. **Still zero
C++-side lighting math** — this codebase has never computed N·L, Fresnel, attenuation, or any other
lighting term in C++; every formula remains a documented expected-GLSL-usage comment, never
compiled or executed by any test.

## Directional Shadow Mapping (Increment 26)

**Delivers real shadow mapping for the primary use case: a single sun/directional light.**
Increments 23-25 built lighting but nothing occludes it — every surface stays fully lit. Two
scope-defining decisions were confirmed with the user before implementation:

1. **Only Directional lights cast shadows.** Point/Spot shadows need cubemap/perspective shadow
   maps and multiple render passes per light — explicitly deferred, matching every prior
   increment's "start with the simple case" precedent.
2. **The shadow frustum is a fixed-half-extent orthographic box centered on the shadow-casting
   light entity's world position** (`EngineConfigUVE::shadowMapHalfExtent`/`shadowMapNearPlane`/
   `shadowMapFarPlane`) before the Increment 29 fitted-frustum follow-up. That fixed box was a
   deliberate minimal baseline, not the final quality target.

**Increment 26 makes the depth pre-pass real, compiled, and tested.** The position-only
`engine/render/shader/built_in/shadow_depth.glsl` shader follows the established physical-file +
embedded-fallback + parity-test convention and produces the directional-light depth texture.

**Increment 27 completes the material-side vertical slice.**
`engine/render/shader/built_in/lit_shadowed_3d.glsl` is the canonical reference material shader
for `ShaderAssetUVE` authors. Its vertex stage emits world position, world normal, texture
coordinates, and light-space position. Its fragment stage samples the renderer-bound
`uShadowMapTexture`, converts light-space clip coordinates to `[0,1]` texture coordinates, treats
fragments outside the shadow map as lit, and applies a slope-scaled depth bias before comparing
depths. The shadow factor multiplies only the direct contribution of Directional lights; ambient,
point-light, and spot-light contributions are unaffected. The shader has a byte-identical embedded
fallback and is compiled in a real OpenGL test.

**Increment 28 replaces the hard comparison with bounded percentage-closer filtering (PCF).**
`EngineConfigUVE::shadowPcfKernelRadius` controls the canonical shader's square filter radius in
shadow-map texels. `0` preserves a single hard comparison, `1` (the default) samples a 3x3 kernel,
and values above `2` clamp to a bounded 5x5 kernel. The shader derives texel size directly from
`textureSize(uShadowMapTexture, 0)`, so no resolution-specific uniform is needed. The real OpenGL
test now proves three ordered samples: an occluded center is darkest, a filter-boundary sample is
intermediate, and an unoccluded sample is brightest.

**Increment 29 fits the directional shadow volume to the active camera and culls shadow casters
against that light volume.** `ICameraSystemUVE::ComputeFrustumCornersUVE()` reconstructs the eight
world-space perspective corners from the camera transform, FOV, near/far planes, and render-target
aspect ratio, avoiding a generic 4x4 inverse. `Renderer3DUVE` transforms those corners into light
view space, builds the orthographic bounds with non-negative `shadowFrustumPadding`, and extracts a
second `RenderQueueUVE` against the fitted light frustum. The main pass retains its independently
camera-culled queue, while off-camera opaque objects inside the shadow volume can still cast into
view. This is exercised by a regression test with two shadow draws and only one main-pass draw.

**Increment 30 adds bounded cascaded directional shadow maps.** The renderer owns exactly three
separate `Depth32Float` cascade textures, avoiding a texture-array RHI expansion. It divides the
active camera depth range with a practical uniform/logarithmic blend controlled by clamped
`EngineConfigUVE::shadowCascadeSplitLambda`, reconstructs a fitted light frustum for each slice,
and records one depth pass per cascade. The canonical material shader selects the first cascade
whose split contains the fragment's camera distance, then performs the existing bounded PCF sampling
against that cascade's depth texture. Legacy `uLightSpaceMatrix`/`uShadowMapTexture` uniforms remain
bound so existing project-authored single-map material shaders and direct shader tests retain their
prior contract.

**Increment 31 adds bounded cascade transition blending.** `EngineConfigUVE::shadowCascadeBlendRatio`
defaults to `0.1` and `Renderer3DUVE` clamps it to `[0, 0.25]`. The canonical material shader always
receives `uShadowCascadeBlendRatio`; when a fragment falls in the final configured fraction of a
non-final cascade's view-depth interval, it evaluates both that cascade and the following cascade
and smoothly cross-fades their PCF shadow factors. A ratio of `0` preserves Increment 30's hard
transition, while the upper bound limits overlap work to a quarter of each cascade. The final
cascade never samples beyond its fixed three-map contract, and the no-directional-light legacy
single-map fallback stays unchanged.

**Increment 32 adds texel-grid stabilization for fitted cascade bounds.** After deriving each
camera-slice AABB in directional-light view space, `Renderer3DUVE` expands its XY extents by
`shadowFrustumPadding`, widens each half extent by one shadow texel on each side, and snaps the
XY center down to that cascade texture's grid. The widening keeps the snapped projection
conservative for camera-frustum coverage; the same stabilized matrix then drives both shadow-pass
rendering and light-frustum caster culling. Z remains fitted with the existing padding because
shadow resolution is two-dimensional. This is always enabled for the fixed three-cascade contract;
there is no public toggle or cascade-count change. A regression test proves sub-texel camera motion
leaves all cascade matrices unchanged and a crossed near-cascade texel boundary produces its
bounded single-texel projection translation.

**`Matrix4x4UVE::OrthographicUVE`** — a new factory matching `PerspectiveUVE`'s existing Vulkan-
depth-range `[0,1]`/Y-up-NDC convention. Real, tested math (`tests/math/matrix4x4_uve_tests.cpp`).

**`LightDataUVE` gains `Math::QuaternionUVE rotation`**, populated from
`worldTransform.worldRotation` — the same source `direction` is already derived from. This reuses
`Matrix4x4UVE::ViewFromPositionAndRotationUVE` directly for a light's view matrix with zero new
view-matrix math, instead of inventing a `LookAtUVE` from a direction vector.

**`GlCommandBufferUVE::BeginRenderPassUVE` depth-only FBO path.** Previously,
`colorAttachment == kInvalidTextureHandleUVE` unconditionally routed to the *default* framebuffer,
silently ignoring any `depthAttachment` — there was no depth-only (no color) FBO path. Fixed by
restructuring the branch to "`colorAttachment == invalid && depthAttachment == invalid` → default
framebuffer; otherwise build a real FBO", attaching color only if present, depth only if present,
calling `glDrawBuffer(GL_NONE)`/`glReadBuffer(GL_NONE)` when there's no color attachment (a
core-profile completeness requirement for a depth-only FBO — both are legacy OpenGL 1.1 entry
points `<GL/gl.h>` already declares directly, no loader entry needed, same as the bare
`glViewport`/`glClear` calls already in that function), and deriving the viewport size from
whichever attachment is actually present.

**`Renderer3DUVE` wiring** — the largest piece:
- Constructor gains `Shader::IShaderManagerUVE& shaderManager` (positioned after `lightSystem`,
  matching `EngineCoreUVE`'s existing construction order — `ShaderManagerUVE` is already
  constructed before `Renderer3D`) plus `shadowMapResolution`/`shadowMapHalfExtent`/
  `shadowMapNearPlane`/`shadowMapFarPlane`, then `shadowFrustumPadding`,
  `shadowCascadeSplitLambda`, `shadowCascadeBlendRatio`, and the bounded PCF radius.
- New members: `shadowMapTargets[3]` (three persistent `Depth32Float` textures sized
  `shadowMapResolution × shadowMapResolution` — unlike the main pass's own `depthTarget`, which is
  written and never sampled, each is later bound as a sampled cascade input during the main pass)
  and `shadowProgram` (a `std::shared_ptr<Shader::ShaderProgramUVE>` created once via
  `shaderManager.CreateProgramUVE(...)`, reusing `MeshVertexLayoutUVE()` — the exact same vertex
  layout materials use, since the shadow pass draws the same mesh vertex buffers even though its
  vertex shader only reads `POSITION`). This is the exact same pattern `EngineCoreUVE`'s demo
  triangle already established for a built-in (non-material) shader: compile via
  `IShaderManagerUVE`, hold the `ShaderProgramUVE`, `ApplyToUVE(commandBuffer)` binds the pipeline
  and flushes queued uniforms.
- `kShadowMapTextureSlotUVE = 3`, the first of three consecutive cascade slots after the three
  material texture slots.
- `RenderFrameUVE()` finds the first slot where `type == Directional && intensity > 0.0F` (a
  simple linear scan via `FindShadowCasterUVE` — no sorting, same first-N-encountered spirit as
  Increment 25). If found, transforms the active camera's eight perspective corners into the
  light's view space, divides the camera range into three practical cascade slices, fits each
  light-space box plus `shadowFrustumPadding`, widens and snaps its XY center to the corresponding
  shadow-map texel grid, then composes every `OrthographicUVE(...)` projection with
  `ViewFromPositionAndRotationUVE(caster.position, caster.rotation)`; otherwise it retains a zero
  cascade-count sentinel and identity matrices.
- **`RecordShadowPassUVE`, called before the existing main pass, runs once per fixed cascade per
  frame** — `BeginRenderPassUVE{colorAttachment: invalid, depthAttachment: shadowMapTargets[i],
  depthLoadOp: Clear, clearDepth: 1.0}`, draws every opaque item only if a caster was found *and*
  `shadowProgram->IsValidUVE()`, then `EndRenderPassUVE`. When there's no caster or the program
  isn't ready yet, the pass still begins/ends but draws nothing, leaving the shadow map cleared to
  `1.0` (far plane). **No C++-side branching or sentinel flag is needed for the "no shadow caster"
  case** — a shadow-comparison formula (`sampledDepth >= currentDepth → lit`) against an all-`1.0`
  map naturally always evaluates "not in shadow," composing for free with the existing clear-value
  default. Matches this codebase's established sentinel philosophy (zero-intensity light slots,
  fallback textures). Increment 29 provides the pass a separately extracted opaque queue culled by
  the fitted light frustum, so valid off-camera casters are retained without submitting meshes that
  cannot intersect the shadow volume.
- The main pass retains the legacy `uLightSpaceMatrix`/`uShadowMapTexture` pair, and also uploads
  `uShadowCascadeCount`, `uShadowCascadeBlendRatio`, `uLightSpaceMatrices[3]`,
  `uShadowCascadeSplits[3]`, and `uShadowMapTextures[3]` on slots 3-5 before
  `uShadowPcfKernelRadius`. Unknown extra uniforms are harmless no-ops for project-authored shaders
  that retain the older single-map contract.

**Canonical material shader contract (Increments 27-33).** `lit_shadowed_3d.glsl` declares the
existing renderer-owned `uLights[4]`, material color/scalar uniforms, texture samplers, and the
legacy `uLightSpaceMatrix`/`uShadowMapTexture` pair. Increment 28 adds
`uShadowPcfKernelRadius`; Increment 30 adds the fixed three-element
`uLightSpaceMatrices`/`uShadowMapTextures`/`uShadowCascadeSplits` arrays plus
`uShadowCascadeCount`; Increment 31 adds `uShadowCascadeBlendRatio` for a bounded cross-fade
near non-final cascade splits; and Increment 33 consumes `uNormalTexture` through a tangent-space
basis built from the renderer-provided `TANGENT` vertex attribute. `Renderer3DUVE::RecordItemsUVE()`
supplies these material and shadow inputs from bounded engine-level configuration. Project-authored
shaders retain their own source and uniform contracts; unknown renderer-provided uniforms remain
safe no-ops.

**Out of scope, deliberately**: Point/Spot shadows, variable cascade counts, texture-array
shadow maps, variable or larger-than-5x5 PCF kernels, skinning/animated tangents, and
material-default substitution for arbitrary `ShaderAssetUVE` sources. The canonical shader is a
reference implementation, not an automatic override of project-authored material shaders.

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


## Render Graph Foundation (Increment 36)

**`RenderGraphUVE` makes existing pass dependencies explicit without taking RHI resource ownership.**
The foundation imports persistent texture handles supplied by `Renderer3DUVE`, declares each pass's
read/write use, validates references before recording, and executes callbacks in deterministic
insertion order. It does not allocate, resize, alias, or destroy textures; `Renderer3DUVE` remains
the only owner of its color target, depth target, and three directional shadow-map targets.

The first graph preserves the existing frame shape exactly: three depth-only
`DirectionalShadowCascadeN` passes write their corresponding shadow map, then `MainColor` writes
the color/depth targets and reads all three shadows. `RenderSystemUVE` still owns one
`BeginFrameUVE()`/`EndFrameUVE()` pair and one command buffer per frame; graph callbacks merely
record the existing `BeginRenderPassUVE`/draw/`EndRenderPassUVE` work into that buffer.

**Deliberately deferred:** transient resource allocation and aliasing, automatic barriers, pass
parallelism, compute scheduling, pass culling, post-processing nodes, and visual graph inspection.
New passes must declare imported-resource reads/writes and retain stable ordering until a later
increment introduces a fuller scheduler.


## Post-Processing Foundation (Increment 37)

**`Renderer3DUVE` now renders its scene pass to an `RGBA16Float` HDR color target before presentation.**
The graph's `MainColor` pass writes that target, and its following `ToneMapping` pass declares the
scene color as a graph read before drawing a vertex-ID fullscreen triangle to the default framebuffer.
The default framebuffer is an external presentation surface rather than a `TextureHandleUVE`; it is
therefore intentionally not imported or owned by `RenderGraphUVE`.

The managed built-in `fullscreen_quad.glsl` program samples `uSourceTexture` with explicit
fullscreen UVs and applies an ACES-style fitted tone-map curve. It has no vertex buffer and disables
depth test/write. The pass safely skips until its `ShaderProgramUVE` is valid, matching existing
managed-program readiness behavior; later frames present automatically. Both physical and embedded
fullscreen shader sources must remain byte-identical.

**Deliberately deferred:** exposure controls, gamma/color-space configuration, bloom, SSAO, color
grade/LUTs, post-process chaining, dynamic-resolution resizing, and HDR presentation swapchain
configuration. Increment 37 establishes only the HDR-scene-to-LDR-presentation graph edge needed
by those future passes.


## Editor Foundation v1 (Increment 38)

**`engine/editor` is an upper-layer composition module.** `EditorUVE` owns editor-session state only: the active document path, dirty flag, one selected entity, an editor-only camera entity, and the editor-private UI backend. It receives `EngineServicesUVE` only during the normal engine lifecycle and never owns or retains lower-layer resources after shutdown. `engine/core`, `engine/scene`, `engine/render`, `engine/window`, and `engine/input` must never depend on `engine/editor`.

The standalone `uve_editor` executable is a sibling of `uve_runtime`. It drives the ordinary `EngineCoreUVE` lifecycle, creates an `EditorUVE` after `EngineCoreUVE::Load()` succeeds, selects the editor camera as the renderer's active camera, and clears/destroys editor state before `EngineCoreUVE::Shutdown()`. `--scene <path>` selects the `.uvescene` document, `--frames <n>` bounds an automated run, `--gl-version <major.minor>` explicitly overrides the desktop context request for a constrained platform such as virtual-display CI, and `--headless` keeps editor logic testable without a native window or GL context.

**Overlay ordering is explicit.** `EngineCoreUVE::SetPostRenderCallbackUVE()` is a generic, non-owning application-overlay seam with no editor or GUI types in the core API. In a windowed frame, `Renderer3DUVE` still performs its own HDR `MainColor -> ToneMapping` graph work first; the legacy proof-of-life default-framebuffer pass then completes; a registered overlay callback draws; and only then does `IRenderDeviceUVE::PresentUVE()` swap the back buffer. The runtime does not register a callback, so its frame behavior remains unchanged. Headless runs never invoke the callback.

**Dear ImGui is a pinned, editor-private dependency.** CMake `FetchContent` retrieves version `v1.91.5` at commit `f401021d5a5d56fe2304056c391e78f81c8d4b8f`, following the repository's established GLFW/GoogleTest dependency pattern. Its GLFW/OpenGL3 backend sources link only into `uve_editor_imgui`; all ImGui headers and types are confined to `engine/editor/src/editor_uve.cpp`. Third-party code is deliberately compiled outside UVE's warning-as-error policy, while all UniVex editor source remains subject to it.

The foundation layout has four panels: **Scene** on the left, **Properties** on the right, an interactive transparent **Viewport** in the center, and **Assets** across the bottom beneath all three. Hierarchy labels prefer a persistent `NameComponentUVE` when present and otherwise retain the stable entity-index/generation fallback for legacy and runtime-created entities. Selection has one shared owner in `EditorUVE`: hierarchy clicks, viewport picking, the Properties panel, and the translate gizmo all operate on the same live `m_selectedEntity`. The inspector supports exactly one live selected entity and writes its Name metadata plus existing local Transform through validated editor paths, preserving dirty propagation into derived world transforms. Invalid, deleted, non-transform, non-finite, or invalid-name edits are rejected without mutating ECS state.

### Viewport Picking + Transform Gizmos v1 (Increments 39 and 46)

The viewport remains an editor input/overlay rectangle over the renderer-owned default framebuffer; it does **not** create a duplicate scene texture, render target, or presentation path. The editor-private ImGui backend reads pointer state directly so overlay interactions remain isolated from runtime action mappings. `EditorUVE::MakeViewportRayUVE()` maps a valid pointer within the active viewport rectangle through the editor camera's FOV/aspect data and read-only `WorldTransformComponentUVE` into a normalized `Math::RayUVE`. Invalid camera state, stale/dirty world transforms, invalid projection values, non-finite coordinates, or out-of-viewport coordinates return no ray.

`EditorUVE::PickViewportUVE()` composes that ray with the existing deterministic `IRaycastSystemUVE`. The initial selection boundary is intentional: only live document entities with the existing `WorldTransformComponentUVE` plus box `ColliderComponentUVE` can be picked. The editor camera and non-document entities cannot become selections; a valid viewport miss clears selection. Mesh-triangle/renderer-ID picking, non-collider selection, and multi-selection remain future work rather than being approximated by an imprecise fallback.

`EditorUVE` exposes first-pass world-axis **Translate**, **Rotate**, and **Scale** modes for one selected document entity. Handle hit-testing retains priority over scene picking. Translate projects pointer movement onto a chosen screen-space axis and converts the resulting world-space delta back to local position; parent world rotation and scale are accounted for before calling the existing scene-graph transform setter. Rotate renders sampled X/Y/Z world-space ring polylines, hit-tests their projected segments, and converts a signed world-axis angular delta into the selected entity's local quaternion. For a parent world rotation `P`, world delta `W`, and authored local rotation `L`, the local update is `inverse(P) × W × P × L`; root entities use `W × L`. The result is normalized before it reaches `ISceneGraphUVE::SetLocalTransformUVE()`.

Scale uses projected world-axis handles for interaction but changes only the matching authored positive `localScale` component. Each transform mode rejects deleted entities, non-finite input, invalid or dirty parent transforms, zero/invalid quaternion magnitudes, unsafe projection, and active competing gestures without mutation. Scale additionally rejects a proposed component below the documented positive minimum, so v1 never creates zero, negative, or mirrored local scale. A left-button drag is a single Transform history transaction: non-recording updates preview the initial Transform, changed release records one entry, and cancellation restores the initial local Transform plus dirty state. The `W`, `E`, and `R` mode shortcuts, and View-menu controls, are rejected while a text field owns input or an active gizmo/navigation gesture exists. This slice deliberately supports world axes only: local axes, plane translate handles, uniform/negative scale, free/trackball rotation, snapping, and final selection-outline rendering remain deferred.

**Scene documents continue to use only `ISceneSerializerUVE`.** Saving serializes document roots as `.uvescene` with `AssetKindUVE::Scene`; the editor camera is explicitly excluded. Loading first writes a recovery copy through the same serializer, clears document roots only after recovery succeeds, and restores that recovery file if the requested document cannot be deserialized. Successful load clears selection and dirty state; a missing or failed load leaves the current editable document intact. The implementation does not add an editor-specific world format or change serializer payload shape.

### Content Browser v1 + Scene Entity Creation (Increment 40)

**`IAssetDatabaseUVE::GetRegisteredAssetsUVE()` is the sole Content Browser data boundary.** It returns copied `AssetRecordUVE` values while the database mutex is held, then sorts the snapshot by normalized lexical path with the 64-bit GUID as a deterministic tie-breaker. The browser therefore never scans the filesystem, infers that a registered path exists, or loads an asset as a side effect of drawing. This keeps the panel correct for first-run projects with an empty registry and makes its ordering stable despite the database's internal hash maps.

The bottom **Assets** panel exposes that read-only snapshot through a case-insensitive path filter, a single selected registry record, and selected-record path/GUID detail. Its editor state stores only the copied record and filter text, so it does not retain AssetDatabase internals or alter asset ownership. The File menu continues to invoke the existing scene save/load workflow; it does not introduce a separate content serialization format or a native file-dialog dependency.

The Scene menu creates a root-level document entity through `EditorUVE::CreateDocumentEntityUVE()`, selects it, and marks the document dirty. Every supported archetype receives `TransformComponentUVE`; **Empty** adds no other component, **Camera** adds `CameraComponentUVE`, **Directional Light** adds `LightComponentUVE` with `LightTypeUVE::Directional`, and **Collision Box** adds `ColliderComponentUVE`. The editor camera remains an excluded non-document entity. All exposed specialized components are already registered by `ISceneSerializerUVE`, so creating and saving them preserves the existing `.uvescene` persistence boundary.

### Scene Hierarchy v2: Entity Naming (Increment 41)

**`NameComponentUVE` is optional persistent scene metadata.** It contains a human-readable `std::string` and is registered by `ISceneSerializerUVE`, so names survive ordinary `.uvescene` save/load. Existing scenes without the component remain valid; the editor does not inject names while loading and renders the established entity-index/generation fallback instead. Names are not globally unique by contract.

Editor-created Empty, Camera, Directional Light, and Collision Box roots receive generated bases matching their archetype. When a generated base is already used by a live document entity, the editor selects the first deterministic numeric suffix such as `Camera 2`; it never scans or names the editor camera and never renames an existing manually named entity. `EditorUVE::SetSelectedEntityNameUVE()` validates lifecycle and document selection, rejects empty, whitespace-only, over-96-byte, or unchanged values, then adds or updates the component and marks the document dirty. The Scene panel uses this name for display with a unique hidden ImGui identity, while Properties exposes the bounded Name field above Transform controls.

### Viewport Navigation v1 (Increment 43)

**Viewport navigation belongs exclusively to the editor camera session state.** `EditorUVE` owns a finite focus point, yaw, pitch, distance, and transient Orbit/Pan gesture mode for its excluded `m_viewportCamera`. Orbit, pan, dolly/zoom, and focus apply the existing scene-graph local Transform path only to that editor camera. They never modify document entities, document selection, scene dirty state, `.uvescene` serialization, or Undo/Redo command stacks.

The transparent Viewport preserves explicit gesture precedence. An active left-button Translate or Rotate gizmo continues until its release; otherwise, right-button drag orbits around the current focus point, middle-button drag pans it by the current camera FOV and viewport pixel scale, and wheel input applies a clamped dolly distance. While the Viewport is hovered and no text field owns input, `F` focuses a selected live document entity with a clean derived world position. Invalid cameras, stale selections, invalid/dirty world transforms, invalid viewport rectangles, non-finite values, and distance/pitch limit violations fail without document mutation.

Camera navigation uses a 0.5–500 world-unit distance range and an 85-degree pitch limit. Existing left-button behavior remains gizmo-first then collider picking, so navigation gestures cannot select or move document entities. The Viewport help overlay is the authoritative control reminder: **LMB** select/drag axis, **RMB** orbit, **MMB** pan, mouse **wheel** zoom, and **F** focus selection.

### Editor History v1: Undo/Redo (Increment 42)

**History is editor-private session state.** `EditorUVE` owns bounded undo and redo deques with a default capacity of 100 entries; a zero constructor capacity is normalized to one. The editor records only successful state-changing Transform, Name, root-archetype creation, duplicate, delete, and reparent operations. Every new recorded command clears redo, and reaching capacity discards the oldest undo command. Core, Scene, runtime, and serializer layers remain independent of command-history types.

Every history entry stores its before/after selection and dirty-state values. Undo restores the prior mutation state, selection when still a live document entity, and dirty flag; redo restores the post-mutation equivalents. Transform and Name replay uses non-recording helpers so history cannot recursively append commands. Creation replay destroys the created root on undo and recreates its stored archetype/name on redo; a new entity handle is valid after recreation. Loading a document clears session history after the recovery copy has been saved and before document replacement begins, so stale handles from the previous scene cannot be replayed. An externally destroyed target, missing original parent, stale reparent parent, or failed subtree restoration causes history replay to fail without partial mutation and clears the unavailable command timeline.

The Edit menu exposes disabled-state Undo/Redo, Duplicate, and Delete actions. `Ctrl+Z`, `Ctrl+Y`, `Ctrl+Shift+Z`, `Ctrl+D`, and `Delete` are accepted only while ImGui does not own text input, avoiding interference with the Properties Name field. The View menu exposes Translate, Rotate, and Scale Gizmo modes, with guarded `W`, `E`, and `R` shortcuts. Lifecycle commands and mode changes are unavailable during an active transform-gizmo or viewport-navigation gesture. A Translate, Rotate, or Scale press-to-release interaction is a single Transform transaction: intermediate drag frames are non-recording previews, a changed release commits one history entry, and cancellation restores the initial Transform without recording a command.

### Transform Snapping v1 (Increment 48)

**Transform snapping is editor-private session state.** `EditorTransformSnappingSettingsUVE` defaults to disabled with finite positive increments of 1.0 world units for translation, 15 degrees for rotation, and 0.1 local-scale units for scale. Settings are never serialized to `.uvescene`, propagated to runtime state, or added to Undo/Redo history. The public setter rejects non-finite or non-positive increments and refuses all mutations while a gizmo drag or viewport-navigation gesture is active.

The View menu provides a guarded **Transform Snapping** submenu with an enable toggle and the supported discrete values: translation 0.25, 0.5, 1.0, or 5.0 world units; rotation 5, 15, or 45 degrees; and scale 0.05, 0.1, or 0.25 local-scale units. When enabled, direct transform commands and Translate, Rotate, and Scale gizmo gestures round their signed delta to the nearest configured increment. Gesture quantization is always calculated from the initial pointer and captured initial local Transform, so preview frames do not accumulate floating-point drift. A rounded zero delta makes no mutation and therefore records no history entry; a changed release still records exactly one Transform transaction.

### Viewport Selection Bounds v1 (Increment 49)

**Selection bounds are derived, read-only editor feedback.** `EditorUVE::TryGetSelectedBoundsUVE()` returns an oriented world-space box only for the selected live document entity with Transform, derived WorldTransform, and box Collider components. The query rejects dirty derived transforms, non-finite values, unsafe zero extents or scale components, and an invalid world quaternion without mutating selection, scene data, dirty state, camera state, or Undo/Redo history.

Each corner starts as one signed combination of the collider half-extents. The editor applies the derived component-wise world scale, rotates by the normalized derived world quaternion, then adds the derived world position. `DrawSelectionBoundsUVE()` projects all eight corners through the existing camera projection helper before drawing any part of the overlay. A failed projection skips the complete overlay rather than presenting a misleading partial box.

The transparent Viewport draws twelve restrained cyan edges, eight corner markers, and one center point through its existing ImGui draw list. Bounds draw after input handling and before the active gizmo, so they provide selection context without consuming pointer input or obscuring Translate, Rotate, and Scale interaction. No shader, render target, render graph node, serializer data, runtime state, preference, or history entry is introduced. Mesh-derived bounds and material-aware selection outlines remain separate rendering increments.

### Scene Hierarchy Reparenting v1 (Increment 45)

**One subtree moves through the existing scene graph.** `EditorUVE::ReparentSelectedEntityUVE()` moves the selected live document root below one valid live document parent, while the invalid parent sentinel returns it to the document-root level. The Scene panel exposes that command through a drag-and-drop payload that contains only an `EntityUVE`: dropping on a document node selects a new parent, while the explicit root drop target detaches the subtree. The private UI backend never writes hierarchy components directly.

**Transform and safety semantics are explicit.** Reparenting keeps the selected root’s authored local `TransformComponentUVE` unchanged and delegates dirty propagation to `ISceneGraphUVE::SetParentUVE()`; a future slice may add an explicit “keep world transform” mode. Editor preflight rejects absent scene-graph components, self-parenting, no-op parent choices, cyclic moves, non-document handles, stale handles, the viewport camera, and active gizmo/navigation gestures before it reaches the assertion-based scene graph API. The root’s descendants remain attached below it unchanged.

**Reparent history uses handles, not snapshots.** A successful move stores the live root, parent-before, parent-after, selection-before/after, and dirty-before/after in one editor-private entry. Undo and redo only call the non-recording graph mutation path; either replay fails and clears the unavailable history timeline if the moved root or the required parent is no longer a valid scene-graph document entity, or if an external edit makes the restored relation cyclic. The moved root itself remains selected after a successful direct move and after replay.

### Entity Lifecycle v1: Duplicate and Delete (Increment 44)

**Snapshot boundary.** `Scene::ISceneSerializerUVE::CaptureUVE()` creates an in-memory universal `.uve*` envelope and `RestoreUVE()` creates fresh entities from that same registered-component payload. Disk `SaveUVE()`/`LoadUVE()` share the payload encode/decode implementation, so lifecycle history cannot silently diverge from `.uvescene` hierarchy mapping, component coverage, or derived `WorldTransformComponentUVE` rebuild rules. Capture rejects invalid roots and unregistered component types before the editor changes any entity; malformed restore data rolls back entities it created.

**Hierarchy semantics.** Duplicate captures the selected root and all descendants, restores one fresh subtree, and attaches its root to the source root's current parent. A document root therefore duplicates as another document root; a child duplicates as a sibling under the same parent. When a valid root `NameComponentUVE` exists, its duplicate root receives the first available deterministic suffix through `MakeUniqueDocumentEntityNameUVE()`; descendants retain their snapshot names. Delete captures before destroying the selected subtree, then selects its still-live document parent or clears selection. Undo restores a deleted subtree below the original parent with a fresh root handle; redo destroys that current restored subtree. Duplicate undo destroys the current duplicate; redo restores a fresh duplicate under its stored parent. No replay path records a nested history command.

**Ownership and safety.** The editor-owned viewport camera is never a document entity and cannot be selected, captured, duplicated, deleted, or restored through lifecycle history. A lifecycle operation requires a running editor, a live selected document entity, no active gizmo drag, no viewport-navigation gesture, and a successful serializer capture. If an externally stale parent prevents restore, the operation fails safely and the unusable history timeline is cleared rather than attaching content to an arbitrary root.

**Deliberately deferred:** offscreen texture compositing of the viewport, mesh-triangle or render-ID picking, non-collider selection, mesh-derived bounds and final material-aware selection outline rendering, first-person/fly navigation, camera bookmarks, cinematic camera controls, local-axis and plane-translate handles, uniform/negative scale, free/trackball rotation, Play/Pause sandboxing, multi-selection, reflection-based/custom inspector drawers, native file dialogs, layout/preferences persistence, autosave policy changes, filesystem scanning, asset import/reimport, asset drag-and-drop, thumbnails, preview generation, hierarchy search/filter, in-place rename shortcuts, world-transform-preserving reparenting, hierarchy child ordering, multi-entity duplication/deletion, OS clipboard copy/paste, multi-entity naming, cross-session command logs, and branchable history. These remain separate increments so every editor slice stays buildable, testable, and compatible with the existing renderer.

**Validation expectation:** every Editor Foundation v1 change must build under GCC and Clang, retain the full headless CTest suite, run `uve_editor --headless --frames <n>` successfully, and exercise the real windowed overlay path under `xvfb-run` where a virtual display is available.
