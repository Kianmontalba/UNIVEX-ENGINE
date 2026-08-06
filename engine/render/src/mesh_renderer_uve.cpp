// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/mesh_renderer_uve.h"

#include <cstddef>
#include <utility>

#include "uve/asset/asset_guid_uve.h"
#include "uve/math/aabb_uve.h"
#include "uve/scene/components/mesh_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Render {

namespace {

/// The near plane's index within FrustumUVE::planes ("left, right, bottom, top, near, far").
constexpr std::size_t kNearPlaneIndexUVE = 4;

} // namespace

RenderQueueUVE MeshRendererUVE::ExtractRenderQueueUVE(Scene::IEntityManagerUVE& entityManager,
                                                        Asset::IAssetManagerUVE& assetManager,
                                                        Asset::IAssetDatabaseUVE& assetDatabase,
                                                        const Math::FrustumUVE& cullFrustum) const {
    RenderQueueUVE queue;

    entityManager.ForEachUVE<Scene::WorldTransformComponentUVE, Scene::MeshComponentUVE>(
        [&](Scene::EntityUVE, const Scene::WorldTransformComponentUVE& worldTransform,
            const Scene::MeshComponentUVE& meshComponent) {
            if (meshComponent.meshGuid == Asset::kInvalidAssetGuidUVE ||
                meshComponent.materialGuid == Asset::kInvalidAssetGuidUVE) {
                return;
            }

            Asset::AssetHandleUVE<Asset::MeshAssetUVE> meshHandle =
                assetManager.LoadUVE<Asset::MeshAssetUVE>(meshComponent.meshGuid, assetDatabase);
            Asset::AssetHandleUVE<Asset::MaterialAssetUVE> materialHandle =
                assetManager.LoadUVE<Asset::MaterialAssetUVE>(meshComponent.materialGuid, assetDatabase);
            if (!meshHandle.IsReadyUVE() || !materialHandle.IsReadyUVE()) {
                return;
            }

            const Asset::MeshAssetUVE* const mesh = meshHandle.TryGetUVE();
            const Asset::MaterialAssetUVE* const material = materialHandle.TryGetUVE();

            const Math::Matrix4x4UVE worldMatrix = Math::Matrix4x4UVE::ComposeTrsUVE(
                worldTransform.worldPosition, worldTransform.worldRotation, worldTransform.worldScale);
            const Math::AabbUVE worldAabb = mesh->localBounds.TransformUVE(worldMatrix);
            if (!cullFrustum.IntersectsUVE(worldAabb)) {
                return;
            }

            const float sortDepth =
                cullFrustum.planes[kNearPlaneIndexUVE].GetSignedDistanceUVE(worldAabb.GetCenterUVE());

            RenderItemUVE item{worldMatrix, std::move(meshHandle), std::move(materialHandle), sortDepth};
            if (material->isTransparent) {
                queue.transparentItems.push_back(std::move(item));
            } else {
                queue.opaqueItems.push_back(std::move(item));
            }
        });

    return queue;
}

} // namespace UVE::Render
