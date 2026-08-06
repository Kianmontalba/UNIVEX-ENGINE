// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/mesh_renderer_uve.h"

#include <atomic>
#include <chrono>
#include <numbers>
#include <thread>

#include <gtest/gtest.h>

#include "uve/asset/asset_database_uve.h"
#include "uve/asset/asset_manager_uve.h"
#include "uve/asset/material_asset_uve.h"
#include "uve/asset/mesh_asset_uve.h"
#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/scene/components/mesh_component_uve.h"
#include "uve/scene/entity_manager_uve.h"
#include "uve/scene/scene_graph_uve.h"
#include "uve/threading/thread_pool_uve.h"

namespace UVE::Render::Tests {
namespace {

constexpr int kMaxPollIterationsUVE = 200000;

class MeshRendererUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    Scene::EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    Scene::SceneGraphUVE sceneGraph;
    Threading::ThreadPoolUVE threadPool{2};
    Asset::AssetDatabaseUVE assetDatabase;
    Asset::AssetManagerUVE assetManager{threadPool, eventSystem};
    MeshRendererUVE meshRenderer;

    // Entities reach MeshRendererUVE only via SceneGraphUVE::AttachTransformUVE, matching
    // CameraSystemUVETest's fixture precedent.
    Scene::EntityUVE MakeMeshEntityUVE(Math::Vector3UVE worldPosition, Asset::AssetGuidUVE meshGuid,
                                        Asset::AssetGuidUVE materialGuid) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE local;
        local.localPosition = worldPosition;
        sceneGraph.AttachTransformUVE(entityManager, entity, local);
        sceneGraph.UpdateUVE(entityManager);
        entityManager.AddComponentUVE<Scene::MeshComponentUVE>(entity, Scene::MeshComponentUVE{meshGuid, materialGuid});
        return entity;
    }

    // A camera at the origin looking down -Z with a 90-degree fov, matching
    // CameraSystemUVETest's ExtractFrustumUVE fixture.
    [[nodiscard]] static Math::FrustumUVE MakeTestFrustumUVE() {
        const Math::Matrix4x4UVE view =
            Math::Matrix4x4UVE::ViewFromPositionAndRotationUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::QuaternionUVE{});
        const Math::Matrix4x4UVE projection =
            Math::Matrix4x4UVE::PerspectiveUVE(std::numbers::pi_v<float> / 2.0F, 1.0F, 1.0F, 100.0F);
        return Math::FrustumUVE::FromViewProjectionUVE(projection * view);
    }

    void RegisterImmediateLoadersUVE(bool materialIsTransparent) {
        assetManager.RegisterLoaderUVE<Asset::MeshAssetUVE>(
            [](const std::filesystem::path&, Asset::MeshAssetUVE& mesh) {
                mesh.localBounds =
                    Math::AabbUVE::FromCenterExtentsUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{0.5F, 0.5F, 0.5F});
                return true;
            });
        assetManager.RegisterLoaderUVE<Asset::MaterialAssetUVE>(
            [materialIsTransparent](const std::filesystem::path&, Asset::MaterialAssetUVE& material) {
                material.isTransparent = materialIsTransparent;
                return true;
            });
    }

    void WaitUntilAssetsReadyUVE(Asset::AssetGuidUVE meshGuid, Asset::AssetGuidUVE materialGuid) {
        Asset::AssetHandleUVE<Asset::MeshAssetUVE> meshHandle =
            assetManager.LoadUVE<Asset::MeshAssetUVE>(meshGuid, assetDatabase);
        Asset::AssetHandleUVE<Asset::MaterialAssetUVE> materialHandle =
            assetManager.LoadUVE<Asset::MaterialAssetUVE>(materialGuid, assetDatabase);
        for (int iteration = 0; iteration < kMaxPollIterationsUVE; ++iteration) {
            if (meshHandle.IsReadyUVE() && materialHandle.IsReadyUVE()) {
                break;
            }
            std::this_thread::yield();
        }
        ASSERT_TRUE(meshHandle.IsReadyUVE());
        ASSERT_TRUE(materialHandle.IsReadyUVE());
    }
};

TEST_F(MeshRendererUVETest, ExtractRenderQueueUVE_VisibleOpaqueEntity_AppearsInOpaqueBucket) {
    RegisterImmediateLoadersUVE(/*materialIsTransparent=*/false);
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("mesh_renderer_tests_opaque.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("mesh_renderer_tests_opaque.uvemat");
    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid);

    const RenderQueueUVE queue =
        meshRenderer.ExtractRenderQueueUVE(entityManager, assetManager, assetDatabase, MakeTestFrustumUVE());

    ASSERT_EQ(queue.opaqueItems.size(), 1U);
    EXPECT_TRUE(queue.transparentItems.empty());
}

