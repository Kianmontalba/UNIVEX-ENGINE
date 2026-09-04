// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstddef>
#include <cstdint>

#include "uve/math/vector3_uve.h"
#include "uve/render/render_resource_descs_uve.h"
#include "uve/scene/entity_uve.h"
#include "uve/scene/i_entity_manager_uve.h"
#include "uve/scene/particle_runtime_uve.h"

namespace UVE::Render {

/// Value-only editor visual facts consumed by the native renderer's optional composite pass.
/// Coordinates are normalized to the presentation surface; invalid or inverted rectangles disable
/// their feature instead of requesting undefined GPU work. Thread-safety: main render thread only.
struct EditorViewportVisualStateUVE final {
    bool enabled = false;
    bool environmentPreviewEnabled = true;
    bool sunPreviewEnabled = true;
    float viewportMinX = 0.0F;
    float viewportMinY = 0.0F;
    float viewportMaxX = 1.0F;
    float viewportMaxY = 1.0F;
    bool activeSelectionVisible = false;
    float selectionMinX = 0.0F;
    float selectionMinY = 0.0F;
    float selectionMaxX = 0.0F;
    float selectionMaxY = 0.0F;
    std::int32_t activeGizmoAxis = 0;
    Math::Vector3UVE cameraPosition{0.0F, 0.0F, 0.0F};
    Math::Vector3UVE cameraForward{0.0F, 0.0F, -1.0F};
    Math::Vector3UVE cameraRight{1.0F, 0.0F, 0.0F};
    Math::Vector3UVE cameraUp{0.0F, 1.0F, 0.0F};
    Math::Vector3UVE gridOrigin{0.0F, 0.0F, 0.0F};
    float cameraTanHalfFov = 0.57735026F;
    float gridSpacing = 1.0F;
    bool orthographic = false;
    float orthographicScale = 10.0F;
};

/// Phase 2b post-process quality-tier toggles. Both default to enabled, matching this project's
/// "on unless a low-end tier opts out" precedent already set by shadow mapping; each is checked
/// independently when Renderer3DUVE builds its per-frame render graph, so disabling one skips that
/// pass's GPU work entirely rather than merely hiding its visual contribution.
struct PostProcessSettingsUVE final {
    bool bloomEnabledUVE = true;
    bool ssaoEnabledUVE = true;
};

/// A copied, frame-local account of observable Renderer3DUVE work. Each field names evidence that
/// the renderer actually has: extraction and recorded-pass/draw counts are CPU-side facts;
/// `glDrawCallsIssued` means a draw call was dispatched through the immediate OpenGL backend, not
/// that the driver completed it or that a compositor displayed pixels. Presentation and pixel
/// verification belong to EngineCoreUVE and dedicated real-GL integration tests respectively.
/// Thread-safety: returned by value; RenderFrameUVE() and this accessor remain main-thread only.
struct Renderer3DFrameDiagnosticsUVE final {
    std::uint32_t renderTargetWidth = 0U;
    std::uint32_t renderTargetHeight = 0U;
    std::size_t meshItemsExtracted = 0U;
    std::size_t invalidAssetReferences = 0U;
    std::size_t pendingAssetLoads = 0U;
    std::size_t failedAssetLoads = 0U;
    std::size_t textureFallbacks = 0U;
    std::size_t primitiveCandidates = 0U;
    std::size_t primitiveItemsExtracted = 0U;
    std::size_t meshDrawCallsRecorded = 0U;
    std::size_t primitiveDrawCallsRecorded = 0U;
    std::size_t particleItemsExtracted = 0U;
    std::size_t particleDrawCommandsRecorded = 0U;
    std::size_t particleDrawCommandsSubmitted = 0U;
    std::size_t particleDrawCallsRecorded = 0U;
    std::size_t glDrawCallsIssued = 0U;
    bool primitiveProgramReady = false;
    bool particleProgramReady = false;
    bool mainPassRecorded = false;
    bool toneMappingProgramReady = false;
    bool toneMappingPassRecorded = false;
    bool editorVisualProgramReady = false;
    bool editorVisualPassRecorded = false;
    bool particleItemsTruncated = false;
    bool particleDrawCommandsSubmissionTruncated = false;
    /// True only when SSAO was enabled (PostProcessSettingsUVE), its post-process targets and
    /// programs were valid, and the camera's projection matrix was invertible this frame - not
    /// merely that SSAO was requested.
    bool ssaoPassRecorded = false;
    /// True only when bloom was enabled (PostProcessSettingsUVE) and its post-process targets and
    /// programs were valid this frame - not merely that bloom was requested.
    bool bloomPassRecorded = false;
};

/// IRenderer3DUVE is the engine's final per-frame render orchestrator (the spec's `Renderer3DUVE`,
/// Part 7.2): for a given camera entity, it computes the view-projection, culls and sorts the
/// scene into a RenderQueueUVE (via IMeshRendererUVE), lazily uploads each encountered mesh's/
/// material's GPU-shaped resources (cached, invalidated on hot reload), records resulting draw
/// calls into an ICommandBufferUVE, and renders to an internally owned HDR color/depth target.
/// The built-in tone-mapping pass then writes the color target to the backend default framebuffer;
/// EngineCoreUVE owns the subsequent overlay callback and explicit PresentUVE() request.
/// Thread-safety: not thread-safe. RenderFrameUVE() must be called once per frame from the main
/// engine thread only, after SceneGraphUVE::UpdateUVE() has run for this frame (it needs current
/// WorldTransformComponentUVE data), matching EngineCoreUVE's single-threaded Update -> Render
/// frame-loop contract.
class IRenderer3DUVE {
public:
    virtual ~IRenderer3DUVE() = default;

