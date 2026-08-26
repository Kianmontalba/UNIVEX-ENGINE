// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


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

    void ExtractRenderQueueIntoUVE(Scene::IEntityManagerUVE& entityManager, Asset::IAssetManagerUVE& assetManager,
                                   Asset::IAssetDatabaseUVE& assetDatabase, const Math::FrustumUVE& cullFrustum,
                                   RenderQueueUVE& outQueue) const override;
};

} // namespace UVE::Render
