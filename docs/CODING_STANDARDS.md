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
| `UVE::Asset`      | `engine/asset/`      | `AssetGuidUVE`, `AssetDatabaseUVE`, `ProjectFileIndexUVE`, `AssetContentFingerprintUVE`, `DerivedArtifactCacheUVE`, `AssetImportQueueUVE`, `AssetManagerUVE`, `AssetImporterUVE`, `HotReloadUVE`, `AssetBundleUVE`, `FileSystemUVE`, the `.uve*` binary envelope |
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

**Increment 58 retires the EngineCore demo-triangle scaffold completely.** `EngineCoreUVE::Render()`
now delegates scene rendering to `Renderer3DUVE`, then invokes the non-owning post-render callback,
and finally presents through `IRenderDeviceUVE::PresentUVE()`. `Renderer3DUVE` owns the sole
scene-to-window path: its existing HDR main color/depth targets, shadow pass, canonical tone-mapping
pass, and default-framebuffer presentation are shared by asset-backed meshes and renderer-owned
built-in primitives. No engine-core draw buffers, shader programs, or proof-of-life geometry may be
reintroduced; visible scene content must enter through ECS extraction into `Renderer3DUVE`.

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

**Overlay ordering is explicit.** `EngineCoreUVE::SetPostRenderCallbackUVE()` is a generic, non-owning application-overlay seam with no editor or GUI types in the core API. In a windowed frame, `Renderer3DUVE` performs the HDR `MainColor -> ToneMapping` graph work and writes the default framebuffer; a registered overlay callback then draws; and only then does `IRenderDeviceUVE::PresentUVE()` swap the back buffer. The runtime does not register a callback, so its frame behavior remains unchanged. Headless runs never invoke the callback.

**Dear ImGui is a pinned, editor-private dependency.** CMake `FetchContent` retrieves version `v1.91.5` at commit `f401021d5a5d56fe2304056c391e78f81c8d4b8f`, following the repository's established GLFW/GoogleTest dependency pattern. Its GLFW/OpenGL3 backend sources link only into `uve_editor_imgui`; all ImGui headers and types are confined to `engine/editor/src/editor_uve.cpp`. Third-party code is deliberately compiled outside UVE's warning-as-error policy, while all UniVex editor source remains subject to it.

### Editor Bridge Contract v1 (Increment 69)

**`EditorBridgeUVE` is a main-thread adapter over `EditorUVE`, never a second editor authority or a replacement for native ImGui.** It accepts and returns only documented value DTOs: `EditorBridgeEntityRefUVE` carries the copied `{ index, generation }` identity pair, while snapshots and responses hold copied strings, selections, lifecycle state, capability values, and diagnostics. No bridge header may expose raw ECS pointers, references, renderer handles, OpenGL resources, or private editor storage. The bridge obtains all visible state from `EditorUVE`; C# or any future client remains a presentation/control surface, not an owner of scene data, serialization, RHI state, or history.

**Revision is the bridge's stale-state boundary.** `EditorBridgeUVE::SynchronizeRevisionUVE()` captures the full v1 bridge-visible fingerprint—editor lifecycle, Play Mode, dirty state, Undo/Redo availability, active scene path, ordered selection with display labels, and active entity—before every snapshot or request response. A changed fingerprint increments the nonzero revision whether its cause was native ImGui, viewport interaction, lifecycle work, Undo/Redo, or a bridge-routed command. Every mutating request must carry the exact current `expectedRevision`; an older value deterministically rejects with `bridge.snapshot.stale` and cannot mutate editor state. Read-only snapshot requests require no optimistic-concurrency token.

**The v1 protocol is explicit and forward-compatible.** `kEditorBridgeProtocolVersionUVE` is checked before dispatch; an unsupported version returns `bridge.protocol.unsupported`. Stable response codes distinguish unsupported protocol, non-running editor, stale snapshot, invalid entity, malformed request, rejected command, successful command, and snapshot read. Capabilities enumerate the implemented contract rather than implying unimplemented UI or rendering features. Later bridge revisions must add fields and capabilities compatibly or negotiate a new protocol version; they must not silently reinterpret existing values.

**All bridge mutations delegate to existing public `EditorUVE` command paths.** Selection, clear selection, selected-name editing, document-entity creation, Undo, and Redo preserve the native command layer's existing lifecycle, document-entity, Play Mode, dirty-state, validation, and history behavior. A C# host must never write `.uvescene` files, mutate ECS components directly, own a GL context/resource, or bypass these commands. Increment 69's in-memory adapter still observes native-ImGui-visible revision changes, so an in-process client must fetch a fresh snapshot after any revision mismatch rather than assuming it is the only control surface.

### Local Framed Bridge Host v1 (Increment 70)

**The shipped `--bridge-stdio` path is mutually exclusive with native ImGui for one process and one `EditorUVE` instance.** The flag always selects headless startup, therefore creates no GLFW window, ImGui context, renderer overlay, or OpenGL resource. Ordinary `uve_editor` startup creates no stdio bridge server. Do not introduce an option that shares a live native-ImGui `EditorUVE` with a stdio client; a future co-hosting design requires a separately approved lifecycle and thread contract.

**Protocol stdout is reserved exclusively for bounded framed JSON-RPC.** `EditorBridgeStdioServerUVE` reads and writes a four-byte network-order length followed by one UTF-8 JSON object, rejects zero or oversized frames before allocation, routes diagnostic logging to stderr/file sinks, and never accepts generic raw scene commands. The wire adapter maps known values into the existing `EditorBridgeRequestUVE` DTO and delegates only to `EditorBridgeUVE`; malformed frames, JSON, unknown methods, and incompatible versions produce deterministic bridge diagnostics without editor-state mutation. The v1 frame taxonomy is intentionally non-overlapping: `bridge.transport.frame.zero_length`, `.oversized`, `.truncated_header`, and `.truncated_body` identify malformed length/framing; `bridge.transport.eof` means the peer closed before any response header; `bridge.transport.json.invalid` identifies a complete but invalid JSON body; and `bridge.transport.response.invalid` identifies an invalid JSON-RPC response envelope. A client must display or log the stable code and must not infer recovery from the text message.

**The managed host owns only one child process and copied response values at a time.** It launches `uve_editor --bridge-stdio` through an argument list, drains stderr separately, performs `bridge.hello` before reporting a connected state, and closes streams before a bounded child-only termination fallback. The host serializes launch and disposal through one lifecycle gate: a new backend may start only from `Disconnected` or a launch-failed `Failed` state with no owned session; `Connecting`, `Connected`, failed-owned, and confirmation states reject a second launch. Replacement starts only after explicit acknowledgement and completion of prior-session disposal. There is no automatic reconnect or restart. A backend crash, EOF, timeout, or protocol failure puts the host into an error state; the user must explicitly acknowledge **Start New Backend Session** because the replacement process owns a new `EditorUVE` and v1 provides no Save command. Last-known `sceneDirty` is displayed as evidence only, never as a recoverability promise. The first terminal failure owns the visible failure state: duplicate late EOF/process-exit callbacks cannot overwrite a `Failed` or fresh-session acknowledgement state, while a failure during dirty-close confirmation supersedes that confirmation. A dirty connected session similarly requires an explicit discard confirmation before host shutdown.

### C# Editor Shell & Docking v1 (Increment 71)

**The managed shell is fixed-region presentation, not a second editor backend.** It may own its local menu/workspace selection, fixed left/center/right/bottom dock visibility, splitter dimensions, and active tab identifiers. It must not create a second `BridgeBackendSession`, launch a child outside `MainWindow`'s lifecycle gate, send a bridge request for layout interaction, change bridge revision, or add scene dirty/history state. The only v1 regions are Scene, Viewport, Inspector/Import/Signals, and FileSystem/Debugger/Animator/AI Toolbar. Floating/tear-off docks, arbitrary window graphs, plugin-owned panels, full C# scene authoring, C# import mutations, native viewport hosting, and GL ownership remain separate milestones.

**Managed shell layout is application-local and explicit-save-only.** `DockShellPreferencesStore` persists a versioned JSON value outside project content, `.uvescene`, `.uveassetdb`, and native `.uvesettings`. It writes only from the explicit **Save Shell Layout** action, using atomic replacement; startup, resize, tab/workspace selection, bridge reconnect, crash/EOF, dirty-close flow, and clean host close never write it. The visible dirty indicator is required whenever current layout differs from the last explicitly saved layout. A recognized v0-shaped record migrates in memory to v1 by retaining valid legacy dimensions/workspace/tabs and filling v1 visibility fields from defaults; migration itself never writes back. Current v1 records validate and normalize known fields. Corrupt, unreadable, unrecognized unversioned, or unknown future-version records leave their source file untouched and use safe v1 defaults. Every future known layout schema change must supply an explicit tested migration instead of discarding recognized preferences.

**Panel cards must be honest about capability and ownership.** An Increment 71 card may describe its deferred implementation and backend connection state but must not imply that a C++ capability was received or that a panel can mutate data. Native ImGui remains the independent C++ fallback; it does not share a process, `EditorUVE`, ImGui context, renderer resource, or OpenGL context with the managed shell.

### C# Hierarchy, Inspector & Content Browser v1 (Increment 72)

