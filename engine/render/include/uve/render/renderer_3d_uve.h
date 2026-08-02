//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

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

namespace UVE::Render {

/// Renderer3DUVE is the concrete, engine-standard implementation of IRenderer3DUVE. Unlike
/// CameraSystemUVE/MeshRendererUVE, it is deliberately stateful (PIMPL, matching
/// AssetManagerUVE's/NullRenderDeviceUVE's precedent for services that own real resources) — it
/// owns the offscreen render target and a GPU-resource cache (mesh GUID -> vertex/index buffers,
/// material GUID -> pipeline) that must persist and be invalidated across frames.
class Renderer3DUVE final : public IRenderer3DUVE {
public:
    /// Constructs the offscreen color+depth render target at `targetWidth` x `targetHeight`
    /// (fixed for this Renderer3DUVE's lifetime — no WindowManagerUVE exists yet to resize
    /// against) and subscribes to Asset::AssetReloadedEventUVE for GPU-resource-cache
    /// invalidation. `ambientColor` is the flat ambient term added to every item's final color
    /// every frame regardless of whether an active light exists (see EngineConfigUVE::ambientColor,
    /// Increment 23). Every reference must outlive this Renderer3DUVE.
    Renderer3DUVE(IRenderDeviceUVE& renderDevice, IRenderSystemUVE& renderSystem, IMeshRendererUVE& meshRenderer,
                  ICameraSystemUVE& cameraSystem, ILightSystemUVE& lightSystem, Asset::IAssetManagerUVE& assetManager,
                  Asset::IAssetDatabaseUVE& assetDatabase, Events::IEventSystemUVE& eventSystem,
                  std::uint32_t targetWidth, std::uint32_t targetHeight, Math::Vector3UVE ambientColor);
    ~Renderer3DUVE() override;

    Renderer3DUVE(const Renderer3DUVE&) = delete;
    Renderer3DUVE& operator=(const Renderer3DUVE&) = delete;

    void RenderFrameUVE(Scene::IEntityManagerUVE& entityManager, Scene::EntityUVE cameraEntity) override;

private:
    struct ImplUVE;
    std::unique_ptr<ImplUVE> m_impl;
};

} // namespace UVE::Render
