// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/physics/collision_system_uve.h"
#include "uve/physics/detail/collider_world_aabb_cache_uve.h"

#include <algorithm>
#include <limits>

#include <gtest/gtest.h>

#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/entity_manager_uve.h"
#include "uve/scene/scene_graph_uve.h"

namespace UVE::Physics::Tests {
namespace {

class CollisionSystemUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    Scene::EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    Scene::SceneGraphUVE sceneGraph;
    CollisionSystemUVE collisionSystem;

    Scene::EntityUVE MakeColliderEntityUVE(Math::Vector3UVE position, Math::Vector3UVE halfExtents,
                                           std::uint32_t collisionLayer = 1U,
                                           std::uint32_t collisionMask = 0xFFFFFFFFU) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE local;
        local.localPosition = position;
        sceneGraph.AttachTransformUVE(entityManager, entity, local);
        sceneGraph.UpdateUVE(entityManager);
        Scene::ColliderComponentUVE collider{halfExtents};
        collider.collisionLayer = collisionLayer;
        collider.collisionMask = collisionMask;
        entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(entity, collider);
        return entity;
    }

    [[nodiscard]] static bool ContainsPairUVE(const std::vector<CollisionPairUVE>& pairs, Scene::EntityUVE a,
                                               Scene::EntityUVE b) {
        return std::any_of(pairs.begin(), pairs.end(), [a, b](const CollisionPairUVE& pair) {
            return (pair.first == a && pair.second == b) || (pair.first == b && pair.second == a);
        });
    }
};

TEST_F(CollisionSystemUVETest, DetectCollisionsUVE_OverlappingEntities_ReturnsOnePair) {
    const Scene::EntityUVE a = MakeColliderEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    const Scene::EntityUVE b = MakeColliderEntityUVE(Math::Vector3UVE{1.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});

    const std::vector<CollisionPairUVE> pairs = collisionSystem.DetectCollisionsUVE(entityManager);

    ASSERT_EQ(pairs.size(), 1U);
    EXPECT_TRUE(ContainsPairUVE(pairs, a, b));
}

TEST_F(CollisionSystemUVETest, DetectCollisionsUVE_DisjointEntities_ReturnsNoPairs) {
    MakeColliderEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{0.5F, 0.5F, 0.5F});
    MakeColliderEntityUVE(Math::Vector3UVE{100.0F, 0.0F, 0.0F}, Math::Vector3UVE{0.5F, 0.5F, 0.5F});

    EXPECT_TRUE(collisionSystem.DetectCollisionsUVE(entityManager).empty());
}

TEST_F(CollisionSystemUVETest, DetectCollisionsUVE_EntityMissingColliderComponent_IsExcluded) {
    // A plain scene-graph entity with a transform but no ColliderComponentUVE.
    const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
    sceneGraph.AttachTransformUVE(entityManager, entity, Scene::TransformComponentUVE{});
    sceneGraph.UpdateUVE(entityManager);
    MakeColliderEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});

    EXPECT_TRUE(collisionSystem.DetectCollisionsUVE(entityManager).empty());
}

TEST_F(CollisionSystemUVETest, BuildColliderWorldAabbCacheUVE_SkipsFiniteBoundsThatOverflowPublication) {
    const Scene::EntityUVE validEntity =
        MakeColliderEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    const Scene::EntityUVE overflowedEntity = MakeColliderEntityUVE(
        Math::Vector3UVE{std::numeric_limits<float>::max(), 0.0F, 0.0F},
        Math::Vector3UVE{1.0e38F, 1.0F, 1.0F});

    const std::vector<Detail::ColliderWorldAabbUVE> cache = Detail::BuildColliderWorldAabbCacheUVE(entityManager);

    ASSERT_EQ(cache.size(), 1U);
    EXPECT_EQ(cache.front().entity, validEntity);
    EXPECT_NE(cache.front().entity, overflowedEntity);
    EXPECT_TRUE(collisionSystem.DetectCollisionsUVE(entityManager).empty());
}

TEST_F(CollisionSystemUVETest, BuildColliderWorldAabbCacheUVE_CapsCopiedProxiesAtBvhLimit) {
    for (std::size_t index = 0U; index < Detail::DynamicAabbBvhUVE::kMaximumProxiesUVE + 1U; ++index) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE local;
        local.localPosition = Math::Vector3UVE{static_cast<float>(index) * 3.0F, 0.0F, 0.0F};
        sceneGraph.AttachTransformUVE(entityManager, entity, local);
        entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(
            entity, Scene::ColliderComponentUVE{Math::Vector3UVE{0.5F, 0.5F, 0.5F}});
    }
    sceneGraph.UpdateUVE(entityManager);

    const std::vector<Detail::ColliderWorldAabbUVE> cache = Detail::BuildColliderWorldAabbCacheUVE(entityManager);
    ASSERT_EQ(cache.size(), Detail::DynamicAabbBvhUVE::kMaximumProxiesUVE);
    EXPECT_TRUE(Detail::DynamicAabbBvhUVE(cache).IsValidUVE());
}

