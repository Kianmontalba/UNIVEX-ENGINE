// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/render_queue_uve.h"

#include <filesystem>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include "uve/asset/asset_database_uve.h"
#include "uve/asset/asset_manager_uve.h"
#include "uve/asset/material_asset_uve.h"
#include "uve/asset/mesh_asset_uve.h"
#include "uve/events/event_system_uve.h"
#include "uve/threading/thread_pool_uve.h"

namespace UVE::Render::Tests {
namespace {

// RenderItemUVE holds live AssetHandleUVE<T> members with no default constructor, so a real
// AssetManagerUVE + registered loaders are the simplest way to obtain valid handles for
// SortUVE()'s test fixtures — the loaders never actually read a file since the handles are only
// used for their identity/lifetime here, not their loaded content.
class RenderQueueUVETest : public ::testing::Test {
protected:
    Threading::ThreadPoolUVE threadPool{2};
    Events::EventSystemUVE eventSystem;
    Asset::AssetDatabaseUVE assetDatabase;
    Asset::AssetManagerUVE assetManager{threadPool, eventSystem};

    RenderQueueUVETest() {
        assetManager.RegisterLoaderUVE<Asset::MeshAssetUVE>(
            [](const std::filesystem::path&, Asset::MeshAssetUVE&) { return true; });
        assetManager.RegisterLoaderUVE<Asset::MaterialAssetUVE>(
            [](const std::filesystem::path&, Asset::MaterialAssetUVE&) { return true; });
    }