**Managed editor panels render typed immutable copied snapshots only.** `BridgeSnapshotParser` must validate every known panel field before UI state is replaced; no `JsonElement`, native pointer, ECS reference, filesystem handle, OpenGL resource, callback, or mutable engine object may escape into panel state. Unknown additive fields may remain forward-compatible, but missing/wrong-typed known fields, unsupported enum values, malformed identifiers, oversized ordinary presentation text or the separately bounded native-validated content path identity, and arrays above `kEditorBridgeMaximumPanelEntriesUVE` / `BridgeSnapshotParser.MaximumPanelEntries` must fail deterministically as `bridge.snapshot.invalid`, never render a partial state. Every copied bounded collection requires an explicit truncation fact whenever omission is possible; a UI must show that fact and must never imply omitted entities/content were deleted.

**C++ remains the source of hierarchy, Inspector, and content semantics.** Native `EditorUVE` computes depth-first hierarchy ordering, parent identity, type-tag priority, selection/active facts, eligible drawer identifiers, Inspector no/single/multi-selection mode, current content directory, breadcrumbs, cached project-index rows, filter/focus matching, extension type labels, and asset registration correlation. Managed code may present these facts and send only normalized user intent through named bridge commands; it must not rescan paths, derive alternative hierarchy/asset semantics, trust a path without native validation, write project files, or manufacture arbitrary property/component commands.

**Every Increment 72 action is an optimistic-concurrency C++ command.** Selection, selected-name update, hierarchy filter, cached content directory/filter/focus, explicit cache refresh, and cached-entry selection require the exact copied revision and return a freshly synchronized snapshot plus stable code/message. `bridge.snapshot.stale` is normal recoverable feedback: discard the local action result, replace presentation with the returned snapshot, and require a fresh user action. The managed `BridgeBackendSession` serializes refresh and dispatch requests for its one owned child; do not pipeline framed protocol traffic or add a second session/transport owner. Disposal waits for an in-flight request before closing the process streams.

**Scope remains deliberately narrow.** The C# Scene panel does not create/delete/reparent entities; the Inspector does not perform generic component reflection or transform editing; the FileSystem panel does not import, write, or rescan independently; and the center viewport does not host native pixels or GL. Subsequent increments must add each wider workflow only with its own authoritative native command/capability, regression evidence, and lifecycle design.

The fixed workspace layout has a compact **UNIVEX** header with session-only Library, Asset, Scripting, Debug, and Plugin labels; a left **Scene** panel; a center **Viewport** region; a right panel with horizontally arranged **Inspector**, **Import**, and **Signals** tabs; and a lower dock. Native ImGui retains the interactive renderer-backed 3D viewport. The managed shell deliberately presents a deferred viewport card until Increment 74 supplies a separately approved surface lifecycle. The active lower dock defaults to **FileSystem**, which presents the cached read-only `ProjectFileIndexUVE` snapshot for the configured project content root.

### GLSL Viewport Visuals v1 (Increment 73)

**Editor viewport visuals are renderer-owned, additive, and value-driven.** `Renderer3DUVE` may insert the bounded `EditorViewportVisuals` composite pass after the HDR scene pass and before tone mapping. The pass consumes only copied normalized viewport/selection/camera/gizmo facts from `EditorViewportVisualStateUVE`; it must never inspect ECS pointers, own editor input, mutate scene state, or create a second presentation path. The pass is disabled by default, and runtime/game rendering must remain unchanged when no editor visual state is published.

**Blend and pass ordering are explicit.** Alpha composition uses an opt-in `PipelineBlendModeUVE::SourceAlphaOver` state that is set when the pipeline binds and is explicitly disabled for opaque pipelines. The editor pass uses `Load` for the existing HDR color target and never clears scene color or depth. It must execute before the existing fullscreen tone-mapping pass, and every new built-in GLSL file must have a byte-identical embedded fallback covered by parity tests.

**GLSL owns appearance, not interaction.** The shader may draw analytic grid/major-axis accents, bounded selection outlines, axis-widget strokes, and gizmo-state markers. C++ remains authoritative for projection, selection bounds, camera facts, gizmo input/active-axis semantics, and diagnostics. Native ImGui remains the input and text-label owner; managed C# never owns these OpenGL resources or receives raw GPU handles. Any later viewport surface hosting, GPU picking, or scene-authoring interaction requires a separately reviewed increment. Debugger, Animator, and AI Toolbar are selectable dock labels only until separate runtime diagnostics, animation, and AI-tooling contracts are implemented.

### Managed Viewport Surface Lifecycle v1 (Increment 74)

**The C++ bridge owns the viewport surface lifecycle.** `EditorBridgeViewportSurfaceStateUVE` is the authoritative state enum: `Unavailable`, `NativeOwned`, or `Detached`. A headless bridge session must report `Unavailable`, `nativeRendererOwnsSurface=true`, and `managedAttachAllowed=false`. The snapshot is descriptive only; it never transfers a window handle, GL context, texture/FBO identifier, picking pass, input-forwarding path, or ECS/resource pointer across the C++↔C# boundary.

**Managed code must not infer or acquire a surface.** `managedAttachAllowed` is required to be `false` for every current bridge session, and the managed snapshot parser must reject any `true` value as `bridge.snapshot.invalid`. `BridgeViewportSurfaceSnapshot.IsPresentableToManagedHost` may become true only in a future, separately reviewed increment that introduces an explicit capability, ownership, teardown, input, GPU-resource, and failure-recovery contract.

**`ReadViewportSurface` is read-only.** Its capability and request may observe the copied surface snapshot, but it does not mutate document state or the bridge revision and therefore is excluded from stale-revision checking. Do not turn an observation request into an implicit attach, render-target allocation, input subscription, or layout persistence operation. All future managed surface hosting requires explicit native capability discovery and regression coverage before implementation.

None of these workspace or panel-selection values are serialized, added to Undo/Redo, or allowed to mutate document state by themselves. Hierarchy labels prefer a persistent `NameComponentUVE` when present and otherwise retain the stable entity-index/generation fallback for legacy and runtime-created entities. Selection has one shared owner in `EditorUVE`: native hierarchy clicks, viewport picking, the native Inspector, and gizmos all operate on the same live native selection. The managed Inspector can issue the delivered selected-name command only; broader property/Transform editing remains a separately scoped workflow.

### Viewport Picking + Transform Gizmos v1 (Increments 39 and 46)

The viewport remains an editor input/overlay rectangle over the renderer-owned default framebuffer; it does **not** create a duplicate scene texture, render target, or presentation path. The editor-private ImGui backend reads pointer state directly so overlay interactions remain isolated from runtime action mappings. `EditorUVE::MakeViewportRayUVE()` maps a valid pointer within the active viewport rectangle through the editor camera's FOV/aspect data and read-only `WorldTransformComponentUVE` into a normalized `Math::RayUVE`. Invalid camera state, stale/dirty world transforms, invalid projection values, non-finite coordinates, or out-of-viewport coordinates return no ray.

`EditorUVE::PickViewportUVE()` composes that ray with the existing deterministic `IRaycastSystemUVE`. The selection boundary is intentional: only live document entities with the existing `WorldTransformComponentUVE` plus box `ColliderComponentUVE` can be picked. The editor camera and non-document entities cannot become selections. A regular viewport click replaces selection and a regular miss clears it; Ctrl-toggle picking adds/removes a valid hit and preserves selection on a miss. Mesh-triangle/renderer-ID picking and non-collider selection remain future work rather than being approximated by an imprecise fallback.

`EditorUVE` exposes first-pass **Translate**, **Rotate**, and **Scale** modes only when exactly one live document entity is selected. Handle hit-testing retains priority over scene picking; multi-selection fully hides the gizmo and rejects all handle hit tests and drag starts. Translate projects pointer movement onto a chosen screen-space axis and converts the resulting world-space delta back to local position; parent world rotation and scale are accounted for before calling the existing scene-graph transform setter. Rotate renders sampled X/Y/Z ring polylines in the active coordinate-space basis, hit-tests their projected segments, and converts a signed captured world-axis angular delta into the selected entity's local quaternion. For a parent world rotation `P`, world delta `W`, and authored local rotation `L`, the local update is `inverse(P) × W × P × L`; root entities use `W × L`. The result is normalized before it reaches `ISceneGraphUVE::SetLocalTransformUVE()`.

Scale uses projected active-coordinate-space axis handles for interaction but changes only the matching authored positive `localScale` component. **Increment 54 adds a center-handle Uniform Scale Offset:** after axis endpoint priority, it captures one radial screen-space scalar and additively applies that same delta to all three authored local-scale components from the drag's initial Transform. It is deliberately not a proportional/multiplicative scale operation. The complete gesture cancels and restores the initial Transform and dirty state if any proposed component is non-finite or below `kMinimumLocalScaleUVE`; no per-component clamping or partial update occurs. Each transform mode rejects deleted entities, non-finite input, invalid or dirty parent transforms, zero/invalid quaternion magnitudes, unsafe projection, and active competing gestures without mutation. Scale additionally rejects a proposed component below the documented positive minimum, so v1 never creates zero, negative, or mirrored local scale. A left-button drag is a single Transform history transaction: non-recording updates preview the initial Transform, changed release records one entry, and cancellation restores the initial local Transform plus dirty state. The `W`, `E`, and `R` mode shortcuts, and View-menu controls, are rejected while a text field owns input or an active gizmo/navigation gesture exists. **Increment 51 extends this contract with session-only World/Local coordinate space and XY/XZ/YZ Translate plane handles.** World remains the default and preserves canonical-axis behavior. In Local mode, each named handle axis derives from the selected entity's normalized, non-dirty `WorldTransformComponentUVE::worldRotation`; the editor deliberately does not decompose an affine world matrix, so finite non-uniform parent scale cannot introduce a sheared local basis. Invalid, dirty, zero-length, or non-finite derived transform state rejects local-handle interaction safely. Coordinate space is neither serialized nor recorded in history, and cannot change during a drag, viewport navigation, Playing, or Paused state.