TEST_F(CollisionSystemUVETest, DetectCollisionsUVE_MultipleSimultaneousOverlaps_AllReported) {
    const Scene::EntityUVE a = MakeColliderEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    const Scene::EntityUVE b = MakeColliderEntityUVE(Math::Vector3UVE{1.5F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    const Scene::EntityUVE c = MakeColliderEntityUVE(Math::Vector3UVE{3.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});

    const std::vector<CollisionPairUVE> pairs = collisionSystem.DetectCollisionsUVE(entityManager);

    ASSERT_EQ(pairs.size(), 2U);
    EXPECT_TRUE(ContainsPairUVE(pairs, a, b));
    EXPECT_TRUE(ContainsPairUVE(pairs, b, c));
    EXPECT_FALSE(ContainsPairUVE(pairs, a, c));
}

TEST_F(CollisionSystemUVETest, DetectCollisionsUVE_CompatibleLayerMasks_ReportOverlap) {
    const Scene::EntityUVE a = MakeColliderEntityUVE(
        Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 1U, 2U);
    const Scene::EntityUVE b = MakeColliderEntityUVE(
        Math::Vector3UVE{0.5F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 2U, 1U);

    const std::vector<CollisionPairUVE> pairs = collisionSystem.DetectCollisionsUVE(entityManager);

    ASSERT_EQ(pairs.size(), 1U);
    EXPECT_TRUE(ContainsPairUVE(pairs, a, b));
}

TEST_F(CollisionSystemUVETest, DetectCollisionsUVE_IncompatibleLayerMask_RejectsOverlap) {
    const Scene::EntityUVE a = MakeColliderEntityUVE(
        Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 1U, 1U);
    const Scene::EntityUVE b = MakeColliderEntityUVE(
        Math::Vector3UVE{0.5F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 2U, 1U);

    EXPECT_TRUE(collisionSystem.DetectCollisionsUVE(entityManager).empty());
    EXPECT_EQ(entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(a).collisionMask, 1U);
    EXPECT_EQ(entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(b).collisionMask, 1U);
}

TEST_F(CollisionSystemUVETest, DetectCollisionsUVE_OneSidedMaskAcceptance_StillRejectsOverlap) {
    const Scene::EntityUVE a = MakeColliderEntityUVE(
        Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 1U, 2U);
    const Scene::EntityUVE b = MakeColliderEntityUVE(
        Math::Vector3UVE{0.5F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 2U, 4U);

    EXPECT_TRUE(collisionSystem.DetectCollisionsUVE(entityManager).empty());
    EXPECT_TRUE(entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(a).collisionMask & 2U);
    EXPECT_FALSE(entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(b).collisionMask & 1U);
}

TEST_F(CollisionSystemUVETest, DetectCollisionsUVE_BvhPreservesLegacyPairOrderAcrossSpatialClusters) {
    const Scene::EntityUVE a = MakeColliderEntityUVE({0.0F, 0.0F, 0.0F}, {0.75F, 0.75F, 0.75F});
    const Scene::EntityUVE b = MakeColliderEntityUVE({10.0F, 0.0F, 0.0F}, {0.75F, 0.75F, 0.75F});
    const Scene::EntityUVE c = MakeColliderEntityUVE({0.5F, 0.0F, 0.0F}, {0.75F, 0.75F, 0.75F});
    const Scene::EntityUVE d = MakeColliderEntityUVE({10.5F, 0.0F, 0.0F}, {0.75F, 0.75F, 0.75F});
    const Scene::EntityUVE e = MakeColliderEntityUVE({1.0F, 0.0F, 0.0F}, {0.75F, 0.75F, 0.75F});
    MakeColliderEntityUVE({100.0F, 0.0F, 0.0F}, {0.75F, 0.75F, 0.75F});

    const std::vector<CollisionPairUVE> pairs = collisionSystem.DetectCollisionsUVE(entityManager);

    ASSERT_EQ(pairs.size(), 4U);
    EXPECT_EQ(pairs[0].first, a);
    EXPECT_EQ(pairs[0].second, c);
    EXPECT_EQ(pairs[1].first, a);
    EXPECT_EQ(pairs[1].second, e);
    EXPECT_EQ(pairs[2].first, b);
    EXPECT_EQ(pairs[2].second, d);
    EXPECT_EQ(pairs[3].first, c);
    EXPECT_EQ(pairs[3].second, e);
}

TEST_F(CollisionSystemUVETest, DetectCollisionsUVE_StaticVsStaticOverlap_IsStillReported) {
    // Both entities have ColliderComponentUVE but neither has RigidBodyComponentUVE — pure
    // static world geometry. Detection doesn't care; resolution (PhysicsSystemUVE) does.
    const Scene::EntityUVE a = MakeColliderEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    const Scene::EntityUVE b = MakeColliderEntityUVE(Math::Vector3UVE{0.5F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});

    const std::vector<CollisionPairUVE> pairs = collisionSystem.DetectCollisionsUVE(entityManager);

    ASSERT_EQ(pairs.size(), 1U);
    EXPECT_TRUE(ContainsPairUVE(pairs, a, b));
}

TEST(DynamicAabbBvhUVETest, UpdateAabbUVE_RefitsMovedProxyAndPreservesCacheIndexOrder) {
    const Scene::EntityUVE firstEntity{1U, 1U};
    const Scene::EntityUVE secondEntity{2U, 1U};
    Detail::DynamicAabbBvhUVE bvh(std::vector<Detail::ColliderWorldAabbUVE>{
        {firstEntity, Math::AabbUVE::FromCenterExtentsUVE({0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}), 1U, 0xFFFFFFFFU},
        {secondEntity, Math::AabbUVE::FromCenterExtentsUVE({100.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}), 1U, 0xFFFFFFFFU},
    });
    ASSERT_TRUE(bvh.IsValidUVE());

    const Math::AabbUVE farBounds =
        Math::AabbUVE::FromCenterExtentsUVE({1000.0F, 0.0F, 0.0F}, {2.0F, 2.0F, 2.0F});
    std::vector<std::size_t> candidates;
    ASSERT_TRUE(bvh.QueryUVE(farBounds, candidates));
    EXPECT_TRUE(candidates.empty());

    const Math::AabbUVE movedAabb =
        Math::AabbUVE::FromCenterExtentsUVE({1000.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F});
    const auto update = bvh.UpdateAabbUVE(firstEntity, movedAabb);
    ASSERT_TRUE(update.IsAppliedUVE());
    ASSERT_TRUE(bvh.QueryUVE(farBounds, candidates));
    ASSERT_FALSE(candidates.empty());
    EXPECT_TRUE(std::is_sorted(candidates.begin(), candidates.end()));
    EXPECT_NE(std::find(candidates.begin(), candidates.end(), 0U), candidates.end());
    EXPECT_EQ(bvh.GetCollidersUVE()[0].entity, firstEntity);
    EXPECT_FLOAT_EQ(bvh.GetCollidersUVE()[0].worldAabb.min.x, 999.0F);

    ASSERT_TRUE(bvh.UpdateAabbUVE(
                    firstEntity, Math::AabbUVE::FromCenterExtentsUVE({0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}))
                    .IsAppliedUVE());
    ASSERT_TRUE(bvh.QueryUVE(farBounds, candidates));
    EXPECT_TRUE(candidates.empty());
}

TEST(DynamicAabbBvhUVETest, UpdateAabbUVE_RejectsInvalidAndStaleProxyIdentities) {
    const Scene::EntityUVE entity{7U, 4U};
    Detail::DynamicAabbBvhUVE bvh(std::vector<Detail::ColliderWorldAabbUVE>{
        {entity, Math::AabbUVE::FromCenterExtentsUVE({0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}), 1U, 0xFFFFFFFFU},
    });
    const Math::AabbUVE validAabb = Math::AabbUVE::FromCenterExtentsUVE({1.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F});

    EXPECT_EQ(bvh.UpdateAabbUVE(Scene::kInvalidEntityUVE, validAabb).code,
              Detail::DynamicColliderWorldAabbUpdateCodeUVE::InvalidEntity);
    EXPECT_EQ(bvh.UpdateAabbUVE({7U, 3U}, validAabb).code,
              Detail::DynamicColliderWorldAabbUpdateCodeUVE::StaleGeneration);
    EXPECT_EQ(bvh.UpdateAabbUVE({8U, 1U}, validAabb).code,
              Detail::DynamicColliderWorldAabbUpdateCodeUVE::UnknownProxy);
    const Math::AabbUVE invalidAabb{
        {std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}};
    EXPECT_EQ(bvh.UpdateAabbUVE(entity, invalidAabb).code,
              Detail::DynamicColliderWorldAabbUpdateCodeUVE::InvalidAabb);
}

} // namespace
} // namespace UVE::Physics::Tests