    [[nodiscard]] RenderItemUVE MakeItemUVE(float sortDepth) {
        static int nextPathSuffix = 0;
        const Asset::AssetGuidUVE meshGuid =
            assetDatabase.RegisterUVE("render_queue_tests_mesh_" + std::to_string(nextPathSuffix++) + ".uvemodel");
        const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE(
            "render_queue_tests_material_" + std::to_string(nextPathSuffix++) + ".uvemat");
        Asset::AssetHandleUVE<Asset::MeshAssetUVE> meshHandle =
            assetManager.LoadUVE<Asset::MeshAssetUVE>(meshGuid, assetDatabase);
        Asset::AssetHandleUVE<Asset::MaterialAssetUVE> materialHandle =
            assetManager.LoadUVE<Asset::MaterialAssetUVE>(materialGuid, assetDatabase);
        return RenderItemUVE{Math::Matrix4x4UVE::IdentityUVE(), std::move(meshHandle), std::move(materialHandle),
                              sortDepth};
    }
};

TEST_F(RenderQueueUVETest, SortUVE_OpaqueItems_SortedFrontToBack) {
    RenderQueueUVE queue;
    queue.opaqueItems.push_back(MakeItemUVE(5.0F));
    queue.opaqueItems.push_back(MakeItemUVE(1.0F));
    queue.opaqueItems.push_back(MakeItemUVE(3.0F));

    queue.SortUVE();

    ASSERT_EQ(queue.opaqueItems.size(), 3U);
    EXPECT_FLOAT_EQ(queue.opaqueItems[0].sortDepth, 1.0F);
    EXPECT_FLOAT_EQ(queue.opaqueItems[1].sortDepth, 3.0F);
    EXPECT_FLOAT_EQ(queue.opaqueItems[2].sortDepth, 5.0F);
}

TEST_F(RenderQueueUVETest, SortUVE_TransparentItems_SortedBackToFront) {
    RenderQueueUVE queue;
    queue.transparentItems.push_back(MakeItemUVE(1.0F));
    queue.transparentItems.push_back(MakeItemUVE(5.0F));
    queue.transparentItems.push_back(MakeItemUVE(3.0F));

    queue.SortUVE();

    ASSERT_EQ(queue.transparentItems.size(), 3U);
    EXPECT_FLOAT_EQ(queue.transparentItems[0].sortDepth, 5.0F);
    EXPECT_FLOAT_EQ(queue.transparentItems[1].sortDepth, 3.0F);
    EXPECT_FLOAT_EQ(queue.transparentItems[2].sortDepth, 1.0F);
}

TEST_F(RenderQueueUVETest, SortUVE_EqualDepthItems_UseDeterministicAssetTieBreak) {
    const auto assetLess = [](const RenderItemUVE& lhs, const RenderItemUVE& rhs) {
        const auto lhsMaterial = lhs.materialHandle.GetGuidUVE().value;
        const auto rhsMaterial = rhs.materialHandle.GetGuidUVE().value;
        if (lhsMaterial != rhsMaterial) {
            return lhsMaterial < rhsMaterial;
        }
        return lhs.meshHandle.GetGuidUVE().value < rhs.meshHandle.GetGuidUVE().value;
    };

    RenderItemUVE opaqueFirst = MakeItemUVE(2.0F);
    RenderItemUVE opaqueSecond = MakeItemUVE(2.0F);
    ASSERT_NE(opaqueFirst.materialHandle.GetGuidUVE(), opaqueSecond.materialHandle.GetGuidUVE());
    RenderQueueUVE opaqueQueue;
    opaqueQueue.opaqueItems.push_back(std::move(opaqueSecond));
    opaqueQueue.opaqueItems.push_back(std::move(opaqueFirst));
    opaqueQueue.SortUVE();
    ASSERT_EQ(opaqueQueue.opaqueItems.size(), 2U);
    EXPECT_TRUE(assetLess(opaqueQueue.opaqueItems[0], opaqueQueue.opaqueItems[1]));
    EXPECT_FLOAT_EQ(opaqueQueue.opaqueItems[0].sortDepth, 2.0F);
    EXPECT_FLOAT_EQ(opaqueQueue.opaqueItems[1].sortDepth, 2.0F);

    RenderItemUVE transparentFirst = MakeItemUVE(3.0F);
    RenderItemUVE transparentSecond = MakeItemUVE(3.0F);
    ASSERT_NE(transparentFirst.materialHandle.GetGuidUVE(), transparentSecond.materialHandle.GetGuidUVE());
    RenderQueueUVE transparentQueue;
    transparentQueue.transparentItems.push_back(std::move(transparentSecond));
    transparentQueue.transparentItems.push_back(std::move(transparentFirst));
    transparentQueue.SortUVE();
    ASSERT_EQ(transparentQueue.transparentItems.size(), 2U);
    EXPECT_TRUE(assetLess(transparentQueue.transparentItems[0], transparentQueue.transparentItems[1]));
    EXPECT_FLOAT_EQ(transparentQueue.transparentItems[0].sortDepth, 3.0F);
    EXPECT_FLOAT_EQ(transparentQueue.transparentItems[1].sortDepth, 3.0F);
}

TEST_F(RenderQueueUVETest, ClearUVE_ClearsFrameStateAndPreservesCapacity) {
    RenderQueueUVE queue;
    queue.ReserveUVE(8U, 7U, 6U);
    queue.opaqueItems.push_back(MakeItemUVE(1.0F));
    queue.transparentItems.push_back(MakeItemUVE(2.0F));
    queue.particleItems.push_back({{11U, 2U}, Math::Vector3UVE{1.0F, 2.0F, 3.0F}, 1.0F, 4.0F, 1U});
    queue.particleItemsTruncated = true;
    queue.invalidAssetReferences = 1U;
    queue.pendingAssetLoads = 2U;
    queue.failedAssetLoads = 3U;
    queue.invalidRenderEligibility = 4U;

    const std::size_t opaqueCapacity = queue.opaqueItems.capacity();
    const std::size_t transparentCapacity = queue.transparentItems.capacity();
    const std::size_t particleCapacity = queue.particleItems.capacity();

    queue.ClearUVE();

    EXPECT_TRUE(queue.opaqueItems.empty());
    EXPECT_TRUE(queue.transparentItems.empty());
    EXPECT_TRUE(queue.particleItems.empty());
    EXPECT_FALSE(queue.particleItemsTruncated);
    EXPECT_EQ(queue.invalidAssetReferences, 0U);
    EXPECT_EQ(queue.pendingAssetLoads, 0U);
    EXPECT_EQ(queue.failedAssetLoads, 0U);
    EXPECT_EQ(queue.invalidRenderEligibility, 0U);
    EXPECT_EQ(queue.opaqueItems.capacity(), opaqueCapacity);
    EXPECT_EQ(queue.transparentItems.capacity(), transparentCapacity);
    EXPECT_EQ(queue.particleItems.capacity(), particleCapacity);
}

} // namespace
} // namespace UVE::Render::Tests
