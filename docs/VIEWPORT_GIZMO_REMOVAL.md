# Viewport + Gizmo Layer Removal (Phase 2)

## Status

The editor's entire 3D viewport panel, transform gizmo, navigation gizmo, gizmo
overlay render pass, orbit/pan/zoom camera, editor-owned viewport camera
entity, infinite grid, entity picking, and environment/sun preview composite
have been removed. This is a demolition, not a stub: there is no placeholder
viewport, no dead camera kept "just in case," and no empty-bodied function
standing in for removed behavior. This document is the interface spec for
whoever rebuilds the viewport later — it records every integration seam that
was severed, the exact removed signatures, and what the renderer used to
expose for the overlay/environment passes.

Also removed in the same pass, at the requester's explicit direction: all
Node3D icon assets, Component icon assets, Gizmo icon assets, and gizmo HTML
prototypes. The editor's own logo/general/content-type icons were kept — see
"Icon assets" below.

## What used to exist

- An editor-owned, non-document ECS entity (`m_viewportCamera`) created in
  `EditorUVE::InitUVE()` and excluded from `GetDocumentRootsUVE()`.
- Orbit/pan/zoom camera navigation (`OrbitViewportUVE`, `PanViewportUVE`,
  `ZoomViewportUVE`, `FocusSelectedEntityUVE`, layout-preset camera framing).
- A ray-based entity picking path (`MakeViewportRayUVE`, `PickViewportUVE`)
  backed by `ColliderComponentUVE` raycasts.
- A unified transform gizmo (Translate/Rotate/Scale/Universal handle
  families, world/local coordinate space, axis/plane/trackball drag
  handling) drawn and dragged through `EditorViewportRectUVE` screen-space
  math (`BeginGizmoDragUVE`, `UpdateGizmoDragUVE`, `CommitGizmoDragUVE`, and
  their many geometry helpers).
- A viewport navigation gizmo widget (`ClickViewportNavigationGizmoUVE` /
  `HandleViewportNavigationGizmoClickUVE`) and the standalone
  `GizmoSystemUVE` / `ViewportNavGizmoUVE` classes plus their HTML/SVG
  authoring assets under `engine/editor/assets/gizmos/`.
- A native render-graph composite pass ("EditorViewportEnvironment") that
  drew an infinite grid, sky, and sun preview, driven by copied per-frame
  facts in `EditorViewportVisualStateUVE` (grid origin/spacing, environment
  and sun preview toggles, camera basis, selection rectangle, active gizmo
  axis).
- A second native render-graph pass ("EditorGizmoOverlay") that drew real
  GPU-shaded 3D gizmo arrows (`GizmoOverlayItemUVE`, one world matrix + color
  per arrow) via `SetEditorGizmoOverlayItemsUVE`.
- Two built-in shader programs backing those passes:
  `kEditorViewportEnvironment*` and `kGizmoLit3D*`
  (`engine/render/shader/built_in/editor_viewport_environment.glsl` and
  `gizmo_lit_3d.glsl`).
- A 2D screen-space viewport tab (`EditorViewportTabUVE::TwoD`,
  `Draw2DCanvasUVE`) that shared the same `EditorViewportRectUVE` plumbing —
  the tab enum and the *panel* were removed, but the underlying 2D canvas
  authoring state (`Editor2DCanvasStateUVE`, pan/zoom, `Get2DCanvasStateUVE`,
  `Set2DCanvasZoomUVE`, `Reset2DCanvasViewUVE`) is unrelated document/UI state
  and was kept.

## Removed public `EditorUVE` interface (`engine/editor/include/uve/editor/editor_uve.h`)

Types:
- `EditorViewportRectUVE`
- `EditorGizmoModeUVE` (Translate/Rotate/Scale/Universal/Select)
- `EditorGizmoCoordinateSpaceUVE` (World/Local)
- `EditorViewportNavigationModeUVE` (None/Orbit/Pan)
- `EditorViewportTabUVE` (Scene/TwoD)
- `EditorTranslatePlaneUVE`, `GizmoHandleKindUVE`, `GizmoDragUVE` (private)

Public methods:
- `MakeViewportRayUVE(const EditorViewportRectUVE&, Math::Vector2UVE) const`
- `PickViewportUVE(const EditorViewportRectUVE&, Math::Vector2UVE, bool)`
- `SetGizmoModeUVE(EditorGizmoModeUVE)` / `GetGizmoModeUVE() const`
- `SetGizmoCoordinateSpaceUVE(EditorGizmoCoordinateSpaceUVE)` /
  `GetGizmoCoordinateSpaceUVE() const`
