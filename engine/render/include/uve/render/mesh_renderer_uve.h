//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include "uve/render/i_mesh_renderer_uve.h"

namespace UVE::Render {

/// MeshRendererUVE is the concrete, engine-standard implementation of IMeshRendererUVE.
/// Deliberately stateless (no members, no PIMPL) — matches CameraSystemUVE's/SceneGraphUVE's
/// precedent for utility services that always take the IEntityManagerUVE&/IAssetManagerUVE& they
/// operate on as explicit parameters rather than owning references to them.
class MeshRendererUVE final : public IMeshRendererUVE {
public:
    [[nodiscard]] RenderQueueUVE ExtractRenderQueueUVE(Scene::IEntityManagerUVE& entityManager,
                                                         Asset::IAssetManagerUVE& assetManager,
                                                         Asset::IAssetDatabaseUVE& assetDatabase,
                                                         const Math::FrustumUVE& cullFrustum) const override;
};

} // namespace UVE::Render
