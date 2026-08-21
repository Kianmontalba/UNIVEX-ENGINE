// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/scene/prefab_system_uve.h"

#include <algorithm>
#include <filesystem>
#include <map>
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

class FakePrefabOverrideTargetUVE final : public IPrefabOverrideTargetUVE {
public:
    std::map<std::string, std::string> values;
    std::string failingWritePath;
    std::size_t readCount = 0U;
    std::size_t writeCount = 0U;

    [[nodiscard]] bool ReadPropertyUVE(const std::string_view propertyPath,
                                       std::string& serializedValue) const override {
        ++const_cast<FakePrefabOverrideTargetUVE*>(this)->readCount;
        const auto iterator = values.find(std::string{propertyPath});
        if (iterator == values.end()) {
            return false;
        }
        serializedValue = iterator->second;
        return true;
    }

    [[nodiscard]] bool WritePropertyUVE(const std::string_view propertyPath,
                                        const std::string_view serializedValue) override {
        ++writeCount;
        if (!failingWritePath.empty() && propertyPath == failingWritePath) {
            return false;
        }
        const auto iterator = values.find(std::string{propertyPath});
        if (iterator == values.end()) {
            return false;
        }
        iterator->second = serializedValue;
        return true;
    }
};

class PrefabSystemUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    SceneGraphUVE sceneGraph;
    Asset::AssetDatabaseUVE assetDatabase;
    PrefabSystemUVE prefabSystem;
};

TEST(PrefabInstanceComponentUVE, ApplyAndRevertPrefabOverridesUVE_RestoresExplicitBaseline) {
    PrefabInstanceComponentUVE instance{
        Asset::AssetGuidUVE{7U},
        {{"Transform.position", "[1,2,3]"}, {"Transform.rotation", "[0,0,0,1]"}}};
    const std::vector<PrefabPropertyOverrideUVE> baseline{
        {"Transform.position", "[0,0,0]"}, {"Transform.rotation", "[0,0,0,1]"}};
    FakePrefabOverrideTargetUVE target;
    target.values = {{"Transform.position", "[0,0,0]"}, {"Transform.rotation", "[0,0,0,1]"}};

    const PrefabOverrideOperationResultUVE applied = ApplyPrefabOverridesUVE(instance, target);
    ASSERT_TRUE(applied.IsAppliedUVE());
    EXPECT_EQ(applied.affectedCount, 2U);
    EXPECT_EQ(target.values.at("Transform.position"), "[1,2,3]");
    EXPECT_EQ(instance.overrides.size(), 2U);

    const PrefabOverrideOperationResultUVE reverted = RevertPrefabOverridesUVE(instance, baseline, target);
    ASSERT_TRUE(reverted.IsAppliedUVE());
    EXPECT_EQ(reverted.affectedCount, 2U);
    EXPECT_EQ(target.values.at("Transform.position"), "[0,0,0]");
    EXPECT_EQ(target.values.at("Transform.rotation"), "[0,0,0,1]");
    EXPECT_TRUE(instance.overrides.empty());
}

TEST(PrefabInstanceComponentUVE, CommitPrefabOverridesToSourceUVE_BakesAndClearsAfterCleanBaseline) {
    PrefabInstanceComponentUVE instance{Asset::AssetGuidUVE{13U}, {{"A.value", "new-a"}}};
    const std::vector<PrefabPropertyOverrideUVE> baseline{{"A.value", "old-a"}};
    FakePrefabOverrideTargetUVE source;
    source.values = {{"A.value", "old-a"}};

    const PrefabOverrideOperationResultUVE result =
        CommitPrefabOverridesToSourceUVE(instance, baseline, source);

    ASSERT_TRUE(result.IsAppliedUVE());
    EXPECT_EQ(result.affectedCount, 1U);
    EXPECT_EQ(source.values.at("A.value"), "new-a");
    EXPECT_TRUE(instance.overrides.empty());
}