**Increment 64 makes transform gestures explicit Editor Tool Sessions.** `GizmoDragUVE` remains the handle-hit and pointer-solver record, while `EditorToolSessionUVE` exclusively owns the copied target, baseline local Transform, baseline dirty state, last successfully applied preview, and terminal outcome. A second Begin during `Previewing` is a strict no-op rejection: the existing gesture, preview, dirty state, and pointer ownership stay untouched. Every preview remains baseline-derived and reaches the scene graph before the session records it. Commit performs no second transform write; it verifies the live authored transform still equals the validated last preview, then records exactly one existing Transform history entry only when changed. A zero-delta session restores/retains baseline dirty state and records no history. Cancellation first compares the live authored Transform with the session's own last successfully applied preview. A mismatch is `ExternalTransformConflict`: the session clears, preserves the external Transform, marks the document dirty, and records no history—never restoring an old baseline over another mutation. Only a matching preview permits one checked baseline restore; if that restore fails while the target remains live, the session clears as `RestoreFailed`, leaves dirty true, and records no history rather than falsely claiming atomic rollback. Deleted or stale targets are never recreated or mutated.

**Increment 65 strengthens Outliner and Inspector workflow without creating a second hierarchy authority.** A live document row carries exactly one session-only specialized tag, using the fixed priority Plane → UV Sphere → Cube → Camera → Directional Light → Collision Box; a matching collider never hides a primitive’s visual identity. Row root/child and child-count context is read-only. The Inspector's ancestry breadcrumb is deliberately read-only in this increment, so it cannot change selection during an active tool session. The `hierarchy` Inspector drawer may display the current parent, ancestry, existing Keep Local/Keep World preference, eligible parent candidates, and Make Root affordance, but it routes every successful move exclusively through `ReparentSelectedEntityUVE()`. Self and descendants are absent from candidates, the current parent is disabled, and only one existing reparent history transaction may result. Multi-selection, stale selections, Play/Pause, active transform/navigation sessions, and invalid hierarchy state remain non-mutating.

Translate plane handles occupy the positive 20–60% square of each active single-selection axis pair. Axis and endpoint candidates are evaluated before any plane candidate—this is an interaction contract, not merely draw order. A plane drag captures both world axes and solves pointer displacement through its 2×2 projected screen basis; snapping independently rounds both world coefficients before the resulting combined world delta passes through the existing parent-safe local-position conversion.

**Increment 55 adds a camera-oriented Free Rotation Trackball.** XYZ ring hit testing completes first; only a ring miss can capture the inner projected disc. Pointer coordinates map to a unit virtual sphere, clamping positions outside the visible disc to the sphere edge (`z = 0`) so active drags remain defined beyond the disc. The delta comes from the cross product and clamped dot product of captured/current sphere vectors, with the resulting camera-space axis rotated into world space before the existing parent-safe local quaternion conversion. A near-zero cross product with dot near `+1` is a zero delta; the same cross-product degeneration with dot near `−1` is an ambiguous antipodal/near-180-degree drag and cancels atomically, rather than choosing an unstable fallback axis. Negative scale, proportional/multiplicative scale, snapping refinements, and final selection-outline rendering remain deferred.

**Scene documents continue to use only `ISceneSerializerUVE`.** Saving serializes document roots as `.uvescene` with `AssetKindUVE::Scene`; the editor camera is explicitly excluded. Loading first writes a recovery copy through the same serializer, clears document roots only after recovery succeeds, and restores that recovery file if the requested document cannot be deserialized. Successful load clears selection and dirty state; a missing or failed load leaves the current editable document intact. The implementation does not add an editor-specific world format or change serializer payload shape.

### Workspace Layout v1 (Increment 57)

**The workspace header replaces legacy top-level File/Edit/View/Scene menus without removing editor command APIs.** `EditorUVE` retains its public Save/Load, creation, transform, history, and Play Mode operations, while the visible header presents only session-local Library, Asset, Scripting, Debug, and Plugin workspace labels plus compact guarded Play/Stop controls. The 3D Viewport itself contains no duplicate application menu controls; it remains a renderer-backed input/feedback rectangle with its viewport-local navigation and gizmo affordances only.

**Right tabs and lower docks are composition state, not functionality claims.** Inspector, Import, and Signals are horizontal tabs within one fixed right panel. Only Inspector owns the existing entity Name/Transform mutation surface. Import exposes copied `AssetImportQueueUVE` job monitoring only; it deliberately does not expose an enqueue or retry mutation surface. Signals communicates its unavailable scripting integration instead of faking bindings. FileSystem is the default lower dock and exposes only copied cached project-file entries plus existing AssetDatabase GUID correlation. Selecting Debugger, Animator, or AI Toolbar changes lower-workspace presentation without enabling unsupported service ownership, background jobs, scene mutation, or history entries. The `+ Add Dock` affordance is explicitly disabled until layout persistence and real dock registration are introduced.

**Layout safety.** The header, Scene, 3D Viewport, right panel, lower dock content, and dock strip are deliberately fixed to one coherent session layout. They reserve the ImGui main-menu height and bottom-dock height before computing the central editor rectangles, preventing workspace windows from overlapping top controls. Headless `RenderOverlayUVE()` remains a no-op and must leave selection, dirty state, and lifecycle unchanged.

### Viewport Scene Rendering v1 (Increment 58)

**`PrimitiveMeshComponentUVE` is the serializable scene boundary for built-in meshes.** It stores only a
validated `PrimitiveMeshKindUVE` (`Cube`, `UVSphere`, or `Plane`) and a finite, bounded `[0,1]`
linear-RGB `baseColor`. It never embeds vertex/index arrays, material assets, GPU handles, or editor-session
state. `ISceneSerializerUVE` registers the component like every other scene component; malformed kind or color
payloads reject the entire restore transaction and roll back all entities created for that transaction. This keeps
`.uvescene` documents compact, deterministic, and forward-compatible with renderer-owned geometry revisions.

**`GetPrimitiveGeometryUVE()` owns immutable CPU geometry, not scene data.** All three unit-scale entries use
`Asset::MeshVertexUVE`, preserving the same position/normal/UV/tangent layout and tangent-generation path as
asset-backed meshes. Cube is authored with face-separated vertices, Plane is origin-centered on XZ with explicit
`+Y` winding, and UV Sphere uses 24 slices × 16 stacks, a duplicated seam column (`u = 0` / `u = 1`), and
per-sector pole vertices with exact unit normals. The catalog returns stable process-local cached geometry; only
one copied vertex stream per primitive kind is tangent-generated and uploaded to `Renderer3DUVE` GPU buffers.

**`Renderer3DUVE` is the exclusive primitive draw owner.** Primitive extraction requires a non-dirty
`WorldTransformComponentUVE` and a valid primitive component, composes world TRS, frustum-culls the transformed
catalog bounds, and records the surviving items in the existing HDR main pass after asset-backed opaque and
transparent items. The primitive program uses `basic_3d.glsl` with canonical mesh vertex layout and exactly three
per-draw uniforms: `uModel`, `uViewProjection`, and `uColor`. Built-in primitives must not set material, texture,
light, or shadow uniforms that the Basic3D contract does not declare. Primitive GPU buffers are cached by kind and
destroyed with the renderer; neither `EngineCoreUVE` nor `EditorUVE` owns RHI resources or issues direct draw calls.

**Editor authoring is deliberately narrow and atomic.** Library workspace controls create selected Cube, UV Sphere,
and Plane document roots with a `PrimitiveMeshComponentUVE` and matching selectable collider. Cube and UV Sphere
use `{0.5, 0.5, 0.5}` half extents; Plane uses `{0.5, 0.025, 0.5}`. With exactly one primitive selected, Inspector
kind/color editing is validated as one `PrimitiveAppearanceHistoryEntryUVE`: the entire component snapshot and
collider dimensions change together, and Undo/Redo applies the before/after snapshot atomically. Multi-selection,
Play/Pause, invalid values, stale selection, unchanged state, and competing editor gestures reject the edit without
mutation. The XZ ground grid is a 10 × 10, one-unit session-only projected overlay; it is not scene data,
selectable content, or history.

**Scope boundary.** This increment supplies ECS-backed visible primitives, not a complete viewport composition or
asset pipeline. The viewport still overlays UI over default-framebuffer presentation; texture compositing, render-ID
or mesh-triangle picking, non-collider selection, mesh-derived selection bounds, fly navigation, camera bookmarks,
import/reimport, drag-and-drop, and thumbnails remain separate work. **Increment 59 adds a read-only project-content
index, while Increment 61 adds a separate portable observation journal and targeted metadata invalidation**; neither
increment adds source mutation, automatic import/reimport, or asset previews.

### Viewport Presentation & Render Verification v1 (Increment 63)

**`Renderer3DFrameDiagnosticsUVE` is evidence, not a pixel-readback claim.** `IRenderer3DUVE` returns a copied
per-frame snapshot with primitive candidates/extractions, mesh and primitive draw calls recorded, the number of
OpenGL-issued draw calls, program readiness, and main/tone-mapping pass recording. It is reset at frame start and is
safe for tooling, tests, and status UI to read after rendering. `glDrawCallsIssued` is populated only for the OpenGL
backend and confirms command issuance only; it must never be interpreted as proof that any particular pixel reached
the default framebuffer. `EditorUVE` displays only compact frame evidence—primitive count and recorded/issued draw
counts—in the viewport overlay.

