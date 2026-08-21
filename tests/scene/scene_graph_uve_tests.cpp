// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/scene/scene_graph_uve.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/scene/components/hierarchy_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"
#include "uve/scene/entity_manager_uve.h"
#include "uve/platform/platform_uve.h"

namespace UVE::Scene::Tests {
namespace {

constexpr float kEpsilon = 1e-4F;

class SceneGraphUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    SceneGraphUVE sceneGraph;
};

TEST_F(SceneGraphUVETest, AttachTransformUVE_AddsAllThreeComponents) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    sceneGraph.AttachTransformUVE(entityManager, entity, TransformComponentUVE{});

    EXPECT_TRUE(entityManager.HasComponentUVE<TransformComponentUVE>(entity));
    EXPECT_TRUE(entityManager.HasComponentUVE<WorldTransformComponentUVE>(entity));
    EXPECT_TRUE(entityManager.HasComponentUVE<HierarchyComponentUVE>(entity));
    EXPECT_EQ(entityManager.GetComponentUVE<HierarchyComponentUVE>(entity).parent, kInvalidEntityUVE);
}

TEST(TransformComponentUVETest, IsTransformComponentValidUVE_RejectsNonFiniteAndNonUnitValues) {
    TransformComponentUVE transform;
    EXPECT_TRUE(IsTransformComponentValidUVE(transform));

    transform.localPosition.x = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(IsTransformComponentValidUVE(transform));
    transform = TransformComponentUVE{};
    transform.localScale.z = std::numeric_limits<float>::infinity();
    EXPECT_FALSE(IsTransformComponentValidUVE(transform));
    transform = TransformComponentUVE{};
    transform.localRotation.w = 0.5F;
    EXPECT_FALSE(IsTransformComponentValidUVE(transform));
    transform.localRotation = Math::QuaternionUVE{0.0F, 0.0F, 0.0F, 1.0004F};
    EXPECT_TRUE(IsTransformComponentValidUVE(transform));
}

TEST_F(SceneGraphUVETest, UpdateUVE_RootEntity_WorldEqualsLocal) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    TransformComponentUVE local;
    local.localPosition = Math::Vector3UVE{1.0F, 2.0F, 3.0F};
    sceneGraph.AttachTransformUVE(entityManager, entity, local);

    sceneGraph.UpdateUVE(entityManager);

    const WorldTransformComponentUVE& world = entityManager.GetComponentUVE<WorldTransformComponentUVE>(entity);
    EXPECT_FALSE(world.dirty);
    EXPECT_EQ(world.worldPosition, local.localPosition);
}

TEST_F(SceneGraphUVETest, SetLocalTransformUVE_MarksDirty) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    sceneGraph.AttachTransformUVE(entityManager, entity, TransformComponentUVE{});
    sceneGraph.UpdateUVE(entityManager);
    ASSERT_FALSE(entityManager.GetComponentUVE<WorldTransformComponentUVE>(entity).dirty);

    TransformComponentUVE newLocal;
    newLocal.localPosition = Math::Vector3UVE{5.0F, 0.0F, 0.0F};
    sceneGraph.SetLocalTransformUVE(entityManager, entity, newLocal);

    EXPECT_TRUE(entityManager.GetComponentUVE<WorldTransformComponentUVE>(entity).dirty);
}

TEST_F(SceneGraphUVETest, ParentChildComposition_CombinesPositionCorrectly) {
    const EntityUVE parent = entityManager.CreateEntityUVE();
    TransformComponentUVE parentLocal;
    parentLocal.localPosition = Math::Vector3UVE{10.0F, 0.0F, 0.0F};
    sceneGraph.AttachTransformUVE(entityManager, parent, parentLocal);

    const EntityUVE child = entityManager.CreateEntityUVE();
    TransformComponentUVE childLocal;
    childLocal.localPosition = Math::Vector3UVE{0.0F, 5.0F, 0.0F};
    sceneGraph.AttachTransformUVE(entityManager, child, childLocal);
    sceneGraph.SetParentUVE(entityManager, child, parent);

    sceneGraph.UpdateUVE(entityManager);

    const WorldTransformComponentUVE& childWorld = entityManager.GetComponentUVE<WorldTransformComponentUVE>(child);
    EXPECT_NEAR(childWorld.worldPosition.x, 10.0F, kEpsilon);
    EXPECT_NEAR(childWorld.worldPosition.y, 5.0F, kEpsilon);
    EXPECT_NEAR(childWorld.worldPosition.z, 0.0F, kEpsilon);
}

TEST_F(SceneGraphUVETest, MultiLevelHierarchy_PropagatesInOnePass) {
    const EntityUVE grandparent = entityManager.CreateEntityUVE();
    TransformComponentUVE grandparentLocal;
    grandparentLocal.localPosition = Math::Vector3UVE{1.0F, 0.0F, 0.0F};
    sceneGraph.AttachTransformUVE(entityManager, grandparent, grandparentLocal);

    const EntityUVE parent = entityManager.CreateEntityUVE();
    TransformComponentUVE parentLocal;
    parentLocal.localPosition = Math::Vector3UVE{1.0F, 0.0F, 0.0F};
    sceneGraph.AttachTransformUVE(entityManager, parent, parentLocal);
    sceneGraph.SetParentUVE(entityManager, parent, grandparent);

    const EntityUVE child = entityManager.CreateEntityUVE();
    TransformComponentUVE childLocal;
    childLocal.localPosition = Math::Vector3UVE{1.0F, 0.0F, 0.0F};
    sceneGraph.AttachTransformUVE(entityManager, child, childLocal);
    sceneGraph.SetParentUVE(entityManager, child, parent);

    sceneGraph.UpdateUVE(entityManager);

    const WorldTransformComponentUVE& childWorld = entityManager.GetComponentUVE<WorldTransformComponentUVE>(child);
    EXPECT_NEAR(childWorld.worldPosition.x, 3.0F, kEpsilon);
}

