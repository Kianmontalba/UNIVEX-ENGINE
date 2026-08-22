// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/physics/area_overlap_system_uve.h"

#include <cstddef>
#include <cstdint>
#include <limits>

#include <gtest/gtest.h>

#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/scene/components/area_component_uve.h"
#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/entity_manager_uve.h"
#include "uve/scene/scene_graph_uve.h"

namespace UVE::Physics::Tests {
namespace {

class AreaOverlapSystemUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    Scene::EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    Scene::SceneGraphUVE sceneGraph;

    Scene::EntityUVE MakeAreaEntityUVE(Math::Vector3UVE position, Math::Vector3UVE halfExtents,
                                       std::uint32_t collisionLayer = 1U,
                                       std::uint32_t collisionMask = 0xFFFFFFFFU) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE transform;
        transform.localPosition = position;
        sceneGraph.AttachTransformUVE(entityManager, entity, transform);
        sceneGraph.UpdateUVE(entityManager);
        entityManager.AddComponentUVE<Scene::AreaComponentUVE>(
            entity, Scene::AreaComponentUVE{halfExtents, collisionLayer, collisionMask});
        return entity;
    }

    Scene::EntityUVE MakeColliderEntityUVE(Math::Vector3UVE position, Math::Vector3UVE halfExtents,
                                           std::uint32_t collisionLayer = 1U,
                                           std::uint32_t collisionMask = 0xFFFFFFFFU) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE transform;
        transform.localPosition = position;
        sceneGraph.AttachTransformUVE(entityManager, entity, transform);
        sceneGraph.UpdateUVE(entityManager);
        entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(
            entity, Scene::ColliderComponentUVE{halfExtents, collisionLayer, collisionMask});
        return entity;
    }
};

TEST_F(AreaOverlapSystemUVETest, QueryUVE_CompatibleOverlapReturnsCopiedPair) {
    const Scene::EntityUVE area = MakeAreaEntityUVE(
        Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 1U, 2U);
    const Scene::EntityUVE collider = MakeColliderEntityUVE(
        Math::Vector3UVE{0.5F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 2U, 1U);

    const AreaOverlapQueryResultUVE result = AreaOverlapSystemUVE::QueryUVE(entityManager);

    ASSERT_EQ(result.inspectedAreas, 1U);
    ASSERT_EQ(result.inspectedColliders, 1U);
    ASSERT_FALSE(result.truncated);
    ASSERT_EQ(result.overlaps.size(), 1U);
    EXPECT_EQ(result.overlaps.front().area, area);
    EXPECT_EQ(result.overlaps.front().other, collider);
    EXPECT_GT(result.overlaps.front().penetrationDepth, 0.0F);
}

TEST_F(AreaOverlapSystemUVETest, QueryUVE_SkipsFiniteAreaBoundsThatOverflowPublication) {
    const Scene::EntityUVE validArea =
        MakeAreaEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    MakeAreaEntityUVE(Math::Vector3UVE{std::numeric_limits<float>::max(), 0.0F, 0.0F},
                      Math::Vector3UVE{1.0e38F, 1.0F, 1.0F});
    const Scene::EntityUVE collider =
        MakeColliderEntityUVE(Math::Vector3UVE{0.5F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});

    const AreaOverlapQueryResultUVE result = AreaOverlapSystemUVE::QueryUVE(entityManager);

    ASSERT_EQ(result.inspectedAreas, 1U);
    ASSERT_EQ(result.inspectedColliders, 1U);
    ASSERT_EQ(result.overlaps.size(), 1U);
    EXPECT_EQ(result.overlaps.front().area, validArea);
    EXPECT_EQ(result.overlaps.front().other, collider);
    EXPECT_FALSE(result.truncated);
}

TEST_F(AreaOverlapSystemUVETest, QueryUVE_SkipsOverflowedFinitePenetrationDepth) {
    const float maximumFloat = std::numeric_limits<float>::max();
    MakeAreaEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F},
                      Math::Vector3UVE{maximumFloat, maximumFloat, maximumFloat});
    MakeColliderEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F},
                          Math::Vector3UVE{maximumFloat, maximumFloat, maximumFloat});

    const AreaOverlapQueryResultUVE result = AreaOverlapSystemUVE::QueryUVE(entityManager);

    EXPECT_EQ(result.inspectedAreas, 1U);
    EXPECT_EQ(result.inspectedColliders, 1U);
    EXPECT_TRUE(result.overlaps.empty());
    EXPECT_FALSE(result.truncated);
}

TEST_F(AreaOverlapSystemUVETest, QueryUVE_IncompatibleOrOneSidedMaskDoesNotReportOverlap) {
    MakeAreaEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 1U, 1U);
    MakeColliderEntityUVE(Math::Vector3UVE{0.5F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 2U, 1U);

    const AreaOverlapQueryResultUVE result = AreaOverlapSystemUVE::QueryUVE(entityManager);

    EXPECT_TRUE(result.overlaps.empty());
    EXPECT_FALSE(result.truncated);
}

TEST_F(AreaOverlapSystemUVETest, QueryUVE_HardCapReportsTruncationAndStableFirstPair) {
    const Scene::EntityUVE area = MakeAreaEntityUVE(
        Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{2.0F, 2.0F, 2.0F});
    const Scene::EntityUVE first = MakeColliderEntityUVE(
        Math::Vector3UVE{0.5F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    MakeColliderEntityUVE(Math::Vector3UVE{-0.5F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});

    const AreaOverlapQueryResultUVE result = AreaOverlapSystemUVE::QueryUVE(entityManager, 1U);

    ASSERT_EQ(result.overlaps.size(), 1U);
    EXPECT_TRUE(result.truncated);
    EXPECT_EQ(result.overlaps.front().area, area);
    EXPECT_EQ(result.overlaps.front().other, first);
}

TEST_F(AreaOverlapSystemUVETest, QueryUVE_AreaCacheCapReportsTruncationAndInspectsBoundedPrefix) {
    for (std::size_t index = 0U; index < kMaximumAreaOverlapQueryAreasUVE + 1U; ++index) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE transform;
        transform.localPosition = Math::Vector3UVE{static_cast<float>(index) * 4.0F, 0.0F, 0.0F};
        sceneGraph.AttachTransformUVE(entityManager, entity, transform);
        entityManager.AddComponentUVE<Scene::AreaComponentUVE>(
            entity, Scene::AreaComponentUVE{Math::Vector3UVE{0.25F, 0.25F, 0.25F}, 1U, 0xFFFFFFFFU});
    }
    sceneGraph.UpdateUVE(entityManager);

    const AreaOverlapQueryResultUVE result = AreaOverlapSystemUVE::QueryUVE(entityManager);

    EXPECT_EQ(result.inspectedAreas, kMaximumAreaOverlapQueryAreasUVE);
    EXPECT_TRUE(result.truncated);
    EXPECT_TRUE(result.overlaps.empty());
}

TEST_F(AreaOverlapSystemUVETest, QueryUVE_NoAreaEntitiesDoesNotTreatColliderPairsAsAreaOverlaps) {
    MakeColliderEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    MakeColliderEntityUVE(Math::Vector3UVE{0.5F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});

    const AreaOverlapQueryResultUVE result = AreaOverlapSystemUVE::QueryUVE(entityManager);

    EXPECT_EQ(result.inspectedAreas, 0U);
    EXPECT_EQ(result.inspectedColliders, 2U);
    EXPECT_TRUE(result.overlaps.empty());
}

} // namespace
} // namespace UVE::Physics::Tests
