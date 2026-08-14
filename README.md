# UNIVEX-ENGINE

A next-generation 3D game engine focused on modular architecture, real-time camera behavior, and streamlined deployment for interactive experiences.

## Status

**Increment 1: Foundation Layer** — a headless, fully unit-tested core consisting of
`LoggerUVE`, `TimerUVE`, `EventSystemUVE`, and `EngineCoreUVE` (the `Init → Load →
BeginFrame → Update → LateUpdate → Render → EndFrame → Shutdown` pipeline). No windowing or
rendering yet — see `docs/MASTER_SPEC.md` for the full engine vision,
[`docs/ROADMAP_INDEX.md`](docs/ROADMAP_INDEX.md) for the active cross-domain roadmap, and
`docs/CODING_STANDARDS.md` for the conventions this codebase follows.

## Building & Testing (Linux)

Requires CMake ≥ 3.24 and a C++20 compiler (GCC ≥ 11 or Clang ≥ 14; verified with GCC 13 and
Clang 18).

```bash
# Configure + build (Debug, GCC)
cmake -S . -B build/gcc-debug -DCMAKE_CXX_COMPILER=g++
cmake --build build/gcc-debug -j"$(nproc)"

# Run the unit test suite
ctest --test-dir build/gcc-debug --output-on-failure

# Run the headless proof-of-life executable
./build/gcc-debug/engine/app/uve_runtime --headless

# Run Editor Foundation v1 without a display (CI smoke test)
./build/gcc-debug/engine/app/uve_editor --headless --frames 2

# Run the desktop editor with an existing scene document
./build/gcc-debug/engine/app/uve_editor --scene path/to/level.uvescene

# Build and test the optional managed C# editor host (requires .NET 8)
dotnet restore editor/managed/UniVex.EditorHost.Tests/UniVex.EditorHost.Tests.csproj --locked-mode
dotnet build editor/managed/UniVex.EditorHost.Tests/UniVex.EditorHost.Tests.csproj --no-restore
dotnet test editor/managed/UniVex.EditorHost.Tests/UniVex.EditorHost.Tests.csproj --no-build --no-restore

# Run a non-UI C# host → real headless C++ bridge probe
./tools/verify_editor_host_probe.sh ./build/gcc-debug/engine/app/uve_editor

# Virtual-display smoke test on a platform capped at OpenGL 4.5
xvfb-run -a ./build/gcc-debug/engine/app/uve_editor --gl-version 4.5 --frames 3
```

Swap `-DCMAKE_CXX_COMPILER=clang++` to build with Clang instead. `UVE_BUILD_TESTS` (default
`ON`) can be set to `OFF` to skip building the GoogleTest suite.

