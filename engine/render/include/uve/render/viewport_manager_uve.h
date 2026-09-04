// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "uve/render/i_renderer_3d_uve.h"
#include "uve/scene/entity_uve.h"
#include "uve/scene/i_entity_manager_uve.h"

namespace UVE::Render {

/// A single split-view pane: a normalized screen rect (top-left origin, matching this engine's
/// existing editor-viewport UV convention - see EditorViewportVisualStateUVE's viewportMin/Max
/// fields) plus the ECS camera entity it renders from. Plain data, not a parallel camera/player
/// abstraction: `cameraEntity` must be an existing entity with a CameraComponentUVE (and a
/// WorldTransformComponentUVE, via SceneGraphUVE, like any other camera), reusing exactly what
/// ICameraSystemUVE/Renderer3DUVE already consume for the single-camera path - a pane owns no
/// camera resource of its own.
///
/// `receivesInput`/`uiOnly` are carried as inert data this phase - no input-routing or UI-only
/// rendering path consumes them yet. They exist so a caller (e.g. a future editor input router)
/// has somewhere to record that intent without ViewportManagerUVE itself needing to change shape
/// later; RenderAllPanesUVE() below does skip a `uiOnly` pane's 3D render, since such a pane has
/// no camera-driven scene content to show by definition.
struct ViewportPaneUVE final {
    float originX01 = 0.0F;
    float originY01 = 0.0F;
    float sizeX01 = 1.0F;
    float sizeY01 = 1.0F;
    Scene::EntityUVE cameraEntity = Scene::kInvalidEntityUVE;
    bool receivesInput = true;
    bool uiOnly = false;
};

/// Renders one scene from multiple cameras into different screen regions in a single frame
/// (Phase 3) - a thin data+orchestration layer, not a parallel renderer: RenderAllPanesUVE() below
/// composes with the existing single-camera IRenderer3DUVE path by calling
/// IRenderer3DUVE::RenderFrameToRegionUVE() once per pane, the same render pipeline Phase 1/2
/// already built. Thread-safety: main render thread only, matching IRenderer3DUVE's own contract.
class ViewportManagerUVE final {
public:
    void AddPaneUVE(ViewportPaneUVE pane);

    /// Removes the pane at `index`. Out-of-range indices are a no-op (logged), never an assert or
    /// a crash - a caller driving this from user-facing editor UI (e.g. closing a pane panel) must
    /// not be able to trigger undefined behavior from a stale index.
    void RemovePaneUVE(std::size_t index);

    [[nodiscard]] std::size_t GetPaneCountUVE() const noexcept;

    /// Returns nullptr for an out-of-range index rather than asserting, matching RemovePaneUVE()'s
    /// own caller-safety contract above.
    [[nodiscard]] const ViewportPaneUVE* GetPaneUVE(std::size_t index) const noexcept;

    /// A simple point-in-rect test against each pane's normalized rect, most-recently-added pane
    /// first (the reference material's equivalent function was a handful of comparisons - this
    /// deliberately stays that simple). Iterating back-to-front means an overlapping pane added
    /// later - the one a viewer would perceive as "on top" if panes ever do overlap - wins the hit
    /// test, matching ordinary UI hit-testing convention; UVE does not currently require panes to
    /// be non-overlapping. Returns std::nullopt if `x01`/`y01` falls outside every pane.
    [[nodiscard]] std::optional<std::size_t> FindPaneAtNormalizedPositionUVE(float x01, float y01) const noexcept;

    /// Renders every non-`uiOnly` pane with a live camera entity into its own region of a
    /// `windowWidth`x`windowHeight` presentation surface, in pane order. For each pane: resizes
    /// `renderer`'s offscreen target to the pane's own pixel dimensions (so its camera's aspect
    /// ratio - and GPU cost - match what's actually displayed, not the full window) via
    /// IRenderer3DUVE::ResizeTargetsUVE(), then calls RenderFrameToRegionUVE() with that same pixel
    /// rect converted to OpenGL's bottom-left-origin viewport convention (this struct's own
    /// `originY01` is top-left, matching this engine's other screen-space UI conventions).
    ///
    /// Known simplification: resizing the shared renderer's offscreen target once per pane means
    /// consecutive panes of different pixel sizes each pay a GPU texture reallocation - correct,
    /// but not the fastest possible approach (a per-pane-cached target pool would avoid the churn).
    /// Acceptable for this phase: "compose with the existing single-camera render path rather than
    /// requiring a parallel one" was the explicit design constraint, and a resize is already the
    /// established mechanism that path uses for adaptive resolution.
    void RenderAllPanesUVE(IRenderer3DUVE& renderer, Scene::IEntityManagerUVE& entityManager,
                            std::uint32_t windowWidth, std::uint32_t windowHeight) const;

private:
    std::vector<ViewportPaneUVE> m_panes;
};

} // namespace UVE::Render
