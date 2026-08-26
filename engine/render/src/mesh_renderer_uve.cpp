

#include "uve/render/mesh_renderer_uve.h"

#include <cstddef>
#include <utility>

#include "uve/asset/asset_guid_uve.h"
#include "uve/debug/assert_uve.h"
#include "uve/render/mesh_render_eligibility_uve.h"
#include "uve/scene/components/mesh_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Render {

RenderQueueUVE MeshRendererUVE::ExtractRenderQueueUVE(Scene::IEntityManagerUVE& entityManager,
                                                        Asset::IAssetManagerUVE& assetManager,
                                                        Asset::IAssetDatabaseUVE& assetDatabase,
                                                        const Math::FrustumUVE& cullFrustum) const {
    RenderQueueUVE queue;
    ExtractRenderQueueIntoUVE(entityManager, assetManager, assetDatabase, cullFrustum, queue);
    return queue;
}

void MeshRendererUVE::ExtractRenderQueueIntoUVE(Scene::IEntityManagerUVE& entityManager,
                                            Asset::IAssetManagerUVE& assetManager,
                                            Asset::IAssetDatabaseUVE& assetDatabase,
                                            const Math::FrustumUVE& cullFrustum, RenderQueueUVE& outQueue) const {
    outQueue.ClearUVE();

    entityManager.ForEachUVE<Scene::WorldTransformComponentUVE, Scene::MeshComponentUVE>(
        [&](Scene::EntityUVE, const Scene::WorldTransformComponentUVE& worldTransform,
            const Scene::MeshComponentUVE& meshComponent) {
            UVE_ASSERT(Scene::IsMeshComponentValidUVE(meshComponent));
            if (meshComponent.meshGuid == Asset::kInvalidAssetGuidUVE) {
                ++outQueue.invalidAssetReferences;
            }
            if (meshComponent.materialGuid == Asset::kInvalidAssetGuidUVE) {
                ++outQueue.invalidAssetReferences;
            }
            if (meshComponent.meshGuid == Asset::kInvalidAssetGuidUVE ||
                meshComponent.materialGuid == Asset::kInvalidAssetGuidUVE) {
                return;
            }

            Asset::AssetHandleUVE<Asset::MeshAssetUVE> meshHandle =
                assetManager.LoadUVE<Asset::MeshAssetUVE>(meshComponent.meshGuid, assetDatabase);
            Asset::AssetHandleUVE<Asset::MaterialAssetUVE> materialHandle =
                assetManager.LoadUVE<Asset::MaterialAssetUVE>(meshComponent.materialGuid, assetDatabase);
            const bool meshFailed = meshHandle.HasFailedUVE();
            const bool materialFailed = materialHandle.HasFailedUVE();
            outQueue.failedAssetLoads += static_cast<std::size_t>(meshFailed) + static_cast<std::size_t>(materialFailed);
            const bool meshPending = !meshFailed && !meshHandle.IsReadyUVE();
            const bool materialPending = !materialFailed && !materialHandle.IsReadyUVE();
            outQueue.pendingAssetLoads += static_cast<std::size_t>(meshPending) + static_cast<std::size_t>(materialPending);
            if (meshPending || materialPending || meshFailed || materialFailed) {
                return;
            }

            const Asset::MeshAssetUVE* const mesh = meshHandle.TryGetUVE();
            const Asset::MaterialAssetUVE* const material = materialHandle.TryGetUVE();

            MeshRenderEligibilityUVE eligibility;
            if (!EvaluateMeshRenderEligibilityUVE(meshComponent, worldTransform, *mesh, cullFrustum, eligibility)) {
                if (eligibility.reason == MeshRenderEligibilityReasonUVE::InvalidWorldTransform ||
                    eligibility.reason == MeshRenderEligibilityReasonUVE::InvalidLocalBounds) {
                    ++outQueue.invalidRenderEligibility;
                }
                return;
            }

            RenderItemUVE item{eligibility.worldMatrix, std::move(meshHandle), std::move(materialHandle),
                               eligibility.sortDepth};
            if (material->isTransparent) {
                outQueue.transparentItems.push_back(std::move(item));
            } else {
                outQueue.opaqueItems.push_back(std::move(item));
            }
        });
}

} // namespace UVE::Render
