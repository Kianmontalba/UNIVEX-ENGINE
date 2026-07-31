//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/scene/prefab_system_uve.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/asset_database_uve.h"
#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logger_uve.h"
#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/scene/components/hierarchy_component_uve.h"
#include "uve/scene/components/mesh_component_uve.h"
#include "uve/scene/components/prefab_instance_component_uve.h"
#include "uve/scene/components/rigid_body_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/entity_manager_uve.h"
#include "uve/scene/scene_graph_uve.h"

namespace UVE::Scene::Tests {
namespace {

class PrefabSystemUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    SceneGraphUVE sceneGraph;
    Asset::AssetDatabaseUVE assetDatabase;
    PrefabSystemUVE prefabSystem;
};

TEST_F(PrefabSystemUVETest, SaveThenInstantiate_ProducesEntityWithSameComponentValues) {
    const EntityUVE source = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<MeshComponentUVE>(source, MeshComponentUVE{"trees/oak.uvemodel"});

    const std::filesystem::path prefabPath = "uve_prefab_tests_oak.uveprefab";
    std::filesystem::remove(prefabPath);
    const Asset::AssetGuidUVE guid = prefabSystem.SavePrefabUVE(entityManager, assetDatabase, source, prefabPath);
    ASSERT_NE(guid, Asset::kInvalidAssetGuidUVE);

    const EntityUVE instance =
        prefabSystem.InstantiateUVE(entityManager, sceneGraph, assetDatabase, guid, kInvalidEntityUVE);
    ASSERT_NE(instance, kInvalidEntityUVE);
    ASSERT_NE(instance, source);

    EXPECT_EQ(entityManager.GetComponentUVE<MeshComponentUVE>(instance).meshAssetPath, "trees/oak.uvemodel");
    ASSERT_TRUE(entityManager.HasComponentUVE<PrefabInstanceComponentUVE>(instance));
    EXPECT_EQ(entityManager.GetComponentUVE<PrefabInstanceComponentUVE>(instance).sourcePrefabGuid, guid);

    std::filesystem::remove(prefabPath);
}

TEST_F(PrefabSystemUVETest, InstantiateTwice_ProducesIndependentEntities) {
    const EntityUVE source = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<RigidBodyComponentUVE>(source, RigidBodyComponentUVE{1.0F, false});

    const std::filesystem::path prefabPath = "uve_prefab_tests_independent.uveprefab";
    std::filesystem::remove(prefabPath);
    const Asset::AssetGuidUVE guid = prefabSystem.SavePrefabUVE(entityManager, assetDatabase, source, prefabPath);

    const EntityUVE instanceA =
        prefabSystem.InstantiateUVE(entityManager, sceneGraph, assetDatabase, guid, kInvalidEntityUVE);
    const EntityUVE instanceB =
        prefabSystem.InstantiateUVE(entityManager, sceneGraph, assetDatabase, guid, kInvalidEntityUVE);
    ASSERT_NE(instanceA, instanceB);

    entityManager.GetComponentUVE<RigidBodyComponentUVE>(instanceA).mass = 99.0F;
    EXPECT_FLOAT_EQ(entityManager.GetComponentUVE<RigidBodyComponentUVE>(instanceB).mass, 1.0F);

    std::filesystem::remove(prefabPath);
}

TEST_F(PrefabSystemUVETest, InstantiateUVE_WithParent_ReparentsNewRoot) {
    const EntityUVE parent = entityManager.CreateEntityUVE();
    sceneGraph.AttachTransformUVE(entityManager, parent, TransformComponentUVE{});

    const EntityUVE source = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<MeshComponentUVE>(source, MeshComponentUVE{"props/box.uvemodel"});
    sceneGraph.AttachTransformUVE(entityManager, source, TransformComponentUVE{});

    const std::filesystem::path prefabPath = "uve_prefab_tests_reparent.uveprefab";
    std::filesystem::remove(prefabPath);
    const Asset::AssetGuidUVE guid = prefabSystem.SavePrefabUVE(entityManager, assetDatabase, source, prefabPath);

    const EntityUVE instance = prefabSystem.InstantiateUVE(entityManager, sceneGraph, assetDatabase, guid, parent);
    ASSERT_NE(instance, kInvalidEntityUVE);
    EXPECT_EQ(entityManager.GetComponentUVE<HierarchyComponentUVE>(instance).parent, parent);

    std::filesystem::remove(prefabPath);
}

TEST_F(PrefabSystemUVETest, InstantiateUVE_UnknownGuid_ReturnsInvalidAndLogsError) {
    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    const EntityUVE instance = prefabSystem.InstantiateUVE(entityManager, sceneGraph, assetDatabase,
                                                            Asset::AssetGuidUVE{424242}, kInvalidEntityUVE);
    EXPECT_EQ(instance, kInvalidEntityUVE);

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("unknown prefab GUID") != std::string::npos;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
}

TEST_F(PrefabSystemUVETest, NestedPrefab_PreservesSourceGuidWithoutRecursiveReinstantiation) {
    const EntityUVE innerSource = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<MeshComponentUVE>(innerSource, MeshComponentUVE{"props/lamp.uvemodel"});
    sceneGraph.AttachTransformUVE(entityManager, innerSource, TransformComponentUVE{});
    const std::filesystem::path innerPath = "uve_prefab_tests_inner.uveprefab";
    std::filesystem::remove(innerPath);
    const Asset::AssetGuidUVE innerGuid =
        prefabSystem.SavePrefabUVE(entityManager, assetDatabase, innerSource, innerPath);

    const EntityUVE outerRoot = entityManager.CreateEntityUVE();
    sceneGraph.AttachTransformUVE(entityManager, outerRoot, TransformComponentUVE{});
    const EntityUVE innerInstance =
        prefabSystem.InstantiateUVE(entityManager, sceneGraph, assetDatabase, innerGuid, outerRoot);
    ASSERT_NE(innerInstance, kInvalidEntityUVE);

    const std::filesystem::path outerPath = "uve_prefab_tests_outer.uveprefab";
    std::filesystem::remove(outerPath);
    const Asset::AssetGuidUVE outerGuid =
        prefabSystem.SavePrefabUVE(entityManager, assetDatabase, outerRoot, outerPath);

    EntityManagerUVE freshManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    SceneGraphUVE freshSceneGraph;
    const EntityUVE outerInstance =
        prefabSystem.InstantiateUVE(freshManager, freshSceneGraph, assetDatabase, outerGuid, kInvalidEntityUVE);
    ASSERT_NE(outerInstance, kInvalidEntityUVE);

    EntityUVE nestedChild = kInvalidEntityUVE;
    freshManager.ForEachUVE<HierarchyComponentUVE>(
        [&nestedChild, outerInstance](EntityUVE entity, HierarchyComponentUVE& hierarchy) {
            if (hierarchy.parent == outerInstance) {
                nestedChild = entity;
            }
        });
    ASSERT_NE(nestedChild, kInvalidEntityUVE);
    ASSERT_TRUE(freshManager.HasComponentUVE<MeshComponentUVE>(nestedChild));
    EXPECT_EQ(freshManager.GetComponentUVE<MeshComponentUVE>(nestedChild).meshAssetPath, "props/lamp.uvemodel");
    ASSERT_TRUE(freshManager.HasComponentUVE<PrefabInstanceComponentUVE>(nestedChild));
    EXPECT_EQ(freshManager.GetComponentUVE<PrefabInstanceComponentUVE>(nestedChild).sourcePrefabGuid, innerGuid);

    std::filesystem::remove(innerPath);
    std::filesystem::remove(outerPath);
}

TEST_F(PrefabSystemUVETest, SavePrefabUVE_SamePathTwice_KeepsGuidStable) {
    const EntityUVE source = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<MeshComponentUVE>(source, MeshComponentUVE{"props/crate.uvemodel"});

    const std::filesystem::path path = "uve_prefab_tests_stable_guid.uveprefab";
    std::filesystem::remove(path);

    const Asset::AssetGuidUVE firstGuid = prefabSystem.SavePrefabUVE(entityManager, assetDatabase, source, path);
    const Asset::AssetGuidUVE secondGuid = prefabSystem.SavePrefabUVE(entityManager, assetDatabase, source, path);
    EXPECT_EQ(firstGuid, secondGuid);

    std::filesystem::remove(path);
}

} // namespace
} // namespace UVE::Scene::Tests
