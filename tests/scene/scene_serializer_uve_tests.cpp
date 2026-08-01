//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/scene/scene_serializer_uve.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/asset_guid_uve.h"
#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logger_uve.h"
#include "uve/events/event_system_uve.h"
#include "uve/math/vector3_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/hierarchy_component_uve.h"
#include "uve/scene/components/light_component_uve.h"
#include "uve/scene/components/mesh_component_uve.h"
#include "uve/scene/components/rigid_body_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"
#include "uve/scene/entity_manager_uve.h"
#include "uve/scene/scene_graph_uve.h"

namespace UVE::Scene::Tests {
namespace {

class SceneSerializerUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    SceneSerializerUVE serializer;
};

TEST_F(SceneSerializerUVETest, SaveThenLoad_SingleEntityWithMultipleComponents_RoundTripsExactly) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<MeshComponentUVE>(
        entity, MeshComponentUVE{Asset::AssetGuidUVE{111}, Asset::AssetGuidUVE{222}});
    entityManager.AddComponentUVE<LightComponentUVE>(
        entity, LightComponentUVE{Math::Vector3UVE{0.2F, 0.4F, 0.6F}, 2.5F});
    entityManager.AddComponentUVE<RigidBodyComponentUVE>(entity, RigidBodyComponentUVE{5.0F, true});

    const std::filesystem::path path = "uve_scene_serializer_tests_single.uvescene";
    std::filesystem::remove(path);
    ASSERT_TRUE(serializer.SaveUVE(entityManager, {entity}, path, SceneAssetTypeUVE::Scene));

    EntityManagerUVE loadedManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    const std::vector<EntityUVE> roots = serializer.LoadUVE(loadedManager, path);
    ASSERT_EQ(roots.size(), 1U);
    const EntityUVE loaded = roots[0];

    EXPECT_EQ(loadedManager.GetComponentUVE<MeshComponentUVE>(loaded).meshGuid, Asset::AssetGuidUVE{111});
    EXPECT_EQ(loadedManager.GetComponentUVE<MeshComponentUVE>(loaded).materialGuid, Asset::AssetGuidUVE{222});
    EXPECT_FLOAT_EQ(loadedManager.GetComponentUVE<LightComponentUVE>(loaded).intensity, 2.5F);
    const Math::Vector3UVE expectedColor{0.2F, 0.4F, 0.6F};
    EXPECT_TRUE(loadedManager.GetComponentUVE<LightComponentUVE>(loaded).color == expectedColor);
    EXPECT_FLOAT_EQ(loadedManager.GetComponentUVE<RigidBodyComponentUVE>(loaded).mass, 5.0F);
    EXPECT_TRUE(loadedManager.GetComponentUVE<RigidBodyComponentUVE>(loaded).isKinematic);

    std::filesystem::remove(path);
}

TEST_F(SceneSerializerUVETest, SaveThenLoad_ColliderComponentUVE_RoundTripsFrictionRestitutionDensity) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    ColliderComponentUVE collider{Math::Vector3UVE{0.5F, 0.5F, 0.5F}};
    collider.collisionLayer = 2;
    collider.collisionMask = 0x0000FFFFU;
    collider.friction = 0.4F;
    collider.restitution = 0.9F;
    collider.density = 2.5F;
    entityManager.AddComponentUVE<ColliderComponentUVE>(entity, collider);

    const std::filesystem::path path = "uve_scene_serializer_tests_collider.uvescene";
    std::filesystem::remove(path);
    ASSERT_TRUE(serializer.SaveUVE(entityManager, {entity}, path, SceneAssetTypeUVE::Scene));

    EntityManagerUVE loadedManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    const std::vector<EntityUVE> roots = serializer.LoadUVE(loadedManager, path);
    ASSERT_EQ(roots.size(), 1U);
    const ColliderComponentUVE& loaded = loadedManager.GetComponentUVE<ColliderComponentUVE>(roots[0]);

    EXPECT_EQ(loaded.collisionLayer, 2U);
    EXPECT_EQ(loaded.collisionMask, 0x0000FFFFU);
    EXPECT_FLOAT_EQ(loaded.friction, 0.4F);
    EXPECT_FLOAT_EQ(loaded.restitution, 0.9F);
    EXPECT_FLOAT_EQ(loaded.density, 2.5F);

    std::filesystem::remove(path);
}

