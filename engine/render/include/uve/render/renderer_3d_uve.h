// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>
#include <memory>

#include "uve/asset/i_asset_database_uve.h"
#include "uve/asset/i_asset_manager_uve.h"
#include "uve/events/i_event_system_uve.h"
#include "uve/math/vector3_uve.h"
#include "uve/render/i_camera_system_uve.h"
#include "uve/render/i_light_system_uve.h"
#include "uve/render/i_mesh_renderer_uve.h"
#include "uve/render/i_render_device_uve.h"
#include "uve/render/i_render_system_uve.h"
#include "uve/render/i_renderer_3d_uve.h"
#include "uve/render/shader/i_shader_manager_uve.h"

namespace UVE::Render {

/// Renderer3DUVE is the concrete, engine-standard implementation of IRenderer3DUVE. Unlike
/// CameraSystemUVE/MeshRendererUVE, it is deliberately stateful (PIMPL, matching
/// AssetManagerUVE's/NullRenderDeviceUVE's precedent for services that own real resources) — it
/// owns the offscreen render target and a GPU-resource cache (mesh GUID -> vertex/index buffers,
/// material GUID -> pipeline) that must persist and be invalidated across frames.
class Renderer3DUVE final : public IRenderer3DUVE {
public:
    /// Constructs the offscreen color+depth render target at `targetWidth` x `targetHeight` and
    /// subscribes to Asset::AssetReloadedEventUVE for GPU-resource-cache
    /// invalidation. `ambientColor` is the flat ambient term added to every item's final color
    /// every frame regardless of whether an active light exists (see EngineConfigUVE::ambientColor,
    /// Increment 23). `shaderManager` compiles the built-in shadow-depth program used by the
    /// directional-light shadow pre-pass (Increment 26); `shadowMapResolution`/
    /// `shadowMapHalfExtent`/`shadowMapNearPlane`/`shadowMapFarPlane` size and bound that
    /// pre-pass's orthographic projection (see EngineConfigUVE's matching fields). Increment 29
    /// derives the projection's actual bounds from the active camera frustum and applies
    /// `shadowFrustumPadding` to protect against edge clipping. `shadowCascadeSplitLambda`
    /// controls the bounded three-cascade practical split distribution. `shadowCascadeBlendRatio`
    /// cross-fades cascade transitions. `shadowPcfKernelRadius` controls the canonical material
    /// shader's bounded PCF filter: zero
    /// retains a hard shadow and one produces a 3x3 kernel. Every reference must outlive this
    /// Renderer3DUVE.
    Renderer3DUVE(IRenderDeviceUVE& renderDevice, IRenderSystemUVE& renderSystem, IMeshRendererUVE& meshRenderer,
                  ICameraSystemUVE& cameraSystem, ILightSystemUVE& lightSystem,
                  Shader::IShaderManagerUVE& shaderManager, Asset::IAssetManagerUVE& assetManager,
                  Asset::IAssetDatabaseUVE& assetDatabase, Events::IEventSystemUVE& eventSystem,
                  std::uint32_t targetWidth, std::uint32_t targetHeight, Math::Vector3UVE ambientColor,
                  std::uint32_t shadowMapResolution, float shadowMapHalfExtent, float shadowMapNearPlane,
                  float shadowMapFarPlane, float shadowFrustumPadding, float shadowCascadeSplitLambda,
                  float shadowCascadeBlendRatio, std::uint32_t shadowPcfKernelRadius);
    ~Renderer3DUVE() override;

    Renderer3DUVE(const Renderer3DUVE&) = delete;
    Renderer3DUVE& operator=(const Renderer3DUVE&) = delete;

    void RenderFrameUVE(Scene::IEntityManagerUVE& entityManager, Scene::EntityUVE cameraEntity) override;
    [[nodiscard]] bool ResizeTargetsUVE(std::uint32_t width, std::uint32_t height) override;
    void RenderFrameWithParticleRuntimeUVE(Scene::IEntityManagerUVE& entityManager, Scene::EntityUVE cameraEntity,
                                           const Scene::ParticleRuntimeUVE& particleRuntime) override;
    void RenderFrameToRegionUVE(Scene::IEntityManagerUVE& entityManager, Scene::EntityUVE cameraEntity,
                                const ViewportRectUVE& region,
                                const Scene::ParticleRuntimeUVE* particleRuntime = nullptr) override;
    void SetEditorViewportVisualStateUVE(const EditorViewportVisualStateUVE& state) override;
    void SetEditorGizmoOverlayItemsUVE(std::span<const GizmoOverlayItemUVE> items) override;
    void SetPostProcessSettingsUVE(const PostProcessSettingsUVE& settings) override;
    [[nodiscard]] Renderer3DFrameDiagnosticsUVE GetLastFrameDiagnosticsUVE() const noexcept override;

private:
    struct ImplUVE;
    std::unique_ptr<ImplUVE> m_impl;
};

} // namespace UVE::Render