TEST(PrefabInstanceComponentUVE, CommitPrefabOverridesToSourceUVE_PublishesCommittedRevisionAfterSuccess) {
    PrefabInstanceComponentUVE instance{Asset::AssetGuidUVE{16U}, { {"A.value", "new-a"} }, 4U, 4U};
    const std::vector<PrefabPropertyOverrideUVE> baseline{{"A.value", "old-a"}};
    FakePrefabOverrideTargetUVE source;
    source.values = {{"A.value", "old-a"}};

    const PrefabOverrideOperationResultUVE result =
        CommitPrefabOverridesToSourceUVE(instance, baseline, source, 5U);

    ASSERT_TRUE(result.IsAppliedUVE());
    EXPECT_EQ(instance.sourceRevision, 5U);
    EXPECT_EQ(instance.instanceRevision, 5U);
    EXPECT_TRUE(instance.overrides.empty());
}

TEST(PrefabInstanceComponentUVE, CommitPrefabOverridesToSourceUVE_RejectsZeroCommittedRevisionAtomically) {
    PrefabInstanceComponentUVE instance{Asset::AssetGuidUVE{17U}, {{"A.value", "new-a"}}, 4U, 4U};
    const std::vector<PrefabPropertyOverrideUVE> baseline{{"A.value", "old-a"}};
    FakePrefabOverrideTargetUVE source;
    source.values = {{"A.value", "old-a"}};

    const PrefabOverrideOperationResultUVE result =
        CommitPrefabOverridesToSourceUVE(instance, baseline, source, 0U);

    EXPECT_EQ(result.code, PrefabOverrideOperationCodeUVE::InvalidInstance);
    EXPECT_EQ(instance.sourceRevision, 4U);
    EXPECT_EQ(instance.instanceRevision, 4U);
    ASSERT_EQ(instance.overrides.size(), 1U);
    EXPECT_EQ(source.writeCount, 0U);
}

TEST(PrefabInstanceComponentUVE, CommitPrefabOverridesToSourceUVE_RejectsStaleBaselineWithoutWrite) {
    PrefabInstanceComponentUVE instance{Asset::AssetGuidUVE{14U}, {{"A.value", "new-a"}}};
    const std::vector<PrefabPropertyOverrideUVE> baseline{{"A.value", "old-a"}};
    FakePrefabOverrideTargetUVE source;
    source.values = {{"A.value", "external-a"}};

    const PrefabOverrideOperationResultUVE result =
        CommitPrefabOverridesToSourceUVE(instance, baseline, source);

    EXPECT_EQ(result.code, PrefabOverrideOperationCodeUVE::ConflictDetected);
    EXPECT_EQ(source.writeCount, 0U);
    EXPECT_EQ(source.values.at("A.value"), "external-a");
    ASSERT_EQ(instance.overrides.size(), 1U);
}

TEST(PrefabInstanceComponentUVE, CommitPrefabOverridesToSourceUVE_PreservesInstanceOnWriteFailure) {
    PrefabInstanceComponentUVE instance{Asset::AssetGuidUVE{15U}, {{"A.value", "new-a"}}};
    const std::vector<PrefabPropertyOverrideUVE> baseline{{"A.value", "old-a"}};
    FakePrefabOverrideTargetUVE source;
    source.values = {{"A.value", "old-a"}};
    source.failingWritePath = "A.value";

    const PrefabOverrideOperationResultUVE result =
        CommitPrefabOverridesToSourceUVE(instance, baseline, source);

    EXPECT_EQ(result.code, PrefabOverrideOperationCodeUVE::WriteFailed);
    EXPECT_EQ(source.values.at("A.value"), "old-a");
    ASSERT_EQ(instance.overrides.size(), 1U);
    EXPECT_EQ(instance.overrides.front().serializedValue, "new-a");
}