**A requested depth clear is state-independent.** `GlCommandBufferUVE::BeginRenderPassUVE()` restores
`GL_DEPTH_WRITEMASK` before `glClear(GL_DEPTH_BUFFER_BIT)` whenever `RenderPassDescUVE::depthLoadOp` is `Clear`.
A fullscreen tone-mapping pipeline correctly disables depth writes, but that prior state must not turn the next
HDR main pass's depth clear into a silent no-op. Pipeline binding remains the authority for the depth-test and
write-mask state used by each draw after the clear.

**Viewport presentation is verified at the right boundary.** Real-GL tests read `GL_BACK` in the
`EngineCoreUVE` post-render callback, after the tone-mapping pass and before swap, rather than treating a
post-swap read as deterministic. The fixture asserts three independently colored ECS primitives at interior raster
coordinates, alongside renderer evidence. Separate RHI regressions cover Basic3D compilation through the mounted
virtual shader path, canonical indexed layouts, HDR-to-default-framebuffer tone mapping, and the persisted
post-tone-map depth-mask transition.

### Project FileSystem & Asset Browser v1 (Increment 59)

**`ProjectFileIndexUVE` is a read-only cached project-content snapshot, not the virtual filesystem or an import pipeline.** `EngineConfigUVE::projectContentRootUVE` defaults to `assets/`; it is normalized at index construction and never created by the index. A relative trailing separator is spelling-only, so `assets/` and `assets` produce the same root boundary and cached snapshot rather than an empty terminal path component. `ProjectChangeWatcherUVE` applies the same normalization before building its independent baseline, ensuring that index and watcher never disagree about the configured root. `RefreshUVE()` is the only operation that performs filesystem enumeration. `GetSnapshotUVE()` returns a copied cached value and performs no I/O, so the editor refreshes exactly when its FileSystem dock first opens and when the user presses **Refresh**, never once per overlay frame or on a hidden timer. A missing or empty root publishes a successful empty snapshot. An inaccessible root, iterator error, or symlink root reports refresh failure and preserves the last successful snapshot.

**The root boundary is strict.** The index calls `symlink_status` for every root and child entry, omits all symlinks, and never follows even one hop. It consequently cannot escape the configured content root through a symlink and cannot recurse into a symlink cycle. Only ordinary directories and regular files are retained. Traversal uses error-code handling with `skip_permission_denied`; an unexpected iterator error rejects the refresh atomically rather than publishing a partial tree.

**Entries are presentation values with deterministic ordering and one-way registry correlation.** Every `ProjectFileEntryUVE` stores a normalized root-relative path, file/directory kind, and an optional `AssetGuidUVE`. Directories sort before files; each kind then sorts by generic lexical path. `AssetDatabaseUVE` remains the sole GUID authority: only existing normalized registry paths physically inside the content root give a regular-file entry a GUID. Stale or out-of-root registry rows neither create tree entries nor affect scanning. The index never loads an asset, mutates the registry, mounts a VFS path, or changes scene data.

**The FileSystem dock is read-only session UI.** Increment 66 presents the copied snapshot as a current-folder browser: **Up** and clickable root-to-folder breadcrumbs navigate only paths that are present as directories in that snapshot; a missing current directory after a successful refresh safely falls back to the content root. The browser renders direct children in the index's deterministic folders-first lexical order. Each row has exactly one extension-derived primary tag—`Folder`, `Scene`, `Prefab`, `Bundle`, `Mesh`, `Texture`, `Shader`, `Material`, `Save`, or `File`—plus an independent optional `Registered` badge sourced from existing AssetDatabase correlation. Registration never replaces the semantic type tag. Case-insensitive text filtering and the type-focus selector are session-only and persist through navigation; a visible zero-match message identifies when active filters exclude every direct child. It stays usable in Play/Pause because navigation, filtering, Refresh, and selection do not mutate document state, dirty state, Undo/Redo, or sandbox state. Increment 61 adds separate watcher-journal feedback to this dock, but the dock itself still does not implement import/reimport, previews/thumbnails, drag-and-drop, rename/move/delete/create-folder operations, native file dialogs, native directory-event backends, or source-control commands. Those features require separate ownership and transactional policies in later increments.

### Import Work Queue & Derived Artifact Cache v1 (Increment 60)

**`AssetImportQueueUVE` is a deterministic main-thread scheduler, not a background content cooker.** `EnqueueUVE()` only validates and snapshots a request; it does not read or write files. `EngineCoreUVE::Update()` calls `TickUVE()` once, and one tick transitions at most one oldest queued job through `Running` to a terminal state. The executor delegates to the existing `IAssetImporterUVE`, so Increment 60 does not add model, image, audio, or other format-specific parsers. Jobs expose copied snapshots only. Failed `RetryUVE(id)` calls preserve the stable id but append the job to the **FIFO tail**, never jumping ahead of earlier queued work; attempt count increments only once the retry reaches the head and executes.

**Fingerprints and cache validity are byte-based, not wall-clock based.** `ComputeAssetContentFingerprintUVE()` uses a deterministic non-cryptographic 64-bit FNV-1a byte hash plus byte count. It is a cache identity signal, not a security digest. A cache record persists normalized source/destination paths, source and destination fingerprints, importer settings version, schema version, and the authoritative `AssetGuidUVE`. A hit requires every one of those metadata fields to agree, the destination to exist, the current destination bytes to match the recorded destination fingerprint, and `AssetDatabaseUVE` to resolve the recorded GUID to that same normalized destination. Consequently, an out-of-band destination-file edit invalidates the record and forces an importer execution in v1.

**`DerivedArtifactCacheUVE` owns only generated metadata under `EngineConfigUVE::derivedArtifactCacheRootUVE`, defaulting to `DerivedData/Import/`.** It creates that root lazily only for a successful metadata write, rejects a symlink cache root, creates no source/destination directories, and writes deterministic `.uveimportcache` JSON artifacts through a temporary sibling followed by rename. A malformed, missing, wrong-schema, or stale artifact is an ordinary cache miss; it never mutates `AssetDatabaseUVE`. A cache write failure after a successful importer result preserves `Succeeded` state but appends a structured warning, since the output and GUID are valid even though future reuse is unavailable.

**The editor Import tab remains observability-only.** It displays copied queue snapshots—id, state, source, destination, attempt count, cache-hit result, GUID, and structured diagnostic messages—but exposes no file picker, enqueue/retry/cancel buttons, drag-and-drop, automatic reimport, or document/history mutation. The monitor remains safe in Play/Pause because it only reads service snapshots. Native dialogs, user-facing enqueue/retry, import settings drawers, background import workers, cancellation, and generated thumbnails/previews remain future work; Increment 61's project-change watcher is intentionally separate and still owns no queue mutation.

### Project Change Watch & Targeted Reload v1 (Increment 61)

**`ProjectChangeWatcherUVE` is a portable, main-thread project-content observer, not `IHotReloadUVE`, `ProjectFileIndexUVE`, or an importer.** `EngineCoreUVE` constructs it after `DerivedArtifactCacheUVE`, publishes it through `EngineServicesUVE`, and calls `PollUVE()` after the loaded-asset hot-reload poll but before garbage collection and the one-job import-queue tick. It owns no worker thread, OS-native watcher backend, project-index reference, importer, queue, or authoring command. `EngineConfigUVE::projectChangeWatchPollIntervalSecondsUVE` defaults to one second and bounds enumeration frequency; a zero interval intentionally polls every engine update. `EngineConfigUVE::projectChangeJournalCapacityUVE` defaults to 256 retained entries and makes journal memory bounded and review behavior explicit.

**One successful scan atomically replaces a private physical baseline and appends deterministic copied changes.** The watcher enumerates only ordinary directories and regular files under the configured root, uses `symlink_status`, and never follows symlinks. A missing root is a successful empty baseline. Enumeration, inspection, or fingerprint failure retains the last successful baseline and exposes only a copied diagnostic. Changes are sorted lexically by root-relative generic path, then kind; a rename remains a deterministic remove-plus-create pair. Each baseline stores the optional in-root AssetDatabase GUID seen at scan time, so a later Removed event retains that GUID without querying a stale registry. `GetSnapshotUVE()` performs no I/O and returns independent copied journal state; `PollNowUVE()` exists as an explicit deterministic test/review seam and never refreshes the ProjectFileIndex or queues work.

**Journal overflow is a correctness boundary, not silent loss.** The bounded journal retains newest records and permanently sets `rescanRequired` when it must evict an older entry; capacity zero records no ordinary entries and immediately establishes the same boundary. The watcher still calls `DerivedArtifactCacheUVE::MarkStaleForSourceUVE()` for every detected regular-file change after overflow, preserving targeted reuse invalidation even when exhaustive UI review is no longer possible. `AcknowledgeThroughUVE(sequence)` removes only retained ordinary entries at or below the sequence and never clears `rescanRequired`. Only the editor flow that has just received a successful full `ProjectFileIndexUVE::RefreshUVE()` may call `AcknowledgeRescanUVE()`; that explicit operation clears the overflow boundary and retained journal. A failed refresh acknowledges nothing.