- `FocusSelectedEntityUVE()`
- `OrbitViewportUVE(float, float)`
- `PanViewportUVE(Math::Vector2UVE, const EditorViewportRectUVE&)`
- `ZoomViewportUVE(float)`
- `GetViewportCameraUVE() const`
- `GetViewportFocusPointUVE() const`
- `GetViewportDistanceUVE() const`
- `GetViewportNavigationModeUVE() const`
- `IsViewportEnvironmentPreviewEnabledUVE() const` /
  `SetViewportEnvironmentPreviewEnabledUVE(bool)`
- `IsViewportSunPreviewEnabledUVE() const` /
  `SetViewportSunPreviewEnabledUVE(bool)`

Private members removed: `m_viewportHost`, `m_viewportCamera`, `m_gizmoMode`,
`m_gizmoCoordinateSpace`, `m_gizmoDrag`, `m_viewportFocusPoint`,
`m_viewportYawRadians`, `m_viewportPitchRadians`, `m_viewportDistance`,
`m_viewportEnvironmentPreviewEnabled`, `m_viewportSunPreviewEnabled`,
`m_viewportPresetTargetYawRadians`, `m_viewportPresetTargetPitchRadians`,
`m_viewportPresetAnimating`, `m_viewportNavigationMode`, `m_viewportTab`.

Constructor: the `Core::IEditorViewportHostUVE* viewportHost = nullptr`
parameter was removed. `EditorUVE`'s constructor is now
`(Core::EngineServicesUVE&, std::filesystem::path, std::size_t,
Core::ISimulationControlUVE*)`.

## Kept (deliberately, do not confuse with the above)

- `EditorTransformAxisUVE` / `EditorTranslateAxisUVE`,
  `EditorTransformSnappingSettingsUVE`, `EditorSelectionBoundsUVE`,
  `TranslateSelectedAlongAxisUVE`, `RotateSelectedAroundWorldAxisUVE`,
  `ScaleSelectedAlongAxisUVE`, `ScaleSelectedUniformlyUVE`,
  `SetTransformSnappingSettingsUVE`/`GetTransformSnappingSettingsUVE`,
  `TryGetSelectedBoundsUVE`, `GetAxisVectorUVE`,
  `ComputeLocalDeltaForWorldDeltaUVE`,
  `ComputeLocalRotationForWorldAxisUVE`. These are the programmatic
  transform-mutation API (used by the inspector's numeric fields and by
  scripting/bridge callers) and do not depend on screen-space picking or a
  drawn gizmo. A rebuilt gizmo should call into these, not reimplement them.
- `Editor2DCanvasStateUVE`, `Get2DCanvasStateUVE`, `Set2DCanvasZoomUVE`,
  `Reset2DCanvasViewUVE`, `m_2dCanvasState`, `m_2dCanvasPanning` — unrelated
  2D screen-space authoring state for loading-screen-style content.
- `EditorReparentTransformModeUVE`, `ContentBrowserTypeFocusUVE`/
  `ContentBrowserItemTypeUVE`.

## `main.cpp` seam

`engine/app/src/editor/main.cpp` previously wired
`engine.SetActiveCameraUVE(editor.GetViewportCameraUVE())`. That call was
removed outright (not replaced with a placeholder camera). This relies on
`EngineCoreUVE::SetActiveCameraUVE`'s own documented "no active camera"
contract: with no active camera set, the engine's per-frame render step
no-ops in headless mode and clears-and-presents an empty frame in windowed
mode. This was a judgment call made to honor the "one pass, no
interruptions" instruction rather than pausing to ask; it is not a stub —
there is no camera to wire up until a new viewport owns one.

`SetPostRenderCallbackUVE([&editor] { editor.RenderOverlayUVE(); })` was left
unchanged: `RenderOverlayUVE()` still draws the editor's own ImGui chrome
(panels, menus, dialogs) and has nothing to do with the removed 3D viewport.

`SetEditorViewportRegionUVE` and `TickUVE` seams: `TickUVE()` itself is
unchanged (it still runs the editor's per-frame bookkeeping — selection
pruning, autosave, layout-preset animation timers unrelated to the camera).
No `SetEditorViewportRegionUVE` call existed in `main.cpp` to remove; the
viewport-rect plumbing it would have fed (`EditorViewportRectUVE`) is gone.

