// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/scene/components/collider_component_uve.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/physics/collision_system_uve.h"
#include "uve/physics/detail/shape_narrow_phase_uve.h"
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

    Scene::EntityUVE MakeColliderEntityUVE(
        Math::Vector3UVE position, const Scene::ColliderComponentUVE& collider,
        Math::QuaternionUVE rotation = {}) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE transform;
        transform.localPosition = position;
        transform.localRotation = rotation;
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

TEST(ShapeNarrowPhaseUVETest, MovingSphereSphereSweepReportsExactHitAndDeterministicNormal) {
    const Math::RayUVE ray{{-5.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}};
    const std::optional<Math::RayHitUVE> hit =
        Detail::IntersectMovingSphereSphereUVE(ray, {0.0F, 1.4F, 0.0F}, 0.5F, 1.0F, 100.0F);
    ASSERT_TRUE(hit.has_value());
    EXPECT_NEAR(hit->distance, 5.0F - std::sqrt(0.29F), 1.0e-4F);
    EXPECT_LT(hit->normal.x, 0.0F);
    EXPECT_LT(hit->normal.y, 0.0F);
    EXPECT_NEAR(Math::LengthSquaredUVE(hit->normal), 1.0F, 1.0e-4F);

    const std::optional<Math::RayHitUVE> initialOverlap =
        Detail::IntersectMovingSphereSphereUVE(Math::RayUVE{{0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}},
                                               {0.0F, 0.0F, 0.0F}, 0.5F, 1.0F, 1.0F);
    ASSERT_TRUE(initialOverlap.has_value());
    EXPECT_FLOAT_EQ(initialOverlap->distance, 0.0F);
    EXPECT_EQ(initialOverlap->normal, (Math::Vector3UVE{}));

    EXPECT_FALSE(Detail::IntersectMovingSphereSphereUVE(
                     Math::RayUVE{{-5.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}}, {0.0F, 1.5F, 0.0F}, 0.5F, 1.0F,
                     100.0F)
                     .has_value());
    EXPECT_FALSE(Detail::IntersectMovingSphereSphereUVE(
                     ray, {0.0F, 1.4F, 0.0F}, 0.5F, 1.0F, 1.0F)
                     .has_value());
}

TEST(ShapeNarrowPhaseUVETest, RadiusSumOverflowFailsClosedBeforePenetrationPublication) {
    const float maximumRadius = std::numeric_limits<float>::max();
    EXPECT_FALSE(Detail::ComputeSphereSpherePenetrationUVE(
                     {}, maximumRadius, {}, maximumRadius)
                     .has_value());
    EXPECT_FALSE(Detail::ComputeCapsuleSpherePenetrationUVE(
                     {}, {}, maximumRadius, {}, maximumRadius)
                     .has_value());
    EXPECT_FALSE(Detail::ComputeCapsuleCapsulePenetrationUVE(
                     {}, {}, maximumRadius, {}, {}, maximumRadius)
                     .has_value());
}

TEST(ShapeNarrowPhaseUVETest, OrientedBoxOrientedBoxUsesFinitePrecisionForLargeCenterDelta) {
    const float maximumExtent = std::numeric_limits<float>::max();
    const float centerMagnitude = maximumExtent * 0.99F;
    const Math::QuaternionUVE identityRotation{};
    const std::optional<Math::PenetrationUVE> penetration =
        Detail::ComputeOrientedBoxOrientedBoxPenetrationUVE(
            {-centerMagnitude, 0.0F, 0.0F}, {maximumExtent, 1.0F, 1.0F}, identityRotation,
            {centerMagnitude, 0.0F, 0.0F}, {maximumExtent, 1.0F, 1.0F}, identityRotation);
    ASSERT_TRUE(penetration.has_value());
    EXPECT_TRUE(std::isfinite(penetration->depth));
    EXPECT_FLOAT_EQ(penetration->depth, 2.0F);
    EXPECT_FLOAT_EQ(penetration->axis.x, 0.0F);
    EXPECT_FLOAT_EQ(penetration->axis.y, 1.0F);
    EXPECT_FLOAT_EQ(penetration->axis.z, 0.0F);
}