**Stale derived metadata is an ordinary cache miss.** A cache record persists `stale` with a backward-compatible `false` default for older JSON. `MarkStaleForSourceUVE()` normalizes the requested source path, scans only cache metadata owned beneath the configured non-symlink cache root, atomically rewrites matching fresh records as stale, and returns the count actually changed. It never creates project source/destination directories, touches `AssetDatabaseUVE`, or changes records for an unrelated source. `AssetImportQueueUVE` rejects stale records before its normal byte/GUID validation; a later successful import writes a fresh non-stale record. The FileSystem dock shows copied pending-change and rescan-required status plus an expandable read-only change review, while **Refresh** performs only the existing index refresh and the documented watcher acknowledgements—never an enqueue, reimport, or source-filesystem mutation. Native event backends, automatic index refresh, automatic reimport, rename pairing, and full content reconciliation remain separately scoped future work.

### Content Browser v1 + Scene Entity Creation (Increment 40)

**`IAssetDatabaseUVE::GetRegisteredAssetsUVE()` remains the sole GUID registry boundary.** It returns copied `AssetRecordUVE` values while the database mutex is held, then sorts the snapshot by normalized lexical path with the 64-bit GUID as a deterministic tie-breaker. Increment 59 passes that copied registry snapshot into explicit `ProjectFileIndexUVE::RefreshUVE()` calls only for one-way in-root GUID correlation; it never infers loading from a row and never causes a path scan to register a file.

**Asset registration identity is lexical and independent of process CWD.** `RegisterUVE()` first lexically normalizes its caller-provided path, so `.`/`..` spelling aliases resolve to one GUID without silently converting a relative path through the process current working directory. Relative and absolute forms intentionally remain distinct because `AssetDatabaseUVE` has no project-root dependency and therefore cannot prove that they refer to the same project asset. `guidToPath` persists the first registered path in lexically normalized form rather than introducing a new registry format or a filesystem-canonical path requirement. On load, legacy JSON may contain lexically equivalent alias rows with distinct GUIDs; every row remains resolvable for scene compatibility, while future equivalent-path registration deterministically chooses the smallest existing GUID. Do not use raw `path.string()` as a new registry identity key; add an explicit project-root contract before introducing physical-path equivalence.

The bottom **FileSystem** dock exposes the cached `ProjectFileIndexUVE` snapshot through session-only folder navigation, clickable breadcrumbs, a case-insensitive current-folder path filter, and an existing-asset type focus selector. It classifies each row solely from the normalized extension and keeps the optional GUID correlation as a separate `Registered` badge, never opening files merely to render a type. Its editor state retains only copied entry/record values, current folder, and filter state, so it does not retain AssetDatabase internals or alter asset ownership. Scene persistence remains available through the existing editor command API; the workspace update does not introduce a separate content serialization format or a native file-dialog dependency.

The Library workspace creates a root-level document entity through `EditorUVE::CreateDocumentEntityUVE()`, selects it, and marks the document dirty. Every supported archetype receives `TransformComponentUVE`; **Empty** adds no other component, **Camera** adds `CameraComponentUVE`, **Directional Light** adds `LightComponentUVE` with `LightTypeUVE::Directional`, **Collision Box** adds `ColliderComponentUVE`, and Increment 58’s **Cube**, **UV Sphere**, and **Plane** add a serializable `PrimitiveMeshComponentUVE` plus their matching collider. The editor camera remains an excluded non-document entity. All exposed specialized components are registered by `ISceneSerializerUVE`, so creating and saving them preserves the existing `.uvescene` persistence boundary.

### Scene Hierarchy v2: Entity Naming (Increment 41)

**`NameComponentUVE` is optional persistent scene metadata.** It contains a human-readable `std::string` and is registered by `ISceneSerializerUVE`, so names survive ordinary `.uvescene` save/load. Existing scenes without the component remain valid; the editor does not inject names while loading and renders the established entity-index/generation fallback instead. Names are not globally unique by contract.

Editor-created Empty, Camera, Directional Light, and Collision Box roots receive generated bases matching their archetype. When a generated base is already used by a live document entity, the editor selects the first deterministic numeric suffix such as `Camera 2`; it never scans or names the editor camera and never renames an existing manually named entity. `EditorUVE::SetSelectedEntityNameUVE()` validates lifecycle and document selection, rejects empty, whitespace-only, over-96-byte, or unchanged values, then adds or updates the component and marks the document dirty. The Scene panel uses this name for display with a unique hidden ImGui identity, while Properties exposes the bounded Name field above Transform controls.

### Hierarchy Search and In-Place Rename v1 (Increment 52)

**Filtering is editor-private and label-stable.** The Scene panel keeps a session-only, case-insensitive query and renders only direct label matches plus their ancestors. The cache stores visible entity handles and rebuilds only after a query, committed name, or known document-structure mutation; it is never serialized, recorded in history, or allowed to mutate selection. While an inline rename field is active, filtering continues to use the committed `GetEntityDisplayLabelUVE()` value rather than its uncommitted text buffer, preventing a currently edited filtered row from disappearing mid-input.

**Inline rename remains one existing Name transaction.** `F2` and the selected-row Rename affordance begin a bounded in-place text edit for one selected live document entity only when authoring commands are allowed and no gizmo or viewport-navigation gesture is active. `Enter` delegates to `SetSelectedEntityNameUVE()` and therefore produces one existing Name Undo/Redo entry only for a valid changed value; `Escape`, selection change, staleness, Play/Pause transition, or competing gesture cancels without a scene mutation or history entry. The editor camera remains ineligible, and ImGui text ownership suppresses conflicting editor shortcuts while typing.

### Viewport Navigation v1 (Increment 43)

**Viewport navigation belongs exclusively to the editor camera session state.** `EditorUVE` owns a finite focus point, yaw, pitch, distance, and transient Orbit/Pan gesture mode for its excluded `m_viewportCamera`. Orbit, pan, dolly/zoom, and focus apply the existing scene-graph local Transform path only to that editor camera. They never modify document entities, document selection, scene dirty state, `.uvescene` serialization, or Undo/Redo command stacks.

The transparent Viewport preserves explicit gesture precedence. An active left-button Translate or Rotate gizmo continues until its release; otherwise, right-button drag orbits around the current focus point, middle-button drag pans it by the current camera FOV and viewport pixel scale, and wheel input applies a clamped dolly distance. While the Viewport is hovered and no text field owns input, `F` focuses a selected live document entity with a clean derived world position. Invalid cameras, stale selections, invalid/dirty world transforms, invalid viewport rectangles, non-finite values, and distance/pitch limit violations fail without document mutation.

Camera navigation uses a 0.5–500 world-unit distance range and an 85-degree pitch limit. Existing left-button behavior remains gizmo-first then collider picking, so navigation gestures cannot select or move document entities. The Viewport help overlay is the authoritative control reminder: **LMB** select/drag axis, **RMB** orbit, **MMB** pan, mouse **wheel** zoom, and **F** focus selection.

### Editor History v1: Undo/Redo (Increment 42)

**History is editor-private session state.** `EditorUVE` owns bounded undo and redo deques with a default capacity of 100 entries; a zero constructor capacity is normalized to one. The editor records only successful state-changing Transform, Name, root-archetype creation, duplicate, delete, and reparent operations. Every new recorded command clears redo, and reaching capacity discards the oldest undo command. Core, Scene, runtime, and serializer layers remain independent of command-history types.

Every history entry stores its ordered before/after selection snapshot, active entity, and dirty-state values. Undo restores the prior mutation state, live selected handles, deterministic active fallback, and dirty flag; redo restores the post-mutation equivalents. Transform and Name replay uses non-recording helpers so history cannot recursively append commands. Creation replay destroys the created root on undo and recreates its stored archetype/name on redo; a new entity handle is valid after recreation. Loading a document clears session history after the recovery copy has been saved and before document replacement begins, so stale handles from the previous scene cannot be replayed. An externally destroyed target, missing original parent, stale reparent parent, or failed subtree restoration causes history replay to fail without partial mutation and clears the unavailable command timeline.

The Edit menu exposes disabled-state Undo/Redo, Duplicate, and Delete actions. `Ctrl+Z`, `Ctrl+Y`, `Ctrl+Shift+Z`, `Ctrl+D`, and `Delete` are accepted only while ImGui does not own text input, avoiding interference with the Properties Name field. The View menu exposes Translate, Rotate, and Scale Gizmo modes, with guarded `W`, `E`, and `R` shortcuts. Lifecycle commands and mode changes are unavailable during an active transform-gizmo or viewport-navigation gesture. A Translate, Rotate, or Scale press-to-release interaction is a single Transform transaction: intermediate drag frames are non-recording previews, a changed release commits one history entry, and cancellation restores the initial Transform without recording a command.

### Transform Snapping v1 (Increment 48)

**Transform snapping is editor-private session state.** `EditorTransformSnappingSettingsUVE` defaults to disabled with finite positive increments of 1.0 world units for translation, 15 degrees for rotation, and 0.1 local-scale units for scale. Settings are never serialized to `.uvescene`, propagated to runtime state, or added to Undo/Redo history. The public setter rejects non-finite or non-positive increments and refuses all mutations while a gizmo drag or viewport-navigation gesture is active.

The View menu provides a guarded **Transform Snapping** submenu with an enable toggle and the supported discrete values: translation 0.25, 0.5, 1.0, or 5.0 world units; rotation 5, 15, or 45 degrees; and scale 0.05, 0.1, or 0.25 local-scale units. When enabled, direct transform commands and Translate, Rotate, and Scale gizmo gestures round their signed delta to the nearest configured increment. Gesture quantization is always calculated from the initial pointer and captured initial local Transform, so preview frames do not accumulate floating-point drift. A rounded zero delta makes no mutation and therefore records no history entry; a changed release still records exactly one Transform transaction.