## Renderer interface removed (`engine/render/include/uve/render/i_renderer_3d_uve.h`, `renderer_3d_uve.h`, `renderer_3d_uve.cpp`)

- `struct EditorViewportVisualStateUVE` (all fields: enabled,
  environmentPreviewEnabled, sunPreviewEnabled, viewport min/max XY,
  activeSelectionVisible + selection min/max XY, activeGizmoAxis, camera
  position/forward/right/up, gridOrigin, cameraTanHalfFov, gridSpacing,
  orthographic, orthographicScale).
- `struct GizmoOverlayItemUVE` (worldMatrix, color).
- `virtual void SetEditorViewportVisualStateUVE(const EditorViewportVisualStateUVE&)`
  (was a no-op-by-default virtual on `IRenderer3DUVE`).
- `virtual void SetEditorGizmoOverlayItemsUVE(std::span<const GizmoOverlayItemUVE>)`
  (same).
- `Renderer3DFrameDiagnosticsUVE` fields: `editorVisualProgramReady`,
  `editorVisualPassRecorded`, `gizmoOverlayItemsSubmitted`,
  `gizmoOverlayDrawCallsRecorded`, `gizmoOverlayProgramReady`,
  `gizmoOverlayPassRecorded`.
- The "EditorViewportEnvironment" and "EditorGizmoOverlay" render-graph pass
  registrations inside `Renderer3DUVE::RenderFrameUVE`, their shader-program
  and GPU-mesh lifecycle (grid quad, gizmo arrow mesh), and
  `EffectiveCameraAspectRatioUVE()` (only used by the environment pass to
  correct for non-square viewports).

## Shaders removed

- `kEditorViewportEnvironmentVertexSource` /
  `kEditorViewportEnvironmentFragmentSource` (declarations in
  `built_in_shaders_uve.h`, bodies in `built_in_shaders_uve.cpp`) and the
  physical file `engine/render/shader/built_in/editor_viewport_environment.glsl`.
- `kGizmoLit3DVertexSource` / `kGizmoLit3DFragmentSource` and
  `engine/render/shader/built_in/gizmo_lit_3d.glsl`.
- `tools/embed_editor_environment_shader.py` (the standalone tool that
  regenerated the embedded environment-shader source; deleted since its one
  input file is gone).

## Icon assets removed

Per the explicit request to include "lahat ng icon ng node3d at component at
gizmo" (all Node3D, Component, and Gizmo icons) in the same pass:

- `engine/editor/assets/icons/*_node.svg` and `*_component.svg` (Node3D and
  Component icon SVGs, ~35 files).
- `engine/editor/assets/gizmos/*.svg` and `assets/gizmos/html/*.html`
  (gizmo icon SVGs and HTML authoring prototypes for move/rotate/scale/
  universal/viewport-nav gizmos).
- Generated byte tables that embedded these into the binary:
  `engine/editor/src/uve_node_icon_bytes.inc`,
  `uve_component_icon_bytes.inc`, `uve_folder_icon_display_bytes.inc`,
  `uve_gizmo_icon_display_bytes.inc`.
- `tools/generate_gizmo_icon_bytes.py`, `tools/validate_gizmo_html.py`
  (generator/validator tools with no remaining inputs).
- `tools/generate_editor_icon_bytes.py` was kept but trimmed:
  `classify_icon()` now only recognizes `GENERAL_ICON_NAMES`  and raises
  `ValueError` for anything else; the node/component/gizmo/folder
  classification branches were deleted rather than left dead.
- `EditorUiAssetsUVE` (`editor_ui_assets_uve.h`/`.cpp`) was rewritten to
  drop the folder/node/component/gizmo icon lookup systems
  (`GetFolderTextureIdUVE`, `GetNodeIconTextureIdUVE`,
  `GetComponentIconTextureIdUVE`, `GetNodeIconKeyForEntityUVE`, and the
  free-function `GetComponentIconTextureIdUVE(assets, kind)` helper in
  `editor_uve.cpp`), keeping only the logo icon, 4 general icons, and the
  10 content-browser content-type icons.
- The editor's own app/window icon (`univex_logo`) was explicitly **kept**.
  It is an in-editor ImGui chrome texture (drawn in the menu bar/title
  area), not an OS-level window or taskbar icon — this codebase has no
  OS-level window-icon system to remove, and the request explicitly
  excluded this icon.

