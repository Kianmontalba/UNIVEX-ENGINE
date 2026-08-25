// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <filesystem>

#include <gtest/gtest.h>

#include "uve/asset/asset_database_uve.h"
#include "uve/asset/asset_content_fingerprint_uve.h"
#include "uve/debug/log_sink_uve.h"
#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/scene/components/mesh_component_uve.h"
#include "uve/scene/components/prefab_instance_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/entity_manager_uve.h"
#include "uve/scene/prefab_system_uve.h"
#include "uve/scene/scene_graph_uve.h"

namespace UVE::Scene::Tests {
namespace {

class PrefabMaturityUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    SceneGraphUVE sceneGraph;
    Asset::AssetDatabaseUVE assetDatabase;
    PrefabSystemUVE prefabSystem;
};

TEST_F(PrefabMaturityUVETest, SourceRevisionUVE_ChangesWhenPrefabEnvelopeChanges) {
    const EntityUVE source = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<MeshComponentUVE>(source,
                                                     MeshComponentUVE{Asset::AssetGuidUVE{11U}, Asset::AssetGuidUVE{12U}});
    const std::filesystem::path path = "uve_prefab_maturity_revision.uveprefab";
    std::filesystem::remove(path);

    ASSERT_NE(prefabSystem.SavePrefabUVE(entityManager, assetDatabase, source, path), Asset::kInvalidAssetGuidUVE);
    const std::optional<std::uint64_t> firstRevision = ComputePrefabSourceRevisionUVE(path);
    ASSERT_TRUE(firstRevision.has_value());

    entityManager.GetComponentUVE<MeshComponentUVE>(source).meshGuid = Asset::AssetGuidUVE{99U};
    ASSERT_NE(prefabSystem.SavePrefabUVE(entityManager, assetDatabase, source, path), Asset::kInvalidAssetGuidUVE);
    const std::optional<std::uint64_t> secondRevision = ComputePrefabSourceRevisionUVE(path);
    ASSERT_TRUE(secondRevision.has_value());
    EXPECT_NE(*firstRevision, *secondRevision);

    std::filesystem::remove(path);
}

TEST_F(PrefabMaturityUVETest, RefreshInstanceUVE_ReplacesCleanInstanceAfterSourceMutation) {
    const EntityUVE source = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<MeshComponentUVE>(source,
                                                     MeshComponentUVE{Asset::AssetGuidUVE{21U}, Asset::AssetGuidUVE{22U}});
    const std::filesystem::path path = "uve_prefab_maturity_refresh.uveprefab";
    std::filesystem::remove(path);
    const Asset::AssetGuidUVE guid = prefabSystem.SavePrefabUVE(entityManager, assetDatabase, source, path);
    ASSERT_NE(guid, Asset::kInvalidAssetGuidUVE);
    const EntityUVE instance = prefabSystem.InstantiateUVE(entityManager, sceneGraph, assetDatabase, guid,
                                                            kInvalidEntityUVE);
    ASSERT_NE(instance, kInvalidEntityUVE);

    entityManager.GetComponentUVE<MeshComponentUVE>(source).meshGuid = Asset::AssetGuidUVE{77U};
    ASSERT_NE(prefabSystem.SavePrefabUVE(entityManager, assetDatabase, source, path), Asset::kInvalidAssetGuidUVE);

    const PrefabRefreshResultUVE result =
        prefabSystem.RefreshInstanceUVE(entityManager, sceneGraph, assetDatabase, instance);
    ASSERT_EQ(result.code, PrefabRefreshCodeUVE::Refreshed);
    ASSERT_NE(result.rootEntity, kInvalidEntityUVE);
    EXPECT_NE(result.rootEntity, instance);
    EXPECT_EQ(entityManager.GetComponentUVE<MeshComponentUVE>(result.rootEntity).meshGuid,
              Asset::AssetGuidUVE{77U});
    EXPECT_FALSE(entityManager.IsAliveUVE(instance));

    std::filesystem::remove(path);
}

TEST_F(PrefabMaturityUVETest, RefreshInstanceUVE_RejectsLocalOverridesWithMergeRequired) {
    const EntityUVE source = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<MeshComponentUVE>(source,
                                                     MeshComponentUVE{Asset::AssetGuidUVE{31U}, Asset::AssetGuidUVE{32U}});
    const std::filesystem::path path = "uve_prefab_maturity_merge_required.uveprefab";
    std::filesystem::remove(path);
    const Asset::AssetGuidUVE guid = prefabSystem.SavePrefabUVE(entityManager, assetDatabase, source, path);
    ASSERT_NE(guid, Asset::kInvalidAssetGuidUVE);
    const EntityUVE instance = prefabSystem.InstantiateUVE(entityManager, sceneGraph, assetDatabase, guid,
                                                            kInvalidEntityUVE);
    ASSERT_NE(instance, kInvalidEntityUVE);

    PrefabInstanceComponentUVE& instanceComponent =
        entityManager.GetComponentUVE<PrefabInstanceComponentUVE>(instance);
    instanceComponent.overrides = {{"Mesh.meshGuid", "99"}};
    entityManager.GetComponentUVE<MeshComponentUVE>(source).meshGuid = Asset::AssetGuidUVE{88U};
    ASSERT_NE(prefabSystem.SavePrefabUVE(entityManager, assetDatabase, source, path), Asset::kInvalidAssetGuidUVE);

    const PrefabRefreshResultUVE result =
        prefabSystem.RefreshInstanceUVE(entityManager, sceneGraph, assetDatabase, instance);
    EXPECT_EQ(result.code, PrefabRefreshCodeUVE::MergeRequired);
    EXPECT_EQ(result.rootEntity, instance);
    EXPECT_TRUE(entityManager.IsAliveUVE(instance));

    std::filesystem::remove(path);
}

TEST_F(PrefabMaturityUVETest, RefreshInstanceUVE_ForceRefreshExplicitlyDiscardsOverrides) {
    const EntityUVE source = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<MeshComponentUVE>(source,
                                                     MeshComponentUVE{Asset::AssetGuidUVE{41U}, Asset::AssetGuidUVE{42U}});
    const std::filesystem::path path = "uve_prefab_maturity_force_refresh.uveprefab";
    std::filesystem::remove(path);
    const Asset::AssetGuidUVE guid = prefabSystem.SavePrefabUVE(entityManager, assetDatabase, source, path);
    ASSERT_NE(guid, Asset::kInvalidAssetGuidUVE);
    const EntityUVE instance = prefabSystem.InstantiateUVE(entityManager, sceneGraph, assetDatabase, guid,
                                                            kInvalidEntityUVE);
    ASSERT_NE(instance, kInvalidEntityUVE);

    entityManager.GetComponentUVE<PrefabInstanceComponentUVE>(instance).overrides = {{"Mesh.meshGuid", "99"}};
    entityManager.GetComponentUVE<MeshComponentUVE>(source).meshGuid = Asset::AssetGuidUVE{123U};
    ASSERT_NE(prefabSystem.SavePrefabUVE(entityManager, assetDatabase, source, path), Asset::kInvalidAssetGuidUVE);

    const PrefabRefreshResultUVE result =
        prefabSystem.RefreshInstanceUVE(entityManager, sceneGraph, assetDatabase, instance, true);
    ASSERT_EQ(result.code, PrefabRefreshCodeUVE::Refreshed);
    ASSERT_NE(result.rootEntity, instance);
    EXPECT_EQ(entityManager.GetComponentUVE<MeshComponentUVE>(result.rootEntity).meshGuid,
              Asset::AssetGuidUVE{123U});
    EXPECT_FALSE(entityManager.IsAliveUVE(instance));

    std::filesystem::remove(path);
}


} // namespace
} // namespace UVE::Scene::Tests
