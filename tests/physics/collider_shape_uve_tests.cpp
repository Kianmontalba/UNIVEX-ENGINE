// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/scene/components/collider_component_uve.h"

#include <algorithm>
#include <vector>

#include <gtest/gtest.h>

#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/physics/collision_system_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/entity_manager_uve.h"
#include "uve/scene/scene_graph_uve.h"

namespace UVE::Physics::Tests {
namespace {

class ColliderShapeUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    Scene::EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    Scene::SceneGraphUVE sceneGraph;
    CollisionSystemUVE collisionSystem;

    Scene::EntityUVE MakeColliderEntityUVE(Math::Vector3UVE position, const Scene::ColliderComponentUVE& collider) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE transform;
        transform.localPosition = position;
        sceneGraph.AttachTransformUVE(entityManager, entity, transform);
        sceneGraph.UpdateUVE(entityManager);
        entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(entity, collider);
        return entity;
    }
};

TEST(ColliderComponentUVETest, IsColliderComponentValidUVE_AcceptsBoxSphereAndCapsule) {
    Scene::ColliderComponentUVE box;
    EXPECT_TRUE(Scene::IsColliderComponentValidUVE(box));

    Scene::ColliderComponentUVE sphere;
    sphere.shapeType = Scene::ColliderShapeTypeUVE::Sphere;
    sphere.radius = 1.0F;
    EXPECT_TRUE(Scene::IsColliderComponentValidUVE(sphere));

    Scene::ColliderComponentUVE capsule;
    capsule.shapeType = Scene::ColliderShapeTypeUVE::Capsule;
    capsule.radius = 0.5F;
    capsule.height = 2.0F;
    EXPECT_TRUE(Scene::IsColliderComponentValidUVE(capsule));
}

TEST(ColliderComponentUVETest, IsColliderComponentValidUVE_RejectsInvalidShapeParameters) {
    Scene::ColliderComponentUVE invalid;
    invalid.shapeType = static_cast<Scene::ColliderShapeTypeUVE>(255U);
    EXPECT_FALSE(Scene::IsColliderComponentValidUVE(invalid));

    invalid = {};
    invalid.shapeType = Scene::ColliderShapeTypeUVE::Sphere;
    invalid.radius = 0.0F;
    EXPECT_FALSE(Scene::IsColliderComponentValidUVE(invalid));

    invalid = {};
    invalid.shapeType = Scene::ColliderShapeTypeUVE::Capsule;
    invalid.radius = 0.5F;
    invalid.height = 0.9F;
    EXPECT_FALSE(Scene::IsColliderComponentValidUVE(invalid));
}

TEST(ColliderComponentUVETest, GetColliderLocalHalfExtentsUVE_UsesConservativeShapeBounds) {
    Scene::ColliderComponentUVE sphere;
    sphere.shapeType = Scene::ColliderShapeTypeUVE::Sphere;
    sphere.radius = 1.25F;
    const Math::Vector3UVE expectedSphereExtents{1.25F, 1.25F, 1.25F};
    EXPECT_EQ(Scene::GetColliderLocalHalfExtentsUVE(sphere), expectedSphereExtents);

    Scene::ColliderComponentUVE capsule;
    capsule.shapeType = Scene::ColliderShapeTypeUVE::Capsule;
    capsule.radius = 0.4F;
    capsule.height = 2.4F;
    const Math::Vector3UVE expectedCapsuleExtents{0.4F, 1.2F, 0.4F};
    EXPECT_EQ(Scene::GetColliderLocalHalfExtentsUVE(capsule), expectedCapsuleExtents);
}

TEST_F(ColliderShapeUVETest, DetectCollisionsUVE_SphereAndCapsuleUseExpandedAabbBounds) {
    Scene::ColliderComponentUVE sphere;
    sphere.shapeType = Scene::ColliderShapeTypeUVE::Sphere;
    sphere.radius = 1.0F;
    const Scene::EntityUVE sphereEntity = MakeColliderEntityUVE({0.0F, 0.0F, 0.0F}, sphere);
    const Scene::EntityUVE sphereTarget = MakeColliderEntityUVE(
        {1.4F, 0.0F, 0.0F}, Scene::ColliderComponentUVE{Math::Vector3UVE{0.5F, 0.5F, 0.5F}});

    const std::vector<CollisionPairUVE> spherePairs = collisionSystem.DetectCollisionsUVE(entityManager);
    ASSERT_EQ(spherePairs.size(), 1U);
    EXPECT_TRUE((spherePairs[0].first == sphereEntity && spherePairs[0].second == sphereTarget) ||
                (spherePairs[0].first == sphereTarget && spherePairs[0].second == sphereEntity));

    const Scene::EntityUVE capsuleEntity = entityManager.CreateEntityUVE();
    Scene::TransformComponentUVE capsuleTransform;
    capsuleTransform.localPosition = {0.0F, 0.0F, 4.0F};
    sceneGraph.AttachTransformUVE(entityManager, capsuleEntity, capsuleTransform);
    sceneGraph.UpdateUVE(entityManager);
    Scene::ColliderComponentUVE capsule;
    capsule.shapeType = Scene::ColliderShapeTypeUVE::Capsule;
    capsule.radius = 0.5F;
    capsule.height = 3.0F;
    entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(capsuleEntity, capsule);
    const Scene::EntityUVE capsuleTarget = MakeColliderEntityUVE(
        {0.0F, 1.8F, 4.0F}, Scene::ColliderComponentUVE{Math::Vector3UVE{0.5F, 0.5F, 0.5F}});

    const std::vector<CollisionPairUVE> allPairs = collisionSystem.DetectCollisionsUVE(entityManager);
    EXPECT_EQ(allPairs.size(), 2U);
    EXPECT_TRUE(std::any_of(allPairs.begin(), allPairs.end(), [&](const CollisionPairUVE& pair) {
        return (pair.first == capsuleEntity && pair.second == capsuleTarget) ||
               (pair.first == capsuleTarget && pair.second == capsuleEntity);
    }));
}

} // namespace
} // namespace UVE::Physics::Tests
