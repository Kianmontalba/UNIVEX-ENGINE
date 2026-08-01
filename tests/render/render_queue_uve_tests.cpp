//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

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

} // namespace
} // namespace UVE::Render::Tests