All ~15 call sites in `editor_uve.cpp` that looked up a now-removed icon
(node/component/folder) were replaced with the literal `0U` sentinel; every
consumer already null-checks via `if (iconTextureId != 0U)` before drawing,
so this is a safe no-render fallback, not a stub — there is simply no icon
to draw where a folder/node/component icon used to appear in the hierarchy
or content browser.

## Test coverage lost

- All gizmo/viewport interaction unit tests in
  `tests/editor/editor_uve_tests.cpp` (14 whole `TEST`/`TEST_F` cases
  covering: gizmo mode switching, coordinate-space toggling, translate/
  rotate/scale/universal gizmo drag begin/update/commit across axis, plane,
  and trackball handle kinds, viewport picking/ray casting, viewport
  navigation gizmo clicks, orbit/pan/zoom camera behavior, and focus-preset
  camera framing).
- `SetEditorGizmoOverlayItemsUVE_SubmittedItems_RecordOneDrawCallEach` and
  the `Renderer3DEditorViewportAspectUVETest` fixture (3 tests covering
  aspect-ratio correction for the environment pass) in
  `tests/render/renderer_3d_uve_tests.cpp`.
  `RenderFrameUVE_VisiblePrimitive_ReportsEvidenceSpecificDiagnostics` was
  trimmed to drop its environment/gizmo-overlay diagnostics assertions
  while keeping its still-valid main-pass/tone-mapping assertions.
  Overall this file lost 234 lines.
- The two `AllBuiltInShaders` parity test-suite instantiation entries for
  `editor_viewport_environment.glsl` and `gizmo_lit_3d.glsl` in
  `tests/render/shader/shader_manager_uve_tests.cpp` (the shaders no longer
  exist, so their byte-identical-embedding parity check has nothing to
  check).
- The standalone `GizmoSystemUVE` and `ViewportNavGizmoUVE` unit test file,
  `tests/render/gizmo_overlay_geometry_uve_tests.cpp`, deleted whole (its
  subject header/source were removed).
- Node/Component/Folder icon coverage: no dedicated icon-lookup tests
  existed for these prior to removal (checked via grep across
  `tests/editor/`), so no test file needed trimming for the icon removal
  itself beyond the shader-adjacent items above.
- Net effect on `tests/editor/editor_uve_tests.cpp`: 14 whole tests deleted,
  2 renamed/trimmed to drop assertions about the now-removed viewport
  camera (`InitUVE_StartsRunningWithEmptyDocumentRootsAndSupportsHeadlessLifecycle`,
  `CreateDocumentEntityUVE_AllocatesUniqueNames`), and roughly 8 other tests
  had incidental `GetViewportCameraUVE()`/`GetGizmoModeUVE()`/
  `SetGizmoModeUVE()` references removed or replaced with a plain
  freshly-created entity where the test's real subject (selection-rejection
  safety, reparent-rejection safety, bounds-query safety) was unrelated to
  the camera itself.

## Rebuild guidance

A future viewport should not resurrect `EditorViewportRectUVE`/gizmo-drag
math verbatim; instead:

1. Own a camera entity the same way `m_viewportCamera` did (created in
   `InitUVE()`, excluded from `GetDocumentRootsUVE()`/`IsDocumentEntityUVE()`
   the way the original implementation special-cased it), and wire it back
   into `main.cpp` via `SetActiveCameraUVE`.
2. Reuse the kept `TranslateSelectedAlongAxisUVE`/
   `RotateSelectedAroundWorldAxisUVE`/`ScaleSelectedAlongAxisUVE`/
   `ScaleSelectedUniformlyUVE` API for the numeric effect of a gizmo drag;
   only the screen-space picking/dragging front-end needs to be rebuilt.
3. Re-add a render-graph pass analogous to "EditorViewportEnvironment" using
   the "copied facts pushed via `Set*()`, consumed next frame" idiom already
   used elsewhere in `Renderer3DUVE` (see `PostProcessSettingsUVE` for a
   live example of the pattern to copy).
4. Re-author gizmo icon/geometry assets from scratch rather than restoring
   the deleted SVG/HTML files verbatim, since `tools/generate_editor_icon_bytes.py`
   and `EditorUiAssetsUVE` no longer have the classification branches to
   consume them without further changes.