### Editor Session Settings & Layout v1 (Increment 67)

**Editor preferences remain `.uvesettings` data, never scene data.** A versioned `editor.sessionSettingsVersion` namespace stores fixed workspace tabs, panel visibility, valid viewport gizmo/snap/camera values, and fixed layout presets. Settings load once after the editor camera exists and apply only validated scalar fields. Missing legacy keys migrate in memory to v1 defaults but are never written back automatically; only the explicit **Save Editor Preferences** action serializes settings. Unsupported future versions leave stable defaults intact.

**Only committed state is persisted.** Save reads the editor's validated member values, never an ImGui text buffer or in-progress numeric edit. Save is rejected during a gizmo drag or viewport navigation gesture. Presets are fixed `Default`, `Focus Viewport`, and `Content Review` visibility/tab combinations; arbitrary dock coordinates, document selection, scene dirty state, Undo/Redo, ECS data, and Play state remain outside this persistence boundary.

### Viewport Selection Bounds v1 (Increment 49)

**Selection bounds are derived, read-only editor feedback.** `EditorUVE::TryGetSelectedBoundsUVE()` returns an oriented world-space box only for the active live document entity with Transform, derived WorldTransform, and box Collider components. The query rejects dirty derived transforms, non-finite values, unsafe zero extents or scale components, and an invalid world quaternion without mutating selection, scene data, dirty state, camera state, or Undo/Redo history.

Each corner starts as one signed combination of the collider half-extents. The editor applies the derived component-wise world scale, rotates by the normalized derived world quaternion, then adds the derived world position. `DrawSelectionBoundsUVE()` projects all eight corners through the existing camera projection helper before drawing any part of the overlay. A failed projection skips the complete overlay rather than presenting a misleading partial box.

The transparent Viewport draws twelve restrained cyan edges, eight corner markers, and one center point for each selected collider through its existing ImGui draw list; the active entity uses a stronger yellow treatment. Bounds draw after input handling and before the active gizmo, so they provide selection context without consuming pointer input or obscuring Translate, Rotate, and Scale interaction. No shader, render target, render graph node, serializer data, runtime state, preference, or history entry is introduced. Mesh-derived bounds and material-aware selection outlines remain separate rendering increments.

### Multi-Selection v1 (Increment 56)

**Selection is ordered session state with an explicit active entity.** `EditorUVE` stores a deduplicated vector of live document handles plus one active handle. `SelectEntityUVE()` replaces the complete selection, while `ToggleEntitySelectionUVE()` appends a newly selected entity and makes it active or removes an already selected entity. Removing a non-active handle leaves the active entity unchanged. Removing the active handle promotes the last remaining element in insertion order; removing the final element clears active state. `TickUVE()` prunes deleted, non-document, duplicate, and camera handles before editor use, then applies the same active fallback rule. The public `GetSelectedEntityUVE()` remains the active-entity compatibility accessor; `GetSelectedEntitiesUVE()` exposes the complete ordered set.

**Input and UI intentionally communicate v1 scope.** A normal hierarchy click or collider-backed viewport pick replaces selection; Ctrl-click toggles one entity. Hierarchy nodes show every selected item, with a distinct active-row treatment. The Properties panel renders a deterministic count, active entity, and ordered selected labels while multiple handles are selected; it does not expose name or Transform fields. Bounds render independently for every valid selected collider, using cyan for secondary selections and a stronger yellow overlay for the active entity. Gizmos are not merely disabled: all Translate, Rotate, Scale, and Trackball drawing and hit-testing are absent unless the selection contains exactly one valid document entity.

**Mutation remains deliberately single-entity.** Name editing, direct Transform commands, gizmo drags, duplicate, delete, reparent, hierarchy drag/drop, and single-entity inspector edits reject as a whole when more than one entity is selected. No partial write, dirty-state mutation, or history entry is allowed. Existing history entries now store ordered before/after selection snapshots with their active entity. Play Mode serializes selection as document-root/child-index paths per selected entity and one active path, resolving only live restored handles after Stop; missing paths are pruned deterministically. Marquee/range selection, group pivots, batched transforms, multi-edit drawers, multi-entity lifecycle, clipboard, and naming remain later increments.

### Play Mode Sandbox v1 (Increment 50)

**Core owns simulation execution; Editor owns the transient session.** `ISimulationControlUVE` is the narrow Core-owned seam that permits `EditorUVE` to request normal fixed execution, pause, or one pending fixed step. `EngineCoreUVE::Update()` continues polling input/window events, dispatching queued events, updating scene-graph-derived transforms, maintaining assets/shaders, rendering, and drawing the editor overlay in every mode. Only fixed `PhysicsSystemUVE` stepping is gated: Paused discards the timer fixed accumulator and runs no regular step; one pending Step runs exactly one configured fixed delta and remains Paused. `ITimerUVE::DiscardFixedStepAccumulatorUVE()` must never reset frame delta, wall-clock time, or the timer clock reference.

**Play data is an immutable editor snapshot.** Entering Play captures all document roots through `ISceneSerializerUVE::CaptureUVE()` before Core is marked as a transient session. Stop pauses Core, captures a transient rollback snapshot, destroys document roots while preserving the editor camera, and restores the immutable pre-Play snapshot with fresh entity handles. Failed authored restore clears its partial result and restores the transient rollback document; it leaves the sandbox session active rather than exposing a partial authoring scene. An empty document is an explicit valid snapshot case and does not use an empty `RestoreUVE()` return as a success signal.

**Raw handles never cross Stop.** `IEntityManagerUVE` remains the canonical handle invalidation boundary: transient entity destruction publishes `EntityDestroyedEventUVE`, and restore publishes fresh `EntityCreatedEventUVE` handles. `EditorUVE` stores no pre-Play raw selected entity; it stores root/child-index paths for the ordered selection plus one active path relative to the serialized immutable snapshot and resolves only live restored handles after the snapshot's root order and hierarchy have been restored. Runtime spawn, despawn, and reparenting cannot shift that lookup because the transient hierarchy is discarded first. Future scripting, event-routing, or cache-owning systems must subscribe to entity lifecycle events or call `IsAliveUVE()` before each handle use.

**Authoring and persistence are isolated.** While Playing or Paused, scene Save/Load, document creation, selection/picking, inspector and transform mutation, gizmos, lifecycle commands, hierarchy drag/drop, and Undo/Redo are rejected and visually disabled. Play, Pause/Resume, Step, Stop, and editor-only orbit/pan/zoom remain available. The private viewport camera is outside document roots, has no simulated physics components, and has no v1 runtime script driver; user navigation intentionally persists after Stop. If a future system drives this camera, that system must add full camera snapshot/restore or introduce a separate game camera.

The explicit Core transient-session flag suppresses `CheckpointManagerUVE::UpdateUVE()` independently from Running or Paused execution. No elapsed checkpoint time, playtime counter, checkpoint file, `.uvescene` save, scene serializer payload, render target, renderer state, or Undo/Redo entry is produced by Play Mode. Runtime UI, Game View, script-event execution, input handoff, audio pause, game-time scale, debugging, hot reload during Play, and save-game semantics remain separate increments.

### Scene Hierarchy Reparenting v1 (Increment 45)

**One subtree moves through the existing scene graph.** `EditorUVE::ReparentSelectedEntityUVE()` moves the selected live document root below one valid live document parent, while the invalid parent sentinel returns it to the document-root level. The Scene panel exposes that command through a drag-and-drop payload that contains only an `EntityUVE`: dropping on a document node selects a new parent, while the explicit root drop target detaches the subtree. The private UI backend never writes hierarchy components directly.

**Transform and safety semantics are explicit.** Reparenting defaults to retaining the selected root’s authored local `TransformComponentUVE` and delegates dirty propagation to `ISceneGraphUVE::SetParentUVE()`; Increment 53 adds an explicit shear-safe Keep World alternative. Editor preflight rejects absent scene-graph components, self-parenting, no-op parent choices, cyclic moves, non-document handles, stale handles, the viewport camera, and active gizmo/navigation gestures before it reaches the assertion-based scene graph API. The root’s descendants remain attached below it unchanged.

**Reparent history uses handles, not snapshots.** A successful move stores the live root, parent-before, parent-after, selection-before/after, and dirty-before/after in one editor-private entry. Undo and redo only call the non-recording graph mutation path; either replay fails and clears the unavailable history timeline if the moved root or the required parent is no longer a valid scene-graph document entity, or if an external edit makes the restored relation cyclic. The moved root itself remains selected after a successful direct move and after replay.

### Keep-World Reparenting v1 (Increment 53)

**Reparent mode is editor-session state.** `EditorReparentTransformModeUVE` defaults to Keep Local, preserving the existing authored-local-transform hierarchy move behavior. Keep World is explicit, is neither serialized nor recorded in history, and is unavailable during authoring-protected Play/Pause or an active gizmo/navigation gesture. The hierarchy drag/drop remains the only move UI and uses the current session mode.

**Keep World is a shear-safe TRS transaction.** Before a structural write, the editor captures clean source world position, normalized rotation, and scale plus a clean prospective parent world TRS. It derives candidate local position, rotation, and scale with parent-relative quaternion and component-wise scale operations, validates every candidate component, then sets parent and local Transform as one editor transaction. World-derived state is never silently approximated: a rotated parent with non-uniform scale is rejected because the engine has no affine shear representation. Parent world scale and each solved local scale component must be finite and at least `kMinimumLocalScaleUVE` (0.001); near-zero values reject before division. Failure after a parent write restores both original parent and local Transform without dirty/history mutation.