The current editor uses a fixed UniVex workspace layout: a compact **UNIVEX** header with session-only
**Library**, **Asset**, **Scripting**, **Debug**, and **Plugin** workspace labels plus guarded Play/Stop controls;
a left Scene panel; a clean central **3D Viewport**; and a right panel with horizontal **Inspector**,
**Import**, and **Signals** tabs. The active lower dock defaults to **FileSystem**, which exposes a cached,
read-only project-content tree correlated with existing AssetDatabase GUIDs; Debugger, Animator, and AI Toolbar
are visible dock tabs but remain separate future-work areas. The editor also supports persistent human-readable entity names, local Transform editing,
collider-backed viewport picking, Translate, Rotate, and Scale
gizmo modes (`W` / `E` / `R`) with a guarded **World**/**Local** coordinate-space selection, scene save/load actions, and
**Library** workspace creation controls for Empty, Camera, Directional Light, Collision Box, Cube, UV Sphere, and Plane
document roots. New archetypes receive deterministic default names such as
`Camera` and `Camera 2`; the Properties panel or selected hierarchy row can rename one selected live document entity, and
names persist through `.uvescene` save/load. **Multi-Selection v1** provides an ordered document-entity selection with one
active entity: normal hierarchy or viewport click replaces selection, while `Ctrl`-click adds/removes one entity. Removing
the active entity promotes the last remaining selected entity; a Ctrl-click viewport miss retains selection while a regular
miss clears it. Multi-select rows and collider-backed bounds remain visible, with a yellow active outline, but the Properties
panel becomes a read-only selection summary and all gizmos are fully hidden. Transform, rename, duplicate, delete,
reparent, hierarchy drag/drop, and single-entity inspector edits remain deliberately unavailable until exactly one entity is
selected. The Scene panel has a session-only case-insensitive hierarchy filter that
retains matching ancestor paths; `F2` or the selected-row **Rename** control opens an inline editor, where `Enter` commits one
existing Name-history transaction and `Escape` cancels without mutation. Legacy scenes without name metadata remain valid and
fall back to stable entity index/generation labels. The editor also provides a bounded in-session
Undo/Redo history for successful Transform, rename, root-creation, duplicate, delete, hierarchy
reparent, and atomic primitive-appearance actions. The Scene panel supports drag-and-drop reparenting: drop a document entity onto
another document entity to make it a child, or onto the root drop target to detach it. Reparenting preserves the moved subtree and offers a session-only **Reparent Transform** choice: the default
**Keep Local** retains the authored local Transform, while **Keep World** derives a compatible replacement local TRS
from captured world state. Keep World rejects rotated non-uniformly scaled parents and parent scale components below
`0.001` rather than approximating unsupported shear; self-parenting, cycles, stale handles, and the editor camera are
rejected safely. The Edit menu provides disabled-state
**Duplicate** (`Ctrl+D`) and **Delete** (`Delete`) controls for
one selected live document entity, in addition to focus-safe `Ctrl+Z`, `Ctrl+Y`, and
`Ctrl+Shift+Z`. The View menu also provides session-only **Transform Snapping** with guarded enable
and increment controls: Translate uses 0.25/0.5/1.0/5.0 world-unit steps, Rotate uses 5/15/45-degree
steps, and Scale uses 0.05/0.1/0.25 local-scale-unit steps. When enabled, direct commands and gizmo
drags round their signed delta from the captured initial Transform, so a changed drag remains one
Undo/Redo transaction while an increment-rounded zero does not dirty the document or add history. The **View** menu also owns versioned editor-only session preferences: panel visibility, workspace tabs, valid viewport camera/gizmo/snap settings, and fixed **Default**, **Focus Viewport**, and **Content Review** presets. These values load from `.uvesettings` without changing the document and save only through the explicit **Save Editor Preferences** action; legacy values migrate in memory without a hidden write.
Duplicate and delete capture the complete selected subtree in memory through the
same registered-component envelope used by `.uvescene`, preserving hierarchy and restoring fresh
entity handles during Undo/Redo; they cannot affect the editor-only camera. Shortcut actions are
suppressed while a text field owns input or an active viewport gesture is in progress. One completed
Translate, Rotate, or Scale gizmo drag is recorded as one history step. **Editor Tool Sessions v1** explicitly
separates pointer/handle solving from transform transaction ownership: preview values remain baseline-derived and non-recording,
a re-entrant begin request cannot replace an active gesture, release commits the already-applied preview without a second
transform write. Cancellation restores the captured baseline only while the live Transform still matches its own
last preview; an external transform conflict is preserved, marks the document dirty, and never gets overwritten by a stale
baseline. Restore failure is reported explicitly without claiming a false rollback. **Scene Outliner & Inspector Workflow v2**
adds compact read-only root/child context and one deterministic specialized type tag per document row (Plane, UV Sphere,
Cube, Camera, Directional Light, or Collision Box priority). The single-selection Inspector presents a read-only ancestry
breadcrumb and a command-backed Hierarchy section for existing Keep Local/Keep World reparent preference, safe parent choice,
and Make Root. It uses the same existing reparent/Undo/Redo boundary; breadcrumbs do not change selection, multi-selection
remains summary-only, and no generic component-editing surface is implied. Translate mode additionally exposes translucent
XY/XZ/YZ plane handles in the positive 20–60% gizmo quadrant; axis hits always take precedence and plane movement solves
both captured screen-space basis coefficients independently before applying the existing parent-safe local delta path.
Rotate mode uses world-axis X/Y/Z rings,
converts a parented entity's world-axis delta back to its local quaternion, and preserves local position
and scale. Its inner translucent disc is a camera-oriented **Free Rotation Trackball**: ring hits retain priority,
while a center drag maps pointer movement to an edge-clamped virtual sphere for arbitrary world-axis rotation. Pointer
movement beyond the disc remains defined at the sphere edge; ambiguous near-180-degree antipodal drags cancel and restore
the captured Transform rather than choosing an unstable arbitrary axis. Scale mode uses X/Y/Z square handles to edit one strictly positive authored local-scale component.
Its center square is **Uniform Scale Offset**: a captured shared additive local-scale delta is applied to X/Y/Z;
it is intentionally not proportional/multiplicative scaling. If any proposed component falls below `0.001`, the
whole center-handle gesture cancels and restores its captured Transform instead of clamping an individual axis. The editor-only viewport camera now supports
right-drag orbit, middle-drag pan, wheel zoom, and `F` focus for a selected live document entity.
These navigation controls never alter the document, dirty state, scene file, or Undo/Redo history.

**Viewport Scene Rendering v1** replaces the retired EngineCore demo-triangle scaffold with renderer-owned built-in
geometry. Cube, UV Sphere, and Plane roots carry a serializable primitive kind plus bounded linear-RGB base color;
the renderer uploads each deterministic mesh once, uses the Basic3D program (`uModel`, `uViewProjection`, and
`uColor`), and culls primitives from their transformed local bounds. The Library workspace exposes `+ Cube`,
`+ Sphere`, and `+ Plane`; the Inspector lets exactly one selected primitive atomically change its kind and base
color, while synchronizing its collider extents for the new kind. The editor draws a session-only 10 × 10 XZ ground
grid at one-unit spacing as viewport feedback. It is neither selectable nor serialized. Mesh-triangle or render-ID
picking, non-collider selection, mesh-derived selection bounds, and offscreen-texture viewport compositing remain
deferred.

**Viewport Presentation & Render Verification v1** ensures the renderer clears the HDR scene to a restrained
blue-gray environment, records primitive evidence through `Renderer3DFrameDiagnosticsUVE`, and presents the HDR
image through the tone-mapping pass to the window back buffer. The viewport overlay displays the extracted primitive
count plus recorded and OpenGL-issued draw-call evidence; these are CPU-side diagnostic facts, not pixel-readback
claims. OpenGL render-pass clears explicitly restore the depth write mask before a requested depth clear, preventing
a previous depth-disabled fullscreen pass from silently blocking later scene draws. Real-GL regressions verify the
mounted Basic3D source, canonical indexed geometry, HDR composition, the cross-frame depth-clear transition, and
red/green/blue ECS primitives sampled from `GL_BACK` after tone mapping and before swap.

**Inspector Drawer Registry v1** keeps the single-entity Inspector extensible without turning it into a generic reflection UI. Its built-in **Name**, **Transform**, and **Primitive Mesh** sections are registered in deterministic order through editor-local eligibility/draw callbacks. The registry owns callbacks only; it never owns ECS state, EngineServices, or Dear ImGui state. Every authored write remains routed through the existing `EditorUVE` commands, preserving validation, dirty-state changes, Transform/Name/Primitive Appearance history, collider synchronization, and Play/Pause authoring protections. Future component sections must register a stable unique identifier and route edits through a dedicated editor command; dynamic plugin loading, generic arbitrary-component inspection, asset settings, and multi-entity component editing remain deferred.

**Editor Bridge Contract v1** adds a versioned, main-thread C++ adapter for a managed editor host without replacing the native ImGui editor. `EditorBridgeUVE` exposes copied snapshots, discoverable capabilities, value-only entity identities, and stable diagnostics; it never exposes raw ECS pointers, renderer/GL resources, or direct scene-file access. The bridge revision changes whenever bridge-visible editor state changes—whether through native ImGui or bridge commands—so every mutation must submit the current `expectedRevision` and receives `bridge.snapshot.stale` on a conflict. v1 routes selection, clear selection, name editing, document-entity creation, Undo, and Redo only through existing `EditorUVE` command paths, retaining existing validation, dirty state, Play Mode, and history rules. The approved direction separates responsibilities: **C++20** remains authoritative for engine/editor backend, scene/assets, history, validation, and rendering; **C#/.NET** owns UI presentation and workflow only; and **GLSL** owns viewport visuals.

**C# Editor Host Foundation v1** now adds an optional .NET 8/Avalonia connection shell and a local length-prefixed JSON-RPC bridge transport. The host launches one separate `uve_editor --bridge-stdio` child, completes a versioned `bridge.hello` handshake, shows copied backend capability/snapshot facts, and presents compatible, incompatible, failed, and disconnected states. Bridge stdio is intentionally **headless-only and mutually exclusive with native ImGui in that process**: it creates no GLFW window, ImGui overlay, or OpenGL resource. Protocol stdout contains framed JSON-RPC only; logs remain on stderr/file sinks. A crash, EOF, timeout, or compatibility failure never auto-restarts the backend. **Start New Backend Session** requires acknowledgement that it creates a fresh `EditorUVE` and may have lost unsaved in-memory work because bridge v1 has no Save command; a dirty connected session similarly requires an explicit discard confirmation before host shutdown. The connected surface intentionally contains no docking, hierarchy/Inspector/Content Browser authoring, scene mutation controls, native viewport hosting, or OpenGL ownership; those remain later increments.

The top-level **Play** menu provides a transient editor sandbox: **Play** (`F5`) captures the complete
editable document in memory and disables authoring, **Pause** (`F6`) holds fixed physics without accumulating
catch-up time, **Step** (`F10`) advances exactly one fixed physics tick while paused, and **Stop** (`Shift+F5`)
restores the captured document with fresh entity handles, its ordered logical selection plus active entity, dirty state, and existing
Undo/Redo timeline. The Viewport shows a PLAYING or PAUSED badge while active. Save/Load, hierarchy and
inspector edits, document creation, picking, transform gizmos, and history replay are disabled in the sandbox;
the editor-only orbit/pan/zoom camera remains available and intentionally retains user navigation changes.
Transient sandbox frames also suppress Core checkpoint/save-game advancement, so Play-time state cannot
produce an autosave or overwrite the active `.uvescene` file.
The active FileSystem dock presents a cached, deterministic `ProjectFileIndexUVE` snapshot of the configured
`assets/` content root, with folders before files, session-only current-folder navigation, **Up**, clickable
breadcrumbs, case-insensitive current-folder path filtering, and existing AssetDatabase GUID correlation. Every row
has one extension-derived semantic tag (`Folder`, `Scene`, `Prefab`, `Bundle`, `Mesh`, `Texture`, `Shader`,
`Material`, `Save`, or `File`); a correlated asset adds a separate `Registered` badge rather than replacing its
semantic type. The type-focus selector and text filter persist through navigation and explicitly report a zero-match
folder state. `assets/` and `assets` normalize to the same root contract. It scans only when the dock first opens or
the user presses **Refresh**; normal overlay frames read the cached snapshot and perform no filesystem traversal.
Missing roots are valid empty states, refresh failure retains the last successful tree, and symlinks are never exposed
or followed. A separate portable `ProjectChangeWatcherUVE`
polls the same root on the main thread at a configurable interval (one second by default), publishes a copied bounded
journal, and marks only matching derived import metadata stale. The FileSystem dock shows pending-change and
rescan-required status plus a read-only **Review Changes** section. A successful **Refresh** explicitly acknowledges
changes reviewed by that full index rebuild; it never imports, reimports, or mutates source files. Journal overflow
retains newest entries, declares an explicit rescan boundary, and still continues targeted stale-cache marking.
Native OS event backends, automatic index refresh, automatic reimport, and rename pairing remain deferred.

The dock is otherwise strictly read-only: it does not preview, rename, move, delete, create folders,
drag-and-drop, or load assets. The **Import** tab now exposes a read-only copied-job monitor for the deterministic
main-thread `AssetImportQueueUVE`; it has no file picker, enqueue, retry, cancel, drag-and-drop, automatic reimport,
or document mutation control. Explicitly programmatic jobs process at most one per engine update and use the existing
generic importer. Successful imports persist project-local metadata under `DerivedData/Import/`; cache reuse requires
matching source bytes, destination bytes, settings version, schema, output existence, AssetDatabase GUID/path
identity, and a non-stale cache record, so out-of-band destination edits and observed source changes force a fresh
import. Signals remains a deliberate session-only placeholder until scripting runtime contracts exist.
Viewport picking intentionally selects only live document entities with the existing box collider
component. Primitive roots receive matching default colliders: Cube and UV Sphere use 0.5-unit half extents on all
axes, while Plane uses `{0.5, 0.025, 0.5}` so its visible, origin-centered XZ surface remains selectable. Every selected collider-backed document entity receives a read-only oriented bounds overlay with corner
and center markers; the active entity is yellow while other selected entities are cyan. This feedback follows derived
world transforms and never changes scene data or history. Mesh picking, mesh-derived bounds, negative scale,
proportional/multiplicative scale, fly navigation, camera bookmarks, cinematic camera tools, native import/reimport
UI, asset drag-and-drop, thumbnails, background import workers/cancellation, workspace-content implementation for
Debugger/Animator/AI Toolbar,
layout persistence, child ordering, multi-entity lifecycle operations,
marquee/range selection, grouped transforms, multi-edit inspector fields, and OS clipboard copy/paste remain future increments.

## Repository layout

```
engine/
├── platform/   — UVE::Platform  — compiler/OS macros
├── debug/      — UVE::Debug     — LoggerUVE, log sinks, UVE_ASSERT, logging macros
├── utilities/  — UVE::Utilities — TimerUVE
├── events/     — UVE::Events    — EventSystemUVE
├── core/       — UVE::Core      — EngineCoreUVE and supporting types
├── editor/     — UVE::Editor    — Editor Foundation v1 session and UI composition
└── app/        — uve_runtime and the standalone uve_editor executables
editor/managed/ — optional .NET 8/Avalonia editor host and managed protocol/session tests
tests/          — GoogleTest suite, mirrors the engine/ layout
docs/           — MASTER_SPEC.md (full design doc), CODING_STANDARDS.md
```

See `CONTRIBUTING.md` before making changes.


## Project Health & Headless Automation

`uve_project_check --project-root <path> --format text|json` validates a project without starting the editor, a window manager, or a render device. It reports deterministic registry and supported universal-asset envelope findings in human-readable text or machine-readable JSON. The command is deliberately read-only: it does not import, register, save, repair, rewrite, move, delete, or follow symlinks. A corrupt file is isolated as its own diagnostic so other files continue to be checked; independent facts for one file are aggregated only when their prerequisite parse data is valid.