TEST(ShapeNarrowPhaseUVETest, SphereOrientedBoxUsesFinitePrecisionForLargeCenterDelta) {
    const float maximumValue = std::numeric_limits<float>::max();
    const float centerMagnitude = maximumValue * 0.99F;
    const Math::QuaternionUVE identityRotation{};
    const std::optional<Math::PenetrationUVE> penetration =
        Detail::ComputeSphereOrientedBoxPenetrationUVE(
            {centerMagnitude, 0.0F, 0.0F}, {maximumValue, 1.0F, 1.0F}, identityRotation,
            {-centerMagnitude, 0.0F, 0.0F}, maximumValue);
    ASSERT_TRUE(penetration.has_value());
    EXPECT_TRUE(std::isfinite(penetration->depth));
    EXPECT_GT(penetration->depth, 0.0F);
    EXPECT_GT(penetration->axis.x, 0.0F);
    EXPECT_FLOAT_EQ(penetration->axis.y, 0.0F);
    EXPECT_FLOAT_EQ(penetration->axis.z, 0.0F);
}

TEST(ShapeNarrowPhaseUVETest, CapsuleOrientedBoxUsesFinitePrecisionForLargeCenterDelta) {
    const float maximumValue = std::numeric_limits<float>::max();
    const float centerMagnitude = maximumValue * 0.99F;
    const Math::QuaternionUVE identityRotation{};
    const std::optional<Math::PenetrationUVE> penetration =
        Detail::ComputeCapsuleOrientedBoxPenetrationUVE(
            {centerMagnitude, 0.0F, 0.0F}, {maximumValue, 1.0F, 1.0F}, identityRotation,
            {-centerMagnitude, 0.0F, 0.0F}, {-centerMagnitude, 0.0F, 0.0F}, maximumValue);
    ASSERT_TRUE(penetration.has_value());
    EXPECT_TRUE(std::isfinite(penetration->depth));
    EXPECT_GT(penetration->depth, 0.0F);
    EXPECT_GT(penetration->axis.x, 0.0F);
    EXPECT_FLOAT_EQ(penetration->axis.y, 0.0F);
    EXPECT_FLOAT_EQ(penetration->axis.z, 0.0F);
}

TEST(ShapeNarrowPhaseUVETest, SphereSphereUsesFinitePrecisionForLargeRadiusOverlap) {
    const float radius = std::numeric_limits<float>::max() * 0.5F;
    const std::optional<Math::PenetrationUVE> penetration =
        Detail::ComputeSphereSpherePenetrationUVE({-radius, 0.0F, 0.0F}, radius,
                                                   {0.0F, 0.0F, 0.0F}, radius);
    ASSERT_TRUE(penetration.has_value());
    EXPECT_TRUE(std::isfinite(penetration->depth));
    EXPECT_FLOAT_EQ(penetration->depth, radius);
    EXPECT_FLOAT_EQ(penetration->axis.x, 1.0F);
    EXPECT_FLOAT_EQ(penetration->axis.y, 0.0F);
    EXPECT_FLOAT_EQ(penetration->axis.z, 0.0F);
}

TEST(ShapeNarrowPhaseUVETest, CapsuleSphereUsesFinitePrecisionForLargeRadiusOverlap) {
    const float radius = std::numeric_limits<float>::max() * 0.5F;
    const std::optional<Math::PenetrationUVE> penetration =
        Detail::ComputeCapsuleSpherePenetrationUVE({-radius, 0.0F, 0.0F}, {radius, 0.0F, 0.0F},
                                                    radius, {std::numeric_limits<float>::max(), 0.0F, 0.0F},
                                                    radius);
    ASSERT_TRUE(penetration.has_value());
    EXPECT_TRUE(std::isfinite(penetration->depth));
    EXPECT_FLOAT_EQ(penetration->depth, radius);
    EXPECT_FLOAT_EQ(penetration->axis.x, 1.0F);
    EXPECT_FLOAT_EQ(penetration->axis.y, 0.0F);
    EXPECT_FLOAT_EQ(penetration->axis.z, 0.0F);
}