**Reparent history restores parent and local TRS together.** Each entry stores the old/new parent plus exact before/after authored local Transform. Undo and Redo apply the corresponding parent then stored local Transform without recording nested history, preserving the selected entity and dirty state. Invalid replay handles or parents retain the existing fail-safe timeline-clearing behavior.

### Entity Lifecycle v1: Duplicate and Delete (Increment 44)

**Snapshot boundary.** `Scene::ISceneSerializerUVE::CaptureUVE()` creates an in-memory universal `.uve*` envelope and `RestoreUVE()` creates fresh entities from that same registered-component payload. Disk `SaveUVE()`/`LoadUVE()` share the payload encode/decode implementation, so lifecycle history cannot silently diverge from `.uvescene` hierarchy mapping, component coverage, or derived `WorldTransformComponentUVE` rebuild rules. Capture rejects invalid roots and unregistered component types before the editor changes any entity; malformed restore data rolls back entities it created.

**Hierarchy semantics.** Duplicate captures the selected root and all descendants, restores one fresh subtree, and attaches its root to the source root's current parent. A document root therefore duplicates as another document root; a child duplicates as a sibling under the same parent. When a valid root `NameComponentUVE` exists, its duplicate root receives the first available deterministic suffix through `MakeUniqueDocumentEntityNameUVE()`; descendants retain their snapshot names. Delete captures before destroying the selected subtree, then selects its still-live document parent or clears selection. Undo restores a deleted subtree below the original parent with a fresh root handle; redo destroys that current restored subtree. Duplicate undo destroys the current duplicate; redo restores a fresh duplicate under its stored parent. No replay path records a nested history command.

**Ownership and safety.** The editor-owned viewport camera is never a document entity and cannot be selected, captured, duplicated, deleted, or restored through lifecycle history. A lifecycle operation requires a running editor, exactly one live selected document entity, no active gizmo drag, no viewport-navigation gesture, and a successful serializer capture. If an externally stale parent prevents restore, the operation fails safely and the unusable history timeline is cleared rather than attaching content to an arbitrary root.

**Deliberately deferred:** offscreen texture compositing of the viewport, mesh-triangle or render-ID picking, non-collider selection, mesh-derived bounds and final material-aware selection outline rendering, first-person/fly navigation, camera bookmarks, cinematic camera controls, negative scale, proportional/multiplicative scale, reflection-based/custom inspector drawers,
 native file dialogs, layout/preferences persistence, autosave policy changes, filesystem scanning, asset import/reimport, asset drag-and-drop, thumbnails, preview generation, Debugger/Animator/AI Toolbar runtime content, dock registration, hierarchy child ordering, multi-entity duplication/deletion, OS clipboard copy/paste, multi-entity naming, marquee/range selection, group pivots, batched transforms, multi-edit drawers, cross-session command logs, and branchable history. These remain separate increments so every editor slice stays buildable, testable, and compatible with the existing renderer.

**Validation expectation:** every Editor Foundation v1 change must build under GCC and Clang, retain the full headless CTest suite, run `uve_editor --headless --frames <n>` successfully, and exercise the real windowed overlay path under `xvfb-run` where a virtual display is available.

## Inspector Drawer Registry (`engine/editor/`, Increment 62)

`InspectorDrawerRegistryUVE` is an editor-local, main-thread-only composition seam for one selected document entity’s Inspector sections. It owns only an append-ordered list of callback entries; it does not own ECS data, `EngineServicesUVE`, Dear ImGui state, history, or scene-document lifetime. Registration rejects empty identifiers, duplicate identifiers, and missing eligibility/draw callbacks. `DrawEligibleUVE()` visits entries in successful registration order and invokes only entries whose predicate accepts the supplied entity. This ordering is observable editor behavior: do not replace it with an unordered container or an implicit component-type sort. A draw pass snapshots its initial entry count, so an entry registered from a callback is retained but first becomes eligible on the next pass; callbacks must not rely on same-pass self-registration.

The first built-in IDs are `name`, `transform`, and `primitive-mesh`. Their eligibility predicates are responsible only for safe section applicability; the enclosing `EditorUVE` Inspector still owns no-selection, multi-selection, disabled-authoring, and missing-Transform presentation. A registry callback must stay thin: it may read the selected entity for UI, but every authored write must call an existing or newly dedicated validated `EditorUVE` command. A callback must never mutate an ECS component directly, bypass dirty-state/history recording, start imports, touch the filesystem, or capture ownership of engine services. This protects Transform/Name/Primitive history, collider synchronization, selection rules, and transient Play/Pause safeguards as new sections are added.

Future supported drawers must use a stable unique ID, a side-effect-free eligibility predicate, and deterministic registration from `EditorUVE`’s built-in registration helper. Adding runtime plugin loading, generic reflection-based editing, arbitrary-component enumeration, asset import settings, or multi-entity component edits is not a small drawer addition; each needs a separately designed contract and increment.

### Project Health & Headless Automation v1 (Increment 68)

**`uve_project_check` is a read-only, headless validation boundary.** The checker may enumerate ordinary project files and build short-lived copied registry/envelope facts, but it must never initialize an editor window, import, register, save, repair, rewrite, move, delete, or follow symlinks. Diagnostics are deterministically sorted by path, code, then message and render identically ordered text or JSON reports. Exit `0` means no errors, `1` means completed health errors, `2` means invalid invocation, and `3` means checker-wide setup failure.

**File validation is isolated and dependency-aware.** Expected or unexpected exceptions at one file boundary become a stable `validation.exception` error and do not prevent remaining files from being inspected. Independent facts for one file aggregate into multiple diagnostics. Downstream checks that require a successfully decoded envelope do not invent kind or payload diagnostics after decode failure; the decode diagnostic plus independently knowable facts remain visible. This ensures one corrupt file never suppresses actionable findings elsewhere while preserving diagnostic truthfulness.

**Only validated state is rendered.** Text and JSON output contain severity, stable code, path, message, and recovery guidance. The report renderer must never serialize raw parser buffers, nondeterministic timestamps, addresses, or unordered container traversal.

## Native Visual Scripting Core v1 (Increment 75)

`uve_scripting` is a native C++20 foundation module below the editor UI and managed host. `ScriptNodeRegistryUVE` owns stable internal node-type identifiers and copied descriptors; `ScriptGraphUVE` owns authored node/link values only. Neither type owns ECS entities, renderer resources, managed objects, filesystem paths, or runtime execution state. Future plugin registration must use this value-only descriptor boundary and a separately reviewed ABI/lifecycle contract.

A node type requires a non-empty stable internal identifier, a non-empty user-facing display name, and unique non-empty pin names. Internal identifiers and C++ types use the `UVE` convention; future user-facing node display names do not. Pins have an explicit direction and `ScriptValueTypeUVE`; links are valid only from an output pin to an input pin with an exact matching value type. Execution pins are typed explicitly rather than represented by an untyped boolean or hidden side channel.

Graph mutation is conservative. Empty node types, duplicate node IDs, empty link endpoints, and duplicate links are rejected without changing the graph. `ValidateUVE()` is deterministic and returns stable diagnostics for unknown node types, unknown pins, wrong pin direction, incompatible pin types, self-links, and missing node endpoints. Validation never repairs, reorders, executes, compiles, serializes, or silently drops authored graph data.

Increment 75 deliberately does not introduce a bytecode VM, engine-call binding registry, hot reload, debugger, C# graph canvas, managed runtime, or bridge capability. Compiler IR, versioned `.uvescript` bytecode, runtime state, and managed presentation require later increments with their own ownership, validation, compatibility, and failure-recovery contracts.


## Visual Scripting Compiler IR v1 (Increment 76)

`CompileScriptGraphToIrUVE()` is a validation-first, native-only lowering seam. An invalid graph returns diagnostics and no partial IR program. A valid program carries an explicit version, deterministic instruction order, and one source-node mapping entry per instruction. Node execution records are ordered by ascending stable node ID; value-transfer records are ordered by output node/pin and then input node/pin. The IR is descriptive and non-executable in this increment: it does not own engine calls, ECS state, bytecode memory, or a managed runtime. Later bytecode and VM increments must consume this versioned boundary rather than reinterpreting authored graph data independently.


## Versioned Visual Script Bytecode v1 (Increment 77)

`ScriptBytecodeProgramUVE` is a bounded, versioned native program container. Encoded data begins with explicit `UVES` magic, a fixed-width little-endian version, and an instruction count; decoding validates magic, version, truncation, instruction count, instruction kind, and trailing bytes before publishing a program. The current instruction cap is `kMaximumInstructionsUVE`; malformed or oversized input returns diagnostics and never allocates an unbounded instruction list. The format is a transport/storage boundary only in this increment: execution belongs to the later VM, and engine-call bindings do not enter bytecode implicitly.


## Bounded Native Visual Script VM v1 (Increment 78)

`ExecuteScriptBytecodeUVE()` executes only validated bytecode instruction descriptors within an explicit instruction budget. The budget is clamped to the bytecode maximum, exhaustion returns a diagnostic and instruction index, and unsupported versions or instruction kinds halt without side effects. This increment intentionally has no ECS, renderer, input, audio, or filesystem binding; engine-call dispatch must be introduced through a separately reviewed, explicit binding registry rather than hidden VM behavior.


## Script Component Runtime Boundary v1 (Increment 79)