    /// Renders the scene as seen by `cameraEntity`: extract -> sort -> record -> submit ->
    /// tone-map to the current backend presentation surface. `cameraEntity` must have both
    /// WorldTransformComponentUVE and CameraComponentUVE (the same contract ICameraSystemUVE
    /// already enforces).
    virtual void RenderFrameUVE(Scene::IEntityManagerUVE& entityManager, Scene::EntityUVE cameraEntity) = 0;

    /// Recreates the owned color/depth targets at a validated adaptive resolution. The default is
    /// a safe no-op for lightweight renderers and test doubles that do not own offscreen targets.
    /// Must be called on the main render thread while no frame command buffer is active.
    [[nodiscard]] virtual bool ResizeTargetsUVE(std::uint32_t width, std::uint32_t height) {
        static_cast<void>(width);
        static_cast<void>(height);
        return false;
    }

    /// Renders the scene while extracting a copied particle snapshot from the caller-owned runtime
    /// into the frame queue. The runtime reference is borrowed for this call only; implementations
    /// must not retain it after returning. The distinct name avoids hiding the legacy virtual in test doubles.
    virtual void RenderFrameWithParticleRuntimeUVE(Scene::IEntityManagerUVE& entityManager,
                                                   Scene::EntityUVE cameraEntity,
                                                   const Scene::ParticleRuntimeUVE& particleRuntime) {
        static_cast<void>(particleRuntime);
        RenderFrameUVE(entityManager, cameraEntity);
    }

    /// Renders exactly like RenderFrameUVE(), except the final presentation write targets `region`
    /// (a pixel sub-rect of the presentation surface) instead of its entire area - Phase 3's
    /// ViewportManagerUVE split-view support. The default implementation ignores `region` and
    /// falls back to a full-surface RenderFrameUVE(), matching this interface's existing
    /// safe-no-op-default convention for test doubles/lightweight renderers that don't own a
    /// resizable offscreen target (see ResizeTargetsUVE()'s own default above) - such a renderer
    /// has no sub-region concept to honor, so a full render is the closest correct behavior rather
    /// than silently dropping the frame.
    virtual void RenderFrameToRegionUVE(Scene::IEntityManagerUVE& entityManager, Scene::EntityUVE cameraEntity,
                                        const ViewportRectUVE& region) {
        static_cast<void>(region);
        RenderFrameUVE(entityManager, cameraEntity);
    }

    /// Updates optional copied editor-only visual facts for a later native render frame. The default
    /// implementation is intentionally a no-op so non-Renderer3D test doubles need not own editor state.
    virtual void SetEditorViewportVisualStateUVE(const EditorViewportVisualStateUVE& state) {
        static_cast<void>(state);
    }

    /// Updates the Phase 2b post-process quality-tier toggles for later render frames. The default
    /// implementation is intentionally a no-op so non-Renderer3D test doubles need not own
    /// post-process state.
    virtual void SetPostProcessSettingsUVE(const PostProcessSettingsUVE& settings) {
        static_cast<void>(settings);
    }

    /// Returns the last frame's copied renderer evidence snapshot. The snapshot intentionally does
    /// not claim a completed GPU frame or visible window pixels; use the real-GL integration tests
    /// for that stronger presentation proof.
    [[nodiscard]] virtual Renderer3DFrameDiagnosticsUVE GetLastFrameDiagnosticsUVE() const noexcept = 0;
};

} // namespace UVE::Render