TEST(ShapeNarrowPhaseUVETest, CapsuleCapsuleUsesFinitePrecisionForLargeRadiusOverlap) {
    const float radius = std::numeric_limits<float>::max() * 0.5F;
    const std::optional<Math::PenetrationUVE> penetration =
        Detail::ComputeCapsuleCapsulePenetrationUVE(
            {-radius, 0.0F, 0.0F}, {radius, 0.0F, 0.0F}, radius,
            {-radius, radius, 0.0F}, {radius, radius, 0.0F}, radius);
    ASSERT_TRUE(penetration.has_value());
    EXPECT_TRUE(std::isfinite(penetration->depth));
    EXPECT_FLOAT_EQ(penetration->depth, radius);
    EXPECT_FLOAT_EQ(penetration->axis.x, 0.0F);
    EXPECT_FLOAT_EQ(penetration->axis.y, 1.0F);
    EXPECT_FLOAT_EQ(penetration->axis.z, 0.0F);
}

TEST(ShapeNarrowPhaseUVETest, CapsuleAabbUsesFinitePrecisionForLargeRadiusOverlap) {
    const float maximumRadius = std::numeric_limits<float>::max();
    const float expectedDepth = 2.0e30F;
    const Math::AabbUVE box{{0.0F, -1.0F, -1.0F}, {expectedDepth, 1.0F, 1.0F}};
    const std::optional<Math::PenetrationUVE> penetration =
        Detail::ComputeCapsuleAabbPenetrationUVE(
            box, {maximumRadius, 0.0F, 0.0F}, {maximumRadius, 0.0F, 0.0F}, maximumRadius);
    ASSERT_TRUE(penetration.has_value());
    EXPECT_TRUE(std::isfinite(penetration->depth));
    EXPECT_FLOAT_EQ(penetration->depth, expectedDepth);
    EXPECT_FLOAT_EQ(penetration->axis.x, -1.0F);
    EXPECT_FLOAT_EQ(penetration->axis.y, 0.0F);
    EXPECT_FLOAT_EQ(penetration->axis.z, 0.0F);
}

