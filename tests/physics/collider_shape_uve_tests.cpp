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

TEST_F(ColliderShapeUVETest, DetectCollisionsUVE_SphereUsesExactSphereBoxAndCapsuleRemainsConservative) {
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

TEST_F(ColliderShapeUVETest, DetectCollisionsUVE_SphereSphereUsesExactDistance) {
    Scene::ColliderComponentUVE firstSphere;
    firstSphere.shapeType = Scene::ColliderShapeTypeUVE::Sphere;
    firstSphere.radius = 1.0F;
    MakeColliderEntityUVE({0.0F, 0.0F, 0.0F}, firstSphere);

    Scene::ColliderComponentUVE secondSphere;
    secondSphere.shapeType = Scene::ColliderShapeTypeUVE::Sphere;
    secondSphere.radius = 1.0F;
    MakeColliderEntityUVE({1.5F, 0.0F, 0.0F}, secondSphere);

    const std::vector<CollisionPairUVE> pairs = collisionSystem.DetectCollisionsUVE(entityManager);
    ASSERT_EQ(pairs.size(), 1U);
    EXPECT_NEAR(pairs[0].penetrationDepth, 0.5F, 1.0e-5F);
    EXPECT_NEAR(pairs[0].separationAxis.x, 1.0F, 1.0e-5F);
    EXPECT_NEAR(pairs[0].separationAxis.y, 0.0F, 1.0e-5F);
    EXPECT_NEAR(pairs[0].separationAxis.z, 0.0F, 1.0e-5F);
}

TEST_F(ColliderShapeUVETest, DetectCollisionsUVE_SphereSphereRejectsTouchingAndDiagonalAabbFalsePositive) {
    Scene::ColliderComponentUVE sphere;
    sphere.shapeType = Scene::ColliderShapeTypeUVE::Sphere;
    sphere.radius = 0.5F;
    MakeColliderEntityUVE({0.0F, 0.0F, 0.0F}, sphere);

    Scene::ColliderComponentUVE touchingSphere;
    touchingSphere.shapeType = Scene::ColliderShapeTypeUVE::Sphere;
    touchingSphere.radius = 0.5F;
    MakeColliderEntityUVE({10.0F, 0.0F, 0.0F}, touchingSphere);

    Scene::ColliderComponentUVE diagonalSphere;
    diagonalSphere.shapeType = Scene::ColliderShapeTypeUVE::Sphere;
    diagonalSphere.radius = 0.5F;
    MakeColliderEntityUVE({0.9F, 0.9F, 0.0F}, diagonalSphere);

    // The diagonal pair's conservative AABBs overlap, but its center distance is greater than
    // the combined radius. The touching pair is isolated at x=10 and is also intentionally
    // excluded.
    EXPECT_TRUE(collisionSystem.DetectCollisionsUVE(entityManager).empty());
}

TEST_F(ColliderShapeUVETest, DetectCollisionsUVE_CapsuleRejectsDiagonalExpandedAabbFalsePositive) {
    Scene::ColliderComponentUVE capsule;
    capsule.shapeType = Scene::ColliderShapeTypeUVE::Capsule;
    capsule.radius = 0.5F;
    capsule.height = 4.0F;
    MakeColliderEntityUVE({0.0F, 0.0F, 0.0F}, capsule);

    Scene::ColliderComponentUVE box;
    box.halfExtents = {0.5F, 0.5F, 0.5F};
    MakeColliderEntityUVE({0.9F, 0.0F, 0.9F}, box);

    // The conservative AABBs overlap by 0.1 on X/Z, but the capsule centerline-to-box distance
    // is sqrt(0.4^2 + 0.4^2) > radius, so the exact narrow phase must reject the pair.
    EXPECT_TRUE(collisionSystem.DetectCollisionsUVE(entityManager).empty());
}

TEST_F(ColliderShapeUVETest, DetectCollisionsUVE_SphereRejectsDiagonalExpandedAabbFalsePositive) {
    Scene::ColliderComponentUVE sphere;
    sphere.shapeType = Scene::ColliderShapeTypeUVE::Sphere;
    sphere.radius = 1.0F;
    MakeColliderEntityUVE({0.0F, 0.0F, 0.0F}, sphere);

    Scene::ColliderComponentUVE box;
    box.halfExtents = {0.5F, 0.5F, 0.5F};
    MakeColliderEntityUVE({1.4F, 1.4F, 0.0F}, box);

    // The conservative AABBs overlap by 0.1 on X/Y, but the sphere-to-box distance is
    // sqrt(0.9^2 + 0.9^2) > radius, so the exact narrow phase must reject the pair.
    EXPECT_TRUE(collisionSystem.DetectCollisionsUVE(entityManager).empty());
}

} // namespace
} // namespace UVE::Physics::Tests