TEST(PrefabInstanceComponentUVE, ApplyPrefabOverridesUVE_RollsBackEarlierWritesOnFailure) {
    const PrefabInstanceComponentUVE instance{
        Asset::AssetGuidUVE{8U},
        {{"A.value", "new-a"}, {"B.value", "new-b"}}};
    FakePrefabOverrideTargetUVE target;
    target.values = {{"A.value", "old-a"}, {"B.value", "old-b"}};
    target.failingWritePath = "B.value";

    const PrefabOverrideOperationResultUVE result = ApplyPrefabOverridesUVE(instance, target);
    EXPECT_EQ(result.code, PrefabOverrideOperationCodeUVE::WriteFailed);
    EXPECT_EQ(target.values.at("A.value"), "old-a");
    EXPECT_EQ(target.values.at("B.value"), "old-b");
    EXPECT_EQ(instance.overrides.size(), 2U);
}

TEST(PrefabInstanceComponentUVE, DetectPrefabOverrideConflictsUVE_CleanBaselineIsReadOnly) {
    const PrefabInstanceComponentUVE instance{
        Asset::AssetGuidUVE{10U}, {{"A.value", "new-a"}, {"B.value", "new-b"}}};
    const std::vector<PrefabPropertyOverrideUVE> baseline{{"A.value", "old-a"}, {"B.value", "old-b"}};
    FakePrefabOverrideTargetUVE target;
    target.values = {{"A.value", "old-a"}, {"B.value", "old-b"}};

    const PrefabOverrideConflictReportUVE report = DetectPrefabOverrideConflictsUVE(instance, baseline, target);
    EXPECT_TRUE(report.IsConflictFreeUVE());
    EXPECT_EQ(report.inspectedCount, 2U);
    EXPECT_TRUE(report.conflicts.empty());
    EXPECT_FALSE(report.truncated);
    EXPECT_EQ(target.writeCount, 0U);
}

TEST(PrefabInstanceComponentUVE, DetectPrefabOverrideConflictsUVE_ReportsExternalMutationWithoutWriting) {
    const PrefabInstanceComponentUVE instance{Asset::AssetGuidUVE{11U}, {{"A.value", "new-a"}}};
    const std::vector<PrefabPropertyOverrideUVE> baseline{{"A.value", "old-a"}};
    FakePrefabOverrideTargetUVE target;
    target.values = {{"A.value", "external-a"}};

    const PrefabOverrideConflictReportUVE report = DetectPrefabOverrideConflictsUVE(instance, baseline, target);
    ASSERT_EQ(report.code, PrefabOverrideOperationCodeUVE::ConflictDetected);
    ASSERT_EQ(report.conflicts.size(), 1U);
    EXPECT_EQ(report.conflicts.front().propertyPath, "A.value");
    EXPECT_EQ(report.conflicts.front().expectedSerializedValue, "old-a");
    EXPECT_EQ(report.conflicts.front().actualSerializedValue, "external-a");
    EXPECT_EQ(target.writeCount, 0U);
    EXPECT_EQ(target.values.at("A.value"), "external-a");
}

TEST(PrefabInstanceComponentUVE, DetectPrefabOverrideConflictsUVE_HonorsConflictHardCap) {
    const PrefabInstanceComponentUVE instance{
        Asset::AssetGuidUVE{12U}, {{"A.value", "new-a"}, {"B.value", "new-b"}}};
    const std::vector<PrefabPropertyOverrideUVE> baseline{{"A.value", "old-a"}, {"B.value", "old-b"}};
    FakePrefabOverrideTargetUVE target;
    target.values = {{"A.value", "external-a"}, {"B.value", "external-b"}};

    const PrefabOverrideConflictReportUVE report = DetectPrefabOverrideConflictsUVE(instance, baseline, target, 1U);
    EXPECT_EQ(report.code, PrefabOverrideOperationCodeUVE::ConflictDetected);
    EXPECT_EQ(report.inspectedCount, 2U);
    EXPECT_EQ(report.conflicts.size(), 1U);
    EXPECT_TRUE(report.truncated);
    EXPECT_EQ(target.writeCount, 0U);
}

