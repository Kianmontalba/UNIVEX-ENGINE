# UNIVEX-ENGINE

A next-generation 3D game engine focused on modular architecture, real-time camera behavior, and streamlined deployment for interactive experiences.

## Status

**Increment 1: Foundation Layer** — a headless, fully unit-tested core consisting of
`LoggerUVE`, `TimerUVE`, `EventSystemUVE`, and `EngineCoreUVE` (the `Init → Load →
BeginFrame → Update → LateUpdate → Render → EndFrame → Shutdown` pipeline). No windowing or
rendering yet — see `docs/MASTER_SPEC.md` for the full engine vision and `docs/
CODING_STANDARDS.md` for the conventions this codebase follows.

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
Undo/Redo transaction while an increment-rounded zero does not dirty the document or add history.
Duplicate and delete capture the complete selected subtree in memory through the
same registered-component envelope used by `.uvescene`, preserving hierarchy and restoring fresh
entity handles during Undo/Redo; they cannot affect the editor-only camera. Shortcut actions are
suppressed while a text field owns input or an active viewport gesture is in progress. One completed
Translate, Rotate, or Scale gizmo drag is recorded as one history step. Translate mode additionally exposes translucent
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
the renderer uploads each deterministic mesh once, uses the canonical lit/shadowed material shader with fallback
white/normal textures, and culls primitives from their transformed local bounds. The Library workspace exposes
`+ Cube`, `+ Sphere`, and `+ Plane`; the Inspector lets exactly one selected primitive atomically change its kind
and base color, while synchronizing its collider extents for the new kind. The editor draws a session-only 10 × 10
XZ ground grid at one-unit spacing as viewport feedback. It is neither selectable nor serialized. Mesh-triangle or
render-ID picking, non-collider selection, mesh-derived selection bounds, and offscreen-texture viewport compositing
remain deferred.

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
`assets/` content root, with folders before files, case-insensitive path filtering, and existing AssetDatabase GUID
correlation. It scans only when the dock first opens or the user presses **Refresh**; normal overlay frames read the
cached snapshot and perform no filesystem traversal. Missing roots are valid empty states, refresh failure retains
the last successful tree, and symlinks are never exposed or followed. The dock is strictly read-only: it does not
import, reimport, preview, rename, move, delete, create folders, drag-and-drop, or load assets. Import and Signals
tabs remain deliberate session-only placeholders until their backing contracts exist.
Viewport picking intentionally selects only live document entities with the existing box collider
component. Primitive roots receive matching default colliders: Cube and UV Sphere use 0.5-unit half extents on all
axes, while Plane uses `{0.5, 0.025, 0.5}` so its visible, origin-centered XZ surface remains selectable. Every selected collider-backed document entity receives a read-only oriented bounds overlay with corner
and center markers; the active entity is yellow while other selected entities are cyan. This feedback follows derived
world transforms and never changes scene data or history. Mesh picking, mesh-derived bounds, negative scale,
proportional/multiplicative scale, fly navigation, camera bookmarks, cinematic camera tools, import/reimport,
asset drag-and-drop, thumbnails, workspace-content implementation for Debugger/Animator/AI Toolbar,
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
tests/          — GoogleTest suite, mirrors the engine/ layout
docs/           — MASTER_SPEC.md (full design doc), CODING_STANDARDS.md
```

See `CONTRIBUTING.md` before making changes.