TEST_F(MeshRendererUVETest, ExtractRenderQueueUVE_VisibleTransparentEntity_AppearsInTransparentBucket) {
    RegisterImmediateLoadersUVE(/*materialIsTransparent=*/true);
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("mesh_renderer_tests_transparent.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("mesh_renderer_tests_transparent.uvemat");
    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid);

    const RenderQueueUVE queue =
        meshRenderer.ExtractRenderQueueUVE(entityManager, assetManager, assetDatabase, MakeTestFrustumUVE());

    EXPECT_TRUE(queue.opaqueItems.empty());
    ASSERT_EQ(queue.transparentItems.size(), 1U);
}

TEST_F(MeshRendererUVETest, ExtractRenderQueueUVE_EntityOutsideFrustum_Excluded) {
    RegisterImmediateLoadersUVE(/*materialIsTransparent=*/false);
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("mesh_renderer_tests_offscreen.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("mesh_renderer_tests_offscreen.uvemat");
    // Behind the camera, matching CameraSystemUVETest's own "behindCameraBox" fixture.
    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 10.0F}, meshGuid, materialGuid);
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid);

    const RenderQueueUVE queue =
        meshRenderer.ExtractRenderQueueUVE(entityManager, assetManager, assetDatabase, MakeTestFrustumUVE());

    EXPECT_TRUE(queue.opaqueItems.empty());
    EXPECT_TRUE(queue.transparentItems.empty());
}

TEST_F(MeshRendererUVETest, ExtractRenderQueueUVE_InvalidGuid_Skipped) {
    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, Asset::kInvalidAssetGuidUVE, Asset::kInvalidAssetGuidUVE);

    const RenderQueueUVE queue =
        meshRenderer.ExtractRenderQueueUVE(entityManager, assetManager, assetDatabase, MakeTestFrustumUVE());

    EXPECT_TRUE(queue.opaqueItems.empty());
    EXPECT_TRUE(queue.transparentItems.empty());
}

TEST_F(MeshRendererUVETest, ExtractRenderQueueUVE_MixOfVisibleAndCulledEntities_OnlyVisibleOneIncluded) {
    RegisterImmediateLoadersUVE(/*materialIsTransparent=*/false);
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("mesh_renderer_tests_mixed.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("mesh_renderer_tests_mixed.uvemat");
    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    MakeMeshEntityUVE(Math::Vector3UVE{1000.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid);

    const RenderQueueUVE queue =
        meshRenderer.ExtractRenderQueueUVE(entityManager, assetManager, assetDatabase, MakeTestFrustumUVE());

    EXPECT_EQ(queue.opaqueItems.size(), 1U);
}

TEST_F(MeshRendererUVETest, ExtractRenderQueueUVE_AssetNotReadyYet_ExcludedThenIncludedOnceLoaded) {
    std::atomic<bool> allowMeshLoad{false};
    assetManager.RegisterLoaderUVE<Asset::MeshAssetUVE>(
        [&allowMeshLoad](const std::filesystem::path&, Asset::MeshAssetUVE& mesh) {
            while (!allowMeshLoad.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            mesh.localBounds =
                Math::AabbUVE::FromCenterExtentsUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{0.5F, 0.5F, 0.5F});
            return true;
        });
    assetManager.RegisterLoaderUVE<Asset::MaterialAssetUVE>(
        [](const std::filesystem::path&, Asset::MaterialAssetUVE& material) {
            material.isTransparent = false;
            return true;
        });

    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("mesh_renderer_tests_gate.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("mesh_renderer_tests_gate.uvemat");
    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    const Math::FrustumUVE frustum = MakeTestFrustumUVE();

    const RenderQueueUVE firstAttempt = meshRenderer.ExtractRenderQueueUVE(entityManager, assetManager, assetDatabase, frustum);
    EXPECT_TRUE(firstAttempt.opaqueItems.empty());

    allowMeshLoad.store(true);

    RenderQueueUVE secondAttempt;
    for (int iteration = 0; iteration < kMaxPollIterationsUVE; ++iteration) {
        secondAttempt = meshRenderer.ExtractRenderQueueUVE(entityManager, assetManager, assetDatabase, frustum);
        if (!secondAttempt.opaqueItems.empty()) {
            break;
        }
        std::this_thread::yield();
    }
    ASSERT_EQ(secondAttempt.opaqueItems.size(), 1U);
}

} // namespace
} // namespace UVE::Render::Tests