TEST(PrefabInstanceComponentUVE, RevertPrefabOverridesUVE_RejectsInvalidBaselineWithoutMutation) {
    PrefabInstanceComponentUVE instance{Asset::AssetGuidUVE{9U}, {{"A.value", "new-a"}}};
    FakePrefabOverrideTargetUVE target;
    target.values = {{"A.value", "current-a"}};
    const std::vector<PrefabPropertyOverrideUVE> invalidBaseline{{"B.value", "one"}, {"A.value", "two"}};

    const PrefabOverrideOperationResultUVE result = RevertPrefabOverridesUVE(instance, invalidBaseline, target);
    EXPECT_EQ(result.code, PrefabOverrideOperationCodeUVE::InvalidBaseline);
    EXPECT_EQ(target.values.at("A.value"), "current-a");
    ASSERT_EQ(instance.overrides.size(), 1U);
    EXPECT_EQ(instance.overrides.front().serializedValue, "new-a");
}

TEST(PrefabInstanceComponentUVE, IsPrefabInstanceComponentValidUVE_RequiresSourceGuidAndSortedOverrides) {
    EXPECT_FALSE(IsPrefabInstanceComponentValidUVE(
        PrefabInstanceComponentUVE{Asset::kInvalidAssetGuidUVE, {}}));
    EXPECT_TRUE(IsPrefabInstanceComponentValidUVE(PrefabInstanceComponentUVE{Asset::AssetGuidUVE{7U}, {}}));
    EXPECT_TRUE(IsPrefabInstanceComponentValidUVE(PrefabInstanceComponentUVE{
        Asset::AssetGuidUVE{7U}, {{"Transform.position", "[1,2,3]"}, {"Transform.rotation", "[0,0,0,1]"}}}));
    EXPECT_FALSE(IsPrefabInstanceComponentValidUVE(PrefabInstanceComponentUVE{
        Asset::AssetGuidUVE{7U}, {{"Transform.rotation", "[0,0,0,1]"}, {"Transform.position", "[1,2,3]"}}}));
    EXPECT_FALSE(IsPrefabInstanceComponentValidUVE(
        PrefabInstanceComponentUVE{Asset::AssetGuidUVE{7U}, {{"Transform.position", ""}}}));
    EXPECT_FALSE(IsPrefabInstanceComponentValidUVE(PrefabInstanceComponentUVE{
        Asset::AssetGuidUVE{7U}, std::vector<PrefabPropertyOverrideUVE>(kMaximumPrefabOverridesUVE + 1U)}));
}

TEST_F(PrefabSystemUVETest, SaveThenInstantiate_ProducesEntityWithSameComponentValues) {
    const EntityUVE source = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<MeshComponentUVE>(source, MeshComponentUVE{Asset::AssetGuidUVE{11}, Asset::AssetGuidUVE{12}});

    const std::filesystem::path prefabPath = "uve_prefab_tests_oak.uveprefab";
    std::filesystem::remove(prefabPath);
    const Asset::AssetGuidUVE guid = prefabSystem.SavePrefabUVE(entityManager, assetDatabase, source, prefabPath);
    ASSERT_NE(guid, Asset::kInvalidAssetGuidUVE);

    const EntityUVE instance =
        prefabSystem.InstantiateUVE(entityManager, sceneGraph, assetDatabase, guid, kInvalidEntityUVE);
    ASSERT_NE(instance, kInvalidEntityUVE);
    ASSERT_NE(instance, source);

    EXPECT_EQ(entityManager.GetComponentUVE<MeshComponentUVE>(instance).meshGuid, Asset::AssetGuidUVE{11});
    ASSERT_TRUE(entityManager.HasComponentUVE<PrefabInstanceComponentUVE>(instance));
    EXPECT_EQ(entityManager.GetComponentUVE<PrefabInstanceComponentUVE>(instance).sourcePrefabGuid, guid);
    EXPECT_EQ(entityManager.GetComponentUVE<PrefabInstanceComponentUVE>(instance).sourceRevision, 1U);
    EXPECT_EQ(entityManager.GetComponentUVE<PrefabInstanceComponentUVE>(instance).instanceRevision, 1U);

    std::filesystem::remove(prefabPath);
}