`ScriptRuntimeUVE` owns only copied script-instance values keyed by generational `Scene::EntityUVE` handles. It does not own entities, components, ECS pointers, scene lifetime, or engine services. Attach rejects invalid or duplicate handles, incompatible bytecode versions, oversized programs, and instance-cap overflow without mutation. Tick order is canonicalized by entity index and generation; disabled instances are skipped; each execution returns a copied VM result. Entity-handle generation is part of identity and must never be reduced to an index-only key.


## Native Graph Persistence v1 (Increment 80)

`EncodeScriptGraphUVE()` and `DecodeScriptGraphUVE()` use a versioned deterministic JSON envelope for authored graph data. The encoder preserves graph order and rejects node/link/text limits before publishing output; the decoder validates schema version, required fields, JSON types, bounded collection sizes, duplicate entries, and endpoint shape before publishing a complete graph. Decode failure never returns a partial graph. Persistence is graph data only: it does not serialize runtime VM state, ECS pointers, engine services, managed UI state, or native resource handles.


## Native Graph Editor Backend v1 (Increment 81)

`ScriptGraphEditorBackendUVE` is the native command boundary for graph authoring. Each add/remove command edits a candidate graph copy and publishes it only after the structural mutation succeeds; rejected commands leave the current graph and history unchanged. Undo and redo store bounded value snapshots, redo is cleared by a new successful edit, and node removal removes incident links atomically. This backend exposes no UI widgets, managed object references, ECS pointers, or runtime execution authority; later bridge presentation must send named commands and receive copied DTOs.


## Managed Visual-Scripting Presentation Boundary v1 (Increment 82)

The managed host may receive only copied visual-scripting presentation facts: availability, graph revision, bounded node/link counts, edit-capability state, and a bounded reason. The C# layer must not deserialize native graph objects, retain native pointers, execute compiler/VM work, or write graph assets directly. A future editable graph surface must use separately named native commands and revision-protected responses; a status DTO is not an authorization to mutate the graph.


## Script Debugger Contract v1 (Increment 83)

`ScriptDebuggerUVE` consumes copied versioned bytecode and uses each instruction's source-node ID as the stable breakpoint mapping. Breakpoints are bounded and snapshots sort IDs deterministically. Continue skips a just-hit breakpoint once to avoid a resume loop; Step executes exactly one validated instruction; completion, detachment, and faults are explicit states. The debugger does not own ECS state, editor UI state, engine services, or hot-reload replacement; later managed debugger presentation must consume copied snapshots and named commands.


## Native Plugin Extension Seam v1 (Increment 85)

The first plugin milestone is a static native lifecycle seam, not a dynamic loader. `NativePluginManifestUVE` requires a bounded identifier, display name, engine protocol version, and unique capability identifiers. `NativePluginRegistryUVE` accepts at most a bounded number of manifests and exposes generation-checked registration scopes; stale or duplicate close operations are rejected. Dynamic libraries, filesystem manifests, ABI negotiation beyond the explicit protocol field, arbitrary engine callbacks, and security-sensitive loading remain future reviewed work. A plugin scope must not bypass existing graph, editor, asset, ECS, or runtime ownership boundaries.


## Bytecode Hot Reload Safety v1 (Increment 84)

Hot reload must decode and validate a complete candidate program before publication. Invalid magic, version, truncation, instruction, or limit diagnostics must leave the active program and generation unchanged. `ScriptHotReloadManagerUVE` retains the last-known-good program on failed replacement, increments the active generation only for an accepted candidate, and reports compatible-version/state-transfer facts explicitly rather than implying that arbitrary VM state can survive. It owns copied bytecode only; ECS, editor, managed, and filesystem state remain outside this boundary.


## Visual Scripting Editor Canvas v1 (Increment 86)

`ScriptGraphCanvasUVE` is a native editor-session layer over `ScriptGraphEditorBackendUVE`. It owns only copied canvas-session values: bounded node layout entries, ordered selection, palette/snapshot presentation, and finite pan/zoom state. Graph structure and typed-link validation remain delegated to the native graph/backend boundary; the canvas must not become a VM, ECS, renderer, filesystem, or plugin ownership surface.

Pan and zoom are explicitly non-undoable session state in v1. Every graph, layout, or selection mutation stores a complete canvas-plus-graph value state rather than a graph-only diff. Undoing node removal must restore exact node order, link order, layout-entry order, selection order, and selected flags; failed or stale commands create no history entry. History is bounded and a successful new mutation clears redo.

The bridge exposes only bounded copy DTOs and named revision-checked requests. C# may render and submit value-only gestures, but it must never receive raw graph objects, ECS pointers, OpenGL handles, ImGui values, filesystem handles, or mutation authority. Canvas layout is session state and must not be added to the durable graph persistence schema without a separately versioned persistence increment.


## Managed Visual Scripting Canvas Presentation v1 (Increment 87)

`VisualScriptCanvasControl` is a managed presentation surface over copied `BridgeVisualScriptCanvasSnapshot` values. It may retain only the latest immutable DTO and transient pointer/gesture state. It must not retain native graph references, ECS pointers, OpenGL resources, ImGui values, filesystem handles, or backend process ownership.

Canvas coordinates are derived from the copied pan/zoom view and are used only for rendering and gesture translation. Selection, node movement, view changes, Undo, and Redo are emitted as the existing named `BridgeCommand` values with the top-level bridge revision, then sent through the serialized bridge session. The managed control must not perform graph validation, link creation, compiler/VM work, persistence, or history mutation itself.

Rendering must remain safe for empty and truncated snapshots. A truncated flag is displayed as status rather than silently treated as complete data. Durable canvas layout, source mapping, watches, breakpoint UI, and managed runtime ownership require separate reviewed increments.


## Developer Console and Diagnostics v1 (Increment 88)

Developer-console commands must be explicitly registered by native code with bounded identifiers, bounded help text, and a native handler. Unknown, oversized, multiline, or malformed command text is rejected without granting an implicit filesystem, process, network, ECS, renderer, or plugin capability. Built-in commands remain safe and deterministic; CVAR names and values are bounded, typed by their registered native descriptor, and read-only state is enforced by the native service.

The managed Console panel renders only copied `DeveloperConsoleSnapshotUVE` DTO values. It may submit bounded text through the named bridge request `SubmitDeveloperConsoleCommand`, but it must never parse or execute commands locally, write logs/settings, or retain a native service reference. Output, history, and CVAR truncation flags must be shown rather than interpreted as deletion or completeness.


## Developer Console Policy and Discovery v1 (Increment 89)

The developer console is explicitly development-only in v1. A shipping-policy instance must publish `available=false` and `developmentOnly=true`, reject command execution and discovery mutations, and never silently expose a debug or cheat surface. Policy state is native-owned and is represented as copied snapshot facts across the bridge.

Severity filtering is a presentation-state mutation over bounded native output; it must never delete or rewrite the underlying log history. Completion is deterministic, prefix-based, registry-backed, lexicographically ordered, and bounded by the native completion limit. History navigation uses a bounded native cursor and an explicit empty-draft state; the managed host may render and request cursor movement but must not own or mutate history.

The Increment 89 bridge additions remain named and value-only: severity-filter, completion-prefix, and history-delta payloads are validated by C++ and serialized as additive DTO fields. C# may display copied availability, filter, cursor, and completion values, but it must not infer policy, execute commands, access filesystem/process/network/ECS/renderer/plugin state, or retain a native console reference.


## Typed Data Table Core v1 (Increment 90)

`DataTableUVE` is a native, value-only asset-module contract. Its schema is ordered and caller-declared; columns are bounded and unique, row identifiers are bounded and unique, and each row must contain exactly one value matching every declared column type. Supported values are Boolean, signed 64-bit Integer, finite Number, and bounded String. The table exposes copied snapshots and a read-only row lookup rather than pointers to mutable row storage.

CSV ingestion is deterministic and failure-atomic. The first header field is `id`, subsequent headers must match the declared schema exactly and in order, quoted fields support doubled quotes, and CRLF/LF line endings are normalized without locale-sensitive behavior. Invalid rows, duplicate identifiers, type conversion failures, overflow, non-finite numbers, malformed quotes, and bounds violations produce bounded diagnostics with stable codes and line/column context. A failed import never replaces previously committed rows.

The Increment 90 core must not infer types or silently acquire filesystem, asset-database, ECS, renderer, process, network, reflection, managed-runtime, editor-bridge, hot-reload, export, reference, or visual-scripting authority. JSON/TSV/XLSX adapters and editor-facing workflows require separate reviewed increments.


## Data Table TSV and JSON Import v1 (Increment 91)

TSV import must reuse the bounded quoted-delimited rules of CSV with a tab delimiter, including CRLF/LF normalization, doubled quotes, exact `id`-first headers, and caller-declared schema order. It must not silently treat commas, whitespace, or inferred columns as alternate delimiters.

JSON import must accept only a top-level array of objects. Every row must contain exactly one bounded string `id` and one member for each declared column; object member order may vary, but output values must follow schema order. JSON kinds are checked directly: booleans require JSON booleans, integers require signed integer numbers, numbers must be finite, and strings must remain within the string bound. Extra members, missing members, nulls, wrong kinds, malformed documents, and duplicate IDs are deterministic diagnostics.

Both adapters are failure-atomic. A failed import publishes bounded diagnostics and a new generation while preserving the last committed rows; a successful import atomically replaces all rows and clears previous diagnostics. These adapters remain native value/asset-module code and acquire no filesystem, editor, managed, runtime, or visual-scripting authority.
