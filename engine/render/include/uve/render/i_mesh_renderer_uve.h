// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/asset/i_asset_database_uve.h"
#include "uve/asset/i_asset_manager_uve.h"
#include "uve/math/frustum_uve.h"
#include "uve/render/render_queue_uve.h"
#include "uve/scene/i_entity_manager_uve.h"

namespace UVE::Render {

/// IMeshRendererUVE walks every entity with a WorldTransformComponentUVE + MeshComponentUVE (the
/// spec's `MeshRendererUVE`, Part 7.2), resolves its mesh/material assets, culls it against a
/// frustum, and buckets it into a sorted RenderQueueUVE — the ECS-to-draw-item extraction step
/// between CameraSystemUVE (Increment 11) and a future Renderer3DUVE (Increment 14).
/// Entities whose meshGuid/materialGuid is kInvalidAssetGuidUVE, or whose asset load hasn't
/// finished yet this frame, are silently skipped (no error) — consistent with AssetManagerUVE's
/// async, non-blocking loading design; they appear automatically once loaded.
/// Thread-safety: implementations should be stateless (holding no members), matching
/// CameraSystemUVE's contract. ExtractRenderQueueUVE itself is not thread-safe to call
/// concurrently — intended to be called once per frame from the main engine thread, after
/// SceneGraphUVE::UpdateUVE() has produced this frame's WorldTransformComponentUVE data.
class IMeshRendererUVE {
public:
    virtual ~IMeshRendererUVE() = default;

    /// Extracts this frame's RenderQueueUVE: every visible, asset-ready entity with both
    /// WorldTransformComponentUVE and MeshComponentUVE, culled against `cullFrustum` and bucketed
    /// into opaque/transparent per its MaterialAssetUVE::isTransparent.
    [[nodiscard]] virtual RenderQueueUVE ExtractRenderQueueUVE(Scene::IEntityManagerUVE& entityManager,
                                                                 Asset::IAssetManagerUVE& assetManager,
                                                                 Asset::IAssetDatabaseUVE& assetDatabase,
                                                                 const Math::FrustumUVE& cullFrustum) const = 0;

    /// Fills caller-owned queue storage. The default preserves compatibility for existing
    /// implementations/test doubles; MeshRendererUVE overrides it to reuse vector capacity.
    virtual void ExtractRenderQueueIntoUVE(Scene::IEntityManagerUVE& entityManager,
                                           Asset::IAssetManagerUVE& assetManager,
                                           Asset::IAssetDatabaseUVE& assetDatabase,
                                           const Math::FrustumUVE& cullFrustum, RenderQueueUVE& outQueue) const {
        outQueue = ExtractRenderQueueUVE(entityManager, assetManager, assetDatabase, cullFrustum);
    }
};

} // namespace UVE::Render
