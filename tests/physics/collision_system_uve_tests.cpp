// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/physics/collision_system_uve.h"

#include <algorithm>

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

    Scene::EntityUVE MakeColliderEntityUVE(Math::Vector3UVE position, Math::Vector3UVE halfExtents) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE local;
        local.localPosition = position;
        sceneGraph.AttachTransformUVE(entityManager, entity, local);
        sceneGraph.UpdateUVE(entityManager);
        entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(entity, Scene::ColliderComponentUVE{halfExtents});
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

TEST_F(CollisionSystemUVETest, DetectCollisionsUVE_StaticVsStaticOverlap_IsStillReported) {
    // Both entities have ColliderComponentUVE but neither has RigidBodyComponentUVE — pure
    // static world geometry. Detection doesn't care; resolution (PhysicsSystemUVE) does.
    const Scene::EntityUVE a = MakeColliderEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    const Scene::EntityUVE b = MakeColliderEntityUVE(Math::Vector3UVE{0.5F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});

    const std::vector<CollisionPairUVE> pairs = collisionSystem.DetectCollisionsUVE(entityManager);

    ASSERT_EQ(pairs.size(), 1U);
    EXPECT_TRUE(ContainsPairUVE(pairs, a, b));
}

} // namespace
} // namespace UVE::Physics::Tests