TEST(ShapeNarrowPhaseUVETest, SphereAabbUsesFinitePrecisionForLargeRadiusOverlap) {
    const float maximumRadius = std::numeric_limits<float>::max();
    const Math::AabbUVE box{{0.0F, -1.0F, -1.0F}, {1.0F, 1.0F, 1.0F}};
    const std::optional<Math::PenetrationUVE> penetration =
        Detail::ComputeSphereAabbPenetrationUVE(box, {2.0e38F, 0.0F, 0.0F}, maximumRadius);
    ASSERT_TRUE(penetration.has_value());
    EXPECT_TRUE(std::isfinite(penetration->depth));
    EXPECT_GT(penetration->depth, 0.0F);
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

TEST_F(ColliderShapeUVETest, DetectCollisionsUVE_RotatedCapsuleUsesWorldSegmentForSphereContact) {
    Scene::ColliderComponentUVE capsule;
    capsule.shapeType = Scene::ColliderShapeTypeUVE::Capsule;
    capsule.radius = 0.5F;
    capsule.height = 4.0F;
    const Math::QuaternionUVE quarterTurnZ{0.0F, 0.0F, 0.70710678118F, 0.70710678118F};
    MakeColliderEntityUVE({0.0F, 0.0F, 0.0F}, capsule, quarterTurnZ);

    Scene::ColliderComponentUVE sphere;
    sphere.shapeType = Scene::ColliderShapeTypeUVE::Sphere;
    sphere.radius = 0.5F;
    const Scene::EntityUVE sphereEntity = MakeColliderEntityUVE({0.0F, 0.75F, 0.0F}, sphere);

    const std::vector<CollisionPairUVE> pairs = collisionSystem.DetectCollisionsUVE(entityManager);
    ASSERT_EQ(pairs.size(), 1U);
    EXPECT_EQ(pairs[0].second, sphereEntity);
    EXPECT_NEAR(pairs[0].penetrationDepth, 0.25F, 1.0e-5F);
    EXPECT_NEAR(pairs[0].separationAxis.x, 0.0F, 1.0e-5F);
    EXPECT_NEAR(pairs[0].separationAxis.y, 1.0F, 1.0e-5F);
    EXPECT_NEAR(pairs[0].separationAxis.z, 0.0F, 1.0e-5F);
}

TEST_F(ColliderShapeUVETest, DetectCollisionsUVE_RotatedBoxSphereUsesExactLocalFrame) {
    Scene::ColliderComponentUVE box;
    box.shapeType = Scene::ColliderShapeTypeUVE::Box;
    box.halfExtents = {0.25F, 1.0F, 0.25F};
    const Math::QuaternionUVE quarterTurnZ{0.0F, 0.0F, 0.3826834324F, 0.9238795325F};
    MakeColliderEntityUVE({0.0F, 0.0F, 0.0F}, box, quarterTurnZ);

    Scene::ColliderComponentUVE sphere;
    sphere.shapeType = Scene::ColliderShapeTypeUVE::Sphere;
    sphere.radius = 0.5F;
    const Scene::EntityUVE sphereEntity = MakeColliderEntityUVE({0.5F, 0.5F, 0.0F}, sphere);

    const std::vector<CollisionPairUVE> pairs = collisionSystem.DetectCollisionsUVE(entityManager);
    ASSERT_EQ(pairs.size(), 1U);
    EXPECT_EQ(pairs[0].second, sphereEntity);
    EXPECT_GT(pairs[0].penetrationDepth, 0.0F);
    EXPECT_NEAR(pairs[0].separationAxis.x, 0.70710678F, 1.0e-4F);
    EXPECT_NEAR(pairs[0].separationAxis.y, 0.70710678F, 1.0e-4F);
    EXPECT_NEAR(pairs[0].separationAxis.z, 0.0F, 1.0e-5F);
}

TEST_F(ColliderShapeUVETest, DetectCollisionsUVE_RotatedBoxCapsuleRejectsDiagonalAabbFalsePositive) {
    Scene::ColliderComponentUVE box;
    box.shapeType = Scene::ColliderShapeTypeUVE::Box;
    box.halfExtents = {0.2F, 1.0F, 0.2F};
    const Math::QuaternionUVE quarterTurnZ{0.0F, 0.0F, 0.3826834324F, 0.9238795325F};
    MakeColliderEntityUVE({0.0F, 0.0F, 0.0F}, box, quarterTurnZ);

    Scene::ColliderComponentUVE capsule;
    capsule.shapeType = Scene::ColliderShapeTypeUVE::Capsule;
    capsule.radius = 0.2F;
    capsule.height = 2.0F;
    MakeColliderEntityUVE({0.8F, 0.8F, 0.0F}, capsule);

    EXPECT_TRUE(collisionSystem.DetectCollisionsUVE(entityManager).empty());
}

TEST_F(ColliderShapeUVETest, DetectCollisionsUVE_OrientedBoxesUseExactSatAndOrientation) {
    Scene::ColliderComponentUVE firstBox;
    firstBox.shapeType = Scene::ColliderShapeTypeUVE::Box;
    firstBox.halfExtents = {1.0F, 0.4F, 0.4F};
    const Math::QuaternionUVE quarterTurnY{0.0F, 0.3826834324F, 0.0F, 0.9238795325F};
    MakeColliderEntityUVE({0.0F, 0.0F, 0.0F}, firstBox, quarterTurnY);

    Scene::ColliderComponentUVE secondBox = firstBox;
    const Scene::EntityUVE secondEntity = MakeColliderEntityUVE({0.5F, 0.0F, -0.5F}, secondBox, quarterTurnY);

    const std::vector<CollisionPairUVE> pairs = collisionSystem.DetectCollisionsUVE(entityManager);
    ASSERT_EQ(pairs.size(), 1U);
    EXPECT_EQ(pairs[0].second, secondEntity);
    EXPECT_GT(pairs[0].penetrationDepth, 0.0F);
    EXPECT_GT(Math::DotUVE(pairs[0].separationAxis, {1.0F, 0.0F, -1.0F}), 0.0F);
    EXPECT_NEAR(Math::LengthSquaredUVE(pairs[0].separationAxis), 1.0F, 1.0e-4F);
}

TEST_F(ColliderShapeUVETest, DetectCollisionsUVE_OrientedBoxesRejectDiagonalAabbFalsePositive) {
    Scene::ColliderComponentUVE box;
    box.shapeType = Scene::ColliderShapeTypeUVE::Box;
    box.halfExtents = {1.0F, 0.2F, 0.2F};
    const Math::QuaternionUVE quarterTurnY{0.0F, 0.3826834324F, 0.0F, 0.9238795325F};
    MakeColliderEntityUVE({0.0F, 0.0F, 0.0F}, box, quarterTurnY);
    MakeColliderEntityUVE({0.49497475F, 0.0F, 0.49497475F}, box, quarterTurnY);

    EXPECT_TRUE(collisionSystem.DetectCollisionsUVE(entityManager).empty());
}

TEST_F(ColliderShapeUVETest, DetectCollisionsUVE_CapsuleCapsuleUsesExactSegmentDistanceAndOrientation) {
    Scene::ColliderComponentUVE firstCapsule;
    firstCapsule.shapeType = Scene::ColliderShapeTypeUVE::Capsule;
    firstCapsule.radius = 0.5F;
    firstCapsule.height = 4.0F;
    const Scene::EntityUVE firstEntity = MakeColliderEntityUVE({0.0F, 0.0F, 0.0F}, firstCapsule);

    Scene::ColliderComponentUVE secondCapsule;
    secondCapsule.shapeType = Scene::ColliderShapeTypeUVE::Capsule;
    secondCapsule.radius = 0.5F;
    secondCapsule.height = 4.0F;
    const Scene::EntityUVE secondEntity = MakeColliderEntityUVE({0.75F, 0.0F, 0.0F}, secondCapsule);

    const std::vector<CollisionPairUVE> pairs = collisionSystem.DetectCollisionsUVE(entityManager);
    ASSERT_EQ(pairs.size(), 1U);
    EXPECT_EQ(pairs[0].first, firstEntity);
    EXPECT_EQ(pairs[0].second, secondEntity);
    EXPECT_NEAR(pairs[0].penetrationDepth, 0.25F, 1.0e-5F);
    EXPECT_NEAR(pairs[0].separationAxis.x, 1.0F, 1.0e-5F);
    EXPECT_NEAR(pairs[0].separationAxis.y, 0.0F, 1.0e-5F);
    EXPECT_NEAR(pairs[0].separationAxis.z, 0.0F, 1.0e-5F);
}

TEST_F(ColliderShapeUVETest, DetectCollisionsUVE_CapsuleCapsuleRejectsTouchingAndDiagonalAabbFalsePositive) {
    Scene::ColliderComponentUVE firstCapsule;
    firstCapsule.shapeType = Scene::ColliderShapeTypeUVE::Capsule;
    firstCapsule.radius = 0.5F;
    firstCapsule.height = 4.0F;
    MakeColliderEntityUVE({0.0F, 0.0F, 0.0F}, firstCapsule);

    Scene::ColliderComponentUVE touchingCapsule;
    touchingCapsule.shapeType = Scene::ColliderShapeTypeUVE::Capsule;
    touchingCapsule.radius = 0.5F;
    touchingCapsule.height = 4.0F;
    MakeColliderEntityUVE({10.0F, 0.0F, 0.0F}, touchingCapsule);

    Scene::ColliderComponentUVE diagonalCapsule;
    diagonalCapsule.shapeType = Scene::ColliderShapeTypeUVE::Capsule;
    diagonalCapsule.radius = 0.5F;
    diagonalCapsule.height = 4.0F;
    MakeColliderEntityUVE({0.9F, 0.0F, 0.9F}, diagonalCapsule);

    // The touching pair is isolated at x=10. Touching centerlines at distance 1.0 and diagonal
    // centerlines at sqrt(0.9^2 + 0.9^2) must both be rejected even though their conservative
    // capsule AABBs overlap.
    EXPECT_TRUE(collisionSystem.DetectCollisionsUVE(entityManager).empty());
}

TEST_F(ColliderShapeUVETest, DetectCollisionsUVE_CapsuleSphereUsesExactCenterlineDistance) {
    Scene::ColliderComponentUVE capsule;
    capsule.shapeType = Scene::ColliderShapeTypeUVE::Capsule;
    capsule.radius = 0.5F;
    capsule.height = 4.0F;
    const Scene::EntityUVE capsuleEntity = MakeColliderEntityUVE({0.0F, 0.0F, 0.0F}, capsule);

    Scene::ColliderComponentUVE sphere;
    sphere.shapeType = Scene::ColliderShapeTypeUVE::Sphere;
    sphere.radius = 0.5F;
    const Scene::EntityUVE sphereEntity = MakeColliderEntityUVE({0.75F, 0.0F, 0.0F}, sphere);

    const std::vector<CollisionPairUVE> pairs = collisionSystem.DetectCollisionsUVE(entityManager);
    ASSERT_EQ(pairs.size(), 1U);
    EXPECT_EQ(pairs[0].first, capsuleEntity);
    EXPECT_EQ(pairs[0].second, sphereEntity);
    EXPECT_NEAR(pairs[0].penetrationDepth, 0.25F, 1.0e-5F);
    EXPECT_NEAR(pairs[0].separationAxis.x, 1.0F, 1.0e-5F);
    EXPECT_NEAR(pairs[0].separationAxis.y, 0.0F, 1.0e-5F);
    EXPECT_NEAR(pairs[0].separationAxis.z, 0.0F, 1.0e-5F);
}

TEST_F(ColliderShapeUVETest, DetectCollisionsUVE_SphereCapsuleRejectsTouchingAndDiagonalAabbFalsePositive) {
    Scene::ColliderComponentUVE touchingSphere;
    touchingSphere.shapeType = Scene::ColliderShapeTypeUVE::Sphere;
    touchingSphere.radius = 0.5F;
    MakeColliderEntityUVE({1.0F, 0.0F, 0.0F}, touchingSphere);

    Scene::ColliderComponentUVE diagonalSphere;
    diagonalSphere.shapeType = Scene::ColliderShapeTypeUVE::Sphere;
    diagonalSphere.radius = 0.5F;
    MakeColliderEntityUVE({0.9F, 1.4F, 0.9F}, diagonalSphere);

    Scene::ColliderComponentUVE capsule;
    capsule.shapeType = Scene::ColliderShapeTypeUVE::Capsule;
    capsule.radius = 0.5F;
    capsule.height = 4.0F;
    MakeColliderEntityUVE({0.0F, 0.0F, 0.0F}, capsule);

    // The touching sphere reaches the capsule AABB boundary but has no strict overlap. The
    // diagonal sphere overlaps the conservative AABB in X/Z while its centerline distance is
    // greater than the combined radius. The two spheres are also separated from one another.
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