TEST_F(SceneSerializerUVETest, SaveThenLoad_Hierarchy_RemapsParentCorrectly) {
    const EntityUVE parent = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<HierarchyComponentUVE>(parent, HierarchyComponentUVE{kInvalidEntityUVE});
    const EntityUVE child = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<HierarchyComponentUVE>(child, HierarchyComponentUVE{parent});

    const std::filesystem::path path = "uve_scene_serializer_tests_hierarchy.uvescene";
    std::filesystem::remove(path);
    ASSERT_TRUE(serializer.SaveUVE(entityManager, {parent}, path, SceneAssetTypeUVE::Scene));

    EntityManagerUVE loadedManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    const std::vector<EntityUVE> roots = serializer.LoadUVE(loadedManager, path);
    ASSERT_EQ(roots.size(), 1U);
    const EntityUVE loadedParent = roots[0];

    EntityUVE loadedChild = kInvalidEntityUVE;
    loadedManager.ForEachUVE<HierarchyComponentUVE>(
        [&loadedChild, loadedParent](EntityUVE entity, HierarchyComponentUVE& hierarchy) {
            if (hierarchy.parent == loadedParent) {
                loadedChild = entity;
            }
        });
    EXPECT_NE(loadedChild, kInvalidEntityUVE);

    std::filesystem::remove(path);
}

TEST_F(SceneSerializerUVETest, SaveThenLoad_MultipleRoots_AllPresentInFileOrder) {
    const EntityUVE rootA = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<MeshComponentUVE>(rootA, MeshComponentUVE{Asset::AssetGuidUVE{1}, Asset::AssetGuidUVE{2}});
    const EntityUVE rootB = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<MeshComponentUVE>(rootB, MeshComponentUVE{Asset::AssetGuidUVE{3}, Asset::AssetGuidUVE{4}});

    const std::filesystem::path path = "uve_scene_serializer_tests_multi_root.uvescene";
    std::filesystem::remove(path);
    ASSERT_TRUE(serializer.SaveUVE(entityManager, {rootA, rootB}, path, SceneAssetTypeUVE::Scene));

    EntityManagerUVE loadedManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    const std::vector<EntityUVE> roots = serializer.LoadUVE(loadedManager, path);
    ASSERT_EQ(roots.size(), 2U);
    EXPECT_EQ(loadedManager.GetComponentUVE<MeshComponentUVE>(roots[0]).meshGuid, Asset::AssetGuidUVE{1});
    EXPECT_EQ(loadedManager.GetComponentUVE<MeshComponentUVE>(roots[1]).meshGuid, Asset::AssetGuidUVE{3});

    std::filesystem::remove(path);
}

TEST_F(SceneSerializerUVETest, SaveUVE_NeverSerializesWorldTransformComponent_AndSceneGraphRecomputesAfterLoad) {
    SceneGraphUVE sceneGraph;
    const EntityUVE entity = entityManager.CreateEntityUVE();
    TransformComponentUVE local;
    local.localPosition = Math::Vector3UVE{1.0F, 2.0F, 3.0F};
    sceneGraph.AttachTransformUVE(entityManager, entity, local);
    sceneGraph.UpdateUVE(entityManager);

    const std::filesystem::path path = "uve_scene_serializer_tests_no_world_transform.uvescene";
    std::filesystem::remove(path);
    ASSERT_TRUE(serializer.SaveUVE(entityManager, {entity}, path, SceneAssetTypeUVE::Scene));

    std::ifstream rawFile(path, std::ios::binary);
    const std::string contents((std::istreambuf_iterator<char>(rawFile)), std::istreambuf_iterator<char>());
    EXPECT_EQ(contents.find("WorldTransformComponentUVE"), std::string::npos);

    EntityManagerUVE loadedManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    const std::vector<EntityUVE> roots = serializer.LoadUVE(loadedManager, path);
    ASSERT_EQ(roots.size(), 1U);
    sceneGraph.UpdateUVE(loadedManager);

    const WorldTransformComponentUVE& world =
        loadedManager.GetComponentUVE<WorldTransformComponentUVE>(roots[0]);
    EXPECT_TRUE(world.worldPosition == local.localPosition);

    std::filesystem::remove(path);
}

TEST_F(SceneSerializerUVETest, LoadUVE_MissingFile_ReturnsEmptyVector) {
    const std::vector<EntityUVE> roots =
        serializer.LoadUVE(entityManager, "uve_scene_serializer_tests_nonexistent.uvescene");
    EXPECT_TRUE(roots.empty());
}

TEST_F(SceneSerializerUVETest, LoadUVE_BadMagic_ReturnsEmptyAndLogsError) {
    const std::filesystem::path path = "uve_scene_serializer_tests_bad_magic.uvescene";
    {
        std::ofstream file(path, std::ios::binary);
        file << "NOT A VALID UVE FILE AT ALL";
    }

    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    const std::vector<EntityUVE> roots = serializer.LoadUVE(entityManager, path);
    EXPECT_TRUE(roots.empty());

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("bad magic") != std::string::npos;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
    std::filesystem::remove(path);
}

} // namespace
} // namespace UVE::Scene::Tests