TEST_F(SceneGraphUVETest, UpdateUVE_PreservesFiniteCacheWhenHierarchyCompositionOverflows) {
    const float maximumFloat = std::numeric_limits<float>::max();
    const EntityUVE parent = entityManager.CreateEntityUVE();
    TransformComponentUVE parentLocal;
    parentLocal.localScale = Math::Vector3UVE{maximumFloat, 1.0F, 1.0F};
    sceneGraph.AttachTransformUVE(entityManager, parent, parentLocal);

    const EntityUVE child = entityManager.CreateEntityUVE();
    TransformComponentUVE childLocal;
    childLocal.localPosition = Math::Vector3UVE{2.0F, 0.0F, 0.0F};
    sceneGraph.AttachTransformUVE(entityManager, child, childLocal);
    sceneGraph.SetParentUVE(entityManager, child, parent);

    sceneGraph.UpdateUVE(entityManager);

    const WorldTransformComponentUVE& childWorld = entityManager.GetComponentUVE<WorldTransformComponentUVE>(child);
    EXPECT_TRUE(childWorld.dirty);
    EXPECT_TRUE(std::isfinite(childWorld.worldPosition.x));
    EXPECT_TRUE(std::isfinite(childWorld.worldPosition.y));
    EXPECT_TRUE(std::isfinite(childWorld.worldPosition.z));
    EXPECT_EQ(childWorld.worldPosition, (Math::Vector3UVE{}));
}

TEST_F(SceneGraphUVETest, MovingParent_RecomputesChildEvenWhenChildNotDirty) {
    const EntityUVE parent = entityManager.CreateEntityUVE();
    sceneGraph.AttachTransformUVE(entityManager, parent, TransformComponentUVE{});

    const EntityUVE child = entityManager.CreateEntityUVE();
    TransformComponentUVE childLocal;
    childLocal.localPosition = Math::Vector3UVE{1.0F, 0.0F, 0.0F};
    sceneGraph.AttachTransformUVE(entityManager, child, childLocal);
    sceneGraph.SetParentUVE(entityManager, child, parent);

    sceneGraph.UpdateUVE(entityManager);
    ASSERT_NEAR(entityManager.GetComponentUVE<WorldTransformComponentUVE>(child).worldPosition.x, 1.0F, kEpsilon);

    // Move only the parent; the child's own dirty flag is never touched by SetLocalTransformUVE
    // on a *different* entity, so it stays false right up until UpdateUVE() runs.
    TransformComponentUVE parentLocal;
    parentLocal.localPosition = Math::Vector3UVE{100.0F, 0.0F, 0.0F};
    sceneGraph.SetLocalTransformUVE(entityManager, parent, parentLocal);
    ASSERT_FALSE(entityManager.GetComponentUVE<WorldTransformComponentUVE>(child).dirty);

    sceneGraph.UpdateUVE(entityManager);

    EXPECT_NEAR(entityManager.GetComponentUVE<WorldTransformComponentUVE>(child).worldPosition.x, 101.0F, kEpsilon);
}

#if UVE_DEBUG
TEST_F(SceneGraphUVETest, SetParentUVEDeathTest_CycleRejected) {
    const EntityUVE a = entityManager.CreateEntityUVE();
    sceneGraph.AttachTransformUVE(entityManager, a, TransformComponentUVE{});
    const EntityUVE b = entityManager.CreateEntityUVE();
    sceneGraph.AttachTransformUVE(entityManager, b, TransformComponentUVE{});

    sceneGraph.SetParentUVE(entityManager, b, a); // b's parent is a
    EXPECT_DEATH({ sceneGraph.SetParentUVE(entityManager, a, b); }, "");
}
#endif

TEST_F(SceneGraphUVETest, GetChildrenUVE_ReturnsExactDirectChildren) {
    const EntityUVE parent = entityManager.CreateEntityUVE();
    sceneGraph.AttachTransformUVE(entityManager, parent, TransformComponentUVE{});

    const EntityUVE childA = entityManager.CreateEntityUVE();
    sceneGraph.AttachTransformUVE(entityManager, childA, TransformComponentUVE{});
    sceneGraph.SetParentUVE(entityManager, childA, parent);

    const EntityUVE childB = entityManager.CreateEntityUVE();
    sceneGraph.AttachTransformUVE(entityManager, childB, TransformComponentUVE{});
    sceneGraph.SetParentUVE(entityManager, childB, parent);

    const EntityUVE unrelated = entityManager.CreateEntityUVE();
    sceneGraph.AttachTransformUVE(entityManager, unrelated, TransformComponentUVE{});

    std::vector<EntityUVE> children = sceneGraph.GetChildrenUVE(entityManager, parent);
    std::sort(children.begin(), children.end(), [](const EntityUVE& lhs, const EntityUVE& rhs) {
        return lhs.index < rhs.index;
    });

    std::vector<EntityUVE> expected{childA, childB};
    std::sort(expected.begin(), expected.end(),
              [](const EntityUVE& lhs, const EntityUVE& rhs) { return lhs.index < rhs.index; });

    EXPECT_EQ(children, expected);
}

} // namespace
} // namespace UVE::Scene::Tests