TEST_F(PrefabSystemUVETest, InstantiateWithRevisionUVE_StampsRevisionAndRejectsZero) {
    const EntityUVE source = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<MeshComponentUVE>(source, MeshComponentUVE{Asset::AssetGuidUVE{13}, Asset::AssetGuidUVE{14}});
    const std::filesystem::path prefabPath = "uve_prefab_tests_revision.uveprefab";
    std::filesystem::remove(prefabPath);
    const Asset::AssetGuidUVE guid = prefabSystem.SavePrefabUVE(entityManager, assetDatabase, source, prefabPath);
    ASSERT_NE(guid, Asset::kInvalidAssetGuidUVE);

    const EntityUVE instance = prefabSystem.InstantiateWithRevisionUVE(
        entityManager, sceneGraph, assetDatabase, guid, kInvalidEntityUVE, 42U);
    ASSERT_NE(instance, kInvalidEntityUVE);
    const PrefabInstanceComponentUVE& component =
        entityManager.GetComponentUVE<PrefabInstanceComponentUVE>(instance);
    EXPECT_EQ(component.sourceRevision, 42U);
    EXPECT_EQ(component.instanceRevision, 42U);
    const std::size_t entityCountAfterValid = entityManager.GetEntityCountUVE();

    EXPECT_EQ(prefabSystem.InstantiateWithRevisionUVE(entityManager, sceneGraph, assetDatabase, guid,
                                                       kInvalidEntityUVE, 0U),
              kInvalidEntityUVE);
    EXPECT_EQ(entityManager.GetEntityCountUVE(), entityCountAfterValid);
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
    entityManager.AddComponentUVE<MeshComponentUVE>(source, MeshComponentUVE{Asset::AssetGuidUVE{21}, Asset::AssetGuidUVE{22}});
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
    entityManager.AddComponentUVE<MeshComponentUVE>(innerSource, MeshComponentUVE{Asset::AssetGuidUVE{31}, Asset::AssetGuidUVE{32}});
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
    EXPECT_EQ(freshManager.GetComponentUVE<MeshComponentUVE>(nestedChild).meshGuid, Asset::AssetGuidUVE{31});
    ASSERT_TRUE(freshManager.HasComponentUVE<PrefabInstanceComponentUVE>(nestedChild));
    EXPECT_EQ(freshManager.GetComponentUVE<PrefabInstanceComponentUVE>(nestedChild).sourcePrefabGuid, innerGuid);

    std::filesystem::remove(innerPath);
    std::filesystem::remove(outerPath);
}

TEST_F(PrefabSystemUVETest, SavePrefabUVE_SamePathTwice_KeepsGuidStable) {
    const EntityUVE source = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<MeshComponentUVE>(source, MeshComponentUVE{Asset::AssetGuidUVE{41}, Asset::AssetGuidUVE{42}});

    const std::filesystem::path path = "uve_prefab_tests_stable_guid.uveprefab";
    std::filesystem::remove(path);

    const Asset::AssetGuidUVE firstGuid = prefabSystem.SavePrefabUVE(entityManager, assetDatabase, source, path);
    const Asset::AssetGuidUVE secondGuid = prefabSystem.SavePrefabUVE(entityManager, assetDatabase, source, path);
    EXPECT_EQ(firstGuid, secondGuid);

    std::filesystem::remove(path);
}

} // namespace
} // namespace UVE::Scene::Tests
