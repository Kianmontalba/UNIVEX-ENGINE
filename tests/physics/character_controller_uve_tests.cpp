// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/physics/character_controller_uve.h"

#include <cmath>
#include <limits>

#include <gtest/gtest.h>

#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/physics/collision_system_uve.h"
#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/rigid_body_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"
#include "uve/scene/entity_manager_uve.h"
#include "uve/scene/scene_graph_uve.h"

namespace UVE::Physics::Tests {
namespace {

class FixedCollisionSystemUVE final : public ICollisionSystemUVE {
public:
    std::vector<CollisionPairUVE> pairs;

    [[nodiscard]] std::vector<CollisionPairUVE> DetectCollisionsUVE(
        Scene::IEntityManagerUVE&) const override {
        return pairs;
    }
};

class CharacterControllerUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    Scene::EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    Scene::SceneGraphUVE sceneGraph;
    CollisionSystemUVE collisionSystem;

    Scene::EntityUVE MakeColliderEntityUVE(Math::Vector3UVE position, Math::Vector3UVE halfExtents,
                                            std::uint32_t layer = 1U,
                                            std::uint32_t mask = 0xFFFFFFFFU) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE transform;
        transform.localPosition = position;
        sceneGraph.AttachTransformUVE(entityManager, entity, transform);
        sceneGraph.UpdateUVE(entityManager);
        Scene::ColliderComponentUVE collider{halfExtents};
        collider.collisionLayer = layer;
        collider.collisionMask = mask;
        entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(entity, collider);
        return entity;
    }

    Scene::EntityUVE MakeControllerEntityUVE(Math::Vector3UVE position, Math::Vector3UVE halfExtents,
                                              std::uint32_t layer = 1U,
                                              std::uint32_t mask = 0xFFFFFFFFU) {
        const Scene::EntityUVE entity = MakeColliderEntityUVE(position, halfExtents, layer, mask);
        entityManager.AddComponentUVE<Scene::RigidBodyComponentUVE>(
            entity, Scene::RigidBodyComponentUVE{1.0F, true});
        return entity;
    }

    [[nodiscard]] Math::Vector3UVE GetWorldPositionUVE(Scene::EntityUVE entity) {
        return entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(entity).worldPosition;
    }
};

TEST(CharacterControllerCcdPolicyUVETest, ClassifiesFiniteDisplacementAgainstSweepThreshold) {
    EXPECT_EQ(EvaluateCcdEligibilityUVE(Math::Vector3UVE{0.1F, 0.0F, 0.0F}, 0.25F),
              CcdEligibilityStatusUVE::Disabled);
    EXPECT_EQ(EvaluateCcdEligibilityUVE(Math::Vector3UVE{0.25F, 0.0F, 0.0F}, 0.25F),
              CcdEligibilityStatusUVE::Enabled);
    EXPECT_EQ(EvaluateCcdEligibilityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, 0.25F),
              CcdEligibilityStatusUVE::Disabled);
}

TEST(CharacterControllerCcdPolicyUVETest, RejectsNonFiniteDisplacementAndInvalidThreshold) {
    EXPECT_EQ(EvaluateCcdEligibilityUVE(
                  Math::Vector3UVE{std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F}, 0.25F),
              CcdEligibilityStatusUVE::Invalid);
    EXPECT_EQ(EvaluateCcdEligibilityUVE(Math::Vector3UVE{1.0F, 0.0F, 0.0F}, 0.0F),
              CcdEligibilityStatusUVE::Invalid);
}

TEST(CharacterControllerCcdPolicyUVETest, RejectsFiniteDisplacementWhoseLengthOverflows) {
    const float maximumFloat = std::numeric_limits<float>::max();
    EXPECT_EQ(EvaluateCcdEligibilityUVE(Math::Vector3UVE{maximumFloat, maximumFloat, 0.0F}, 0.25F),
              CcdEligibilityStatusUVE::Invalid);
}

TEST_F(CharacterControllerUVETest, MoveUVE_RejectsFiniteDisplacementWhoseLengthOverflowsWithoutMutation) {
    const Scene::EntityUVE controller = MakeControllerEntityUVE({1.0F, 2.0F, 3.0F}, {0.5F, 0.5F, 0.5F});
    const Math::Vector3UVE positionBefore = GetWorldPositionUVE(controller);
    const float maximumFloat = std::numeric_limits<float>::max();

    const CharacterControllerMoveResultUVE moveResult = CharacterControllerUVE::MoveUVE(
        entityManager, sceneGraph, collisionSystem,
        CharacterControllerInputUVE{controller, {maximumFloat, maximumFloat, 0.0F}, 8U, 0.25F});

    EXPECT_EQ(moveResult.code, CharacterControllerMoveCodeUVE::InvalidInput);
    EXPECT_EQ(moveResult.substeps, 0U);
    EXPECT_EQ(GetWorldPositionUVE(controller), positionBefore);
}

TEST_F(CharacterControllerUVETest, MoveUVE_SkipsNonUnitFiniteCollisionAxisWithoutNonFiniteMutation) {
    const Scene::EntityUVE controller = MakeControllerEntityUVE({}, {0.5F, 0.5F, 0.5F});
    const Scene::EntityUVE target = MakeColliderEntityUVE({0.0F, 0.0F, 0.0F}, {0.5F, 0.5F, 0.5F});
    FixedCollisionSystemUVE malformedCollisionSystem;
    malformedCollisionSystem.pairs.push_back(
        CollisionPairUVE{controller, target, {std::numeric_limits<float>::max(), 0.0F, 0.0F}, 1.0F});

    const CharacterControllerMoveResultUVE result = CharacterControllerUVE::MoveUVE(
        entityManager, sceneGraph, malformedCollisionSystem,
        CharacterControllerInputUVE{controller, {1.0F, 0.0F, 0.0F}, 8U, 0.25F});

    EXPECT_EQ(result.code, CharacterControllerMoveCodeUVE::Moved);
    EXPECT_FALSE(result.blocked);
    EXPECT_EQ(result.contactCount, 0U);
    const Math::Vector3UVE position = GetWorldPositionUVE(controller);
    EXPECT_TRUE(std::isfinite(position.x));
    EXPECT_TRUE(std::isfinite(position.y));
    EXPECT_TRUE(std::isfinite(position.z));
    EXPECT_NEAR(position.x, 1.0F, 1.0e-4F);
}

TEST_F(CharacterControllerUVETest, MoveUVE_FreeSpaceAppliesRequestedDisplacementDeterministically) {
    const Scene::EntityUVE controller = MakeControllerEntityUVE({}, {0.5F, 0.5F, 0.5F});

    const CharacterControllerMoveResultUVE result = CharacterControllerUVE::MoveUVE(
        entityManager, sceneGraph, collisionSystem,
        CharacterControllerInputUVE{controller, {1.0F, 0.0F, 0.0F}, 8U, 0.25F});

    ASSERT_TRUE(result.IsAcceptedUVE());
    EXPECT_FALSE(result.blocked);
    EXPECT_EQ(result.substeps, 4U);
    EXPECT_EQ(result.contactCount, 0U);
    EXPECT_NEAR(GetWorldPositionUVE(controller).x, 1.0F, 1.0e-4F);
    EXPECT_NEAR(result.remainingDisplacement.x, 0.0F, 1.0e-4F);
}

TEST_F(CharacterControllerUVETest, MoveUVE_WallContactStopsAtMinimumTranslationDistance) {
    const Scene::EntityUVE controller = MakeControllerEntityUVE({}, {0.5F, 0.5F, 0.5F});
    MakeColliderEntityUVE({2.0F, 0.0F, 0.0F}, {0.5F, 2.0F, 2.0F});

    const CharacterControllerMoveResultUVE result = CharacterControllerUVE::MoveUVE(
        entityManager, sceneGraph, collisionSystem,
        CharacterControllerInputUVE{controller, {3.0F, 0.0F, 0.0F}, 32U, 0.25F});

    ASSERT_TRUE(result.IsAcceptedUVE());
    EXPECT_TRUE(result.blocked);
    EXPECT_FALSE(result.grounded);
    EXPECT_GT(result.contactCount, 0U);
    EXPECT_NEAR(GetWorldPositionUVE(controller).x, 1.0F, 1.0e-4F);
    EXPECT_NEAR(GetWorldPositionUVE(controller).y, 0.0F, 1.0e-4F);
}

TEST_F(CharacterControllerUVETest, MoveUVE_GroundContactReportsSupportNormal) {
    const Scene::EntityUVE controller = MakeControllerEntityUVE({0.0F, 2.0F, 0.0F}, {0.5F, 0.5F, 0.5F});
    MakeColliderEntityUVE({0.0F, 0.0F, 0.0F}, {2.0F, 0.5F, 2.0F});

    const CharacterControllerMoveResultUVE result = CharacterControllerUVE::MoveUVE(
        entityManager, sceneGraph, collisionSystem,
        CharacterControllerInputUVE{controller, {0.0F, -2.0F, 0.0F}, 16U, 0.25F});

    ASSERT_TRUE(result.IsAcceptedUVE());
    EXPECT_TRUE(result.blocked);
    EXPECT_TRUE(result.grounded);
    EXPECT_NEAR(result.groundNormal.x, 0.0F, 1.0e-5F);
    EXPECT_NEAR(result.groundNormal.y, 1.0F, 1.0e-5F);
    EXPECT_NEAR(result.groundNormal.z, 0.0F, 1.0e-5F);
    EXPECT_NEAR(GetWorldPositionUVE(controller).y, 1.0F, 1.0e-4F);
}

TEST_F(CharacterControllerUVETest, MoveUVE_WallContactPreservesTangentialDisplacement) {
    const Scene::EntityUVE controller = MakeControllerEntityUVE({}, {0.5F, 0.5F, 0.5F});
    MakeColliderEntityUVE({2.0F, 0.0F, 0.0F}, {0.5F, 2.0F, 2.0F});

    const CharacterControllerMoveResultUVE result = CharacterControllerUVE::MoveUVE(
        entityManager, sceneGraph, collisionSystem,
        CharacterControllerInputUVE{controller, {3.0F, 2.0F, 0.0F}, 32U, 0.25F});

    ASSERT_TRUE(result.IsAcceptedUVE());
    EXPECT_TRUE(result.blocked);
    EXPECT_NEAR(GetWorldPositionUVE(controller).x, 1.0F, 1.0e-4F);
    EXPECT_NEAR(GetWorldPositionUVE(controller).y, 2.0F, 1.0e-4F);
}

TEST_F(CharacterControllerUVETest, MoveUVE_IncompatibleLayerMaskPassesThroughCollider) {
    const Scene::EntityUVE controller = MakeControllerEntityUVE({}, {0.5F, 0.5F, 0.5F}, 1U, 1U);
    MakeColliderEntityUVE({2.0F, 0.0F, 0.0F}, {0.5F, 2.0F, 2.0F}, 2U, 0xFFFFFFFFU);

    const CharacterControllerMoveResultUVE result = CharacterControllerUVE::MoveUVE(
        entityManager, sceneGraph, collisionSystem,
        CharacterControllerInputUVE{controller, {3.0F, 0.0F, 0.0F}, 32U, 0.25F});

    ASSERT_TRUE(result.IsAcceptedUVE());
    EXPECT_FALSE(result.blocked);
    EXPECT_EQ(result.contactCount, 0U);
    EXPECT_NEAR(GetWorldPositionUVE(controller).x, 3.0F, 1.0e-4F);
}

TEST_F(CharacterControllerUVETest, MoveWithToIUVE_StopsBeforeThinWallWithoutTunneling) {
    const Scene::EntityUVE controller = MakeControllerEntityUVE({}, {0.5F, 0.5F, 0.5F});
    MakeColliderEntityUVE({2.0F, 0.0F, 0.0F}, {0.05F, 2.0F, 2.0F});

    const CharacterControllerMoveResultUVE result = CharacterControllerUVE::MoveWithToIUVE(
        entityManager, sceneGraph, collisionSystem,
        CharacterControllerInputUVE{controller, {10.0F, 0.0F, 0.0F}, 1U, 100.0F});

    ASSERT_TRUE(result.IsAcceptedUVE());
    EXPECT_TRUE(result.blocked);
    EXPECT_TRUE(result.toiUsed);
    EXPECT_EQ(result.contactCount, 1U);
    EXPECT_NEAR(result.earliestImpactTime, 0.145F, 1.0e-4F);
    EXPECT_NEAR(GetWorldPositionUVE(controller).x, 1.449F, 2.0e-3F);
    EXPECT_LT(GetWorldPositionUVE(controller).x, 1.5F);
}

TEST_F(CharacterControllerUVETest, MoveWithToIUVE_StepHeightOptInTraversesLowObstacle) {
    const Scene::EntityUVE controller = MakeControllerEntityUVE({0.0F, 1.0F, 0.0F}, {0.5F, 1.0F, 0.5F});
    MakeColliderEntityUVE({1.0F, 0.25F, 0.0F}, {0.5F, 0.25F, 2.0F});

    const CharacterControllerMoveResultUVE result = CharacterControllerUVE::MoveWithToIUVE(
        entityManager, sceneGraph, collisionSystem,
        CharacterControllerInputUVE{controller, {3.0F, 0.0F, 0.0F}, 8U, 100.0F, 45.0F, 0.75F});

    ASSERT_TRUE(result.IsAcceptedUVE());
    EXPECT_TRUE(result.blocked);
    EXPECT_TRUE(result.toiUsed);
    EXPECT_TRUE(result.stepUpUsed);
    EXPECT_NEAR(GetWorldPositionUVE(controller).x, 3.0F, 1.0e-4F);
    EXPECT_NEAR(GetWorldPositionUVE(controller).y, 1.0F, 1.0e-4F);
}

TEST_F(CharacterControllerUVETest, MoveWithToIUVE_StepHeightTooLowPreservesWallBlock) {
    const Scene::EntityUVE controller = MakeControllerEntityUVE({0.0F, 1.0F, 0.0F}, {0.5F, 1.0F, 0.5F});
    MakeColliderEntityUVE({1.0F, 1.0F, 0.0F}, {0.5F, 1.0F, 2.0F});

    const CharacterControllerMoveResultUVE result = CharacterControllerUVE::MoveWithToIUVE(
        entityManager, sceneGraph, collisionSystem,
        CharacterControllerInputUVE{controller, {3.0F, 0.0F, 0.0F}, 8U, 100.0F, 45.0F, 0.75F});

    ASSERT_TRUE(result.IsAcceptedUVE());
    EXPECT_TRUE(result.blocked);
    EXPECT_TRUE(result.toiUsed);
    EXPECT_FALSE(result.stepUpUsed);
    EXPECT_LT(GetWorldPositionUVE(controller).x, 0.5F);
}

TEST_F(CharacterControllerUVETest, MoveWithToIUVE_PreservesTangentialDisplacementAfterImpact) {
    const Scene::EntityUVE controller = MakeControllerEntityUVE({}, {0.5F, 0.5F, 0.5F});
    MakeColliderEntityUVE({2.0F, 0.0F, 0.0F}, {0.5F, 2.0F, 2.0F});

    const CharacterControllerMoveResultUVE result = CharacterControllerUVE::MoveWithToIUVE(
        entityManager, sceneGraph, collisionSystem,
        CharacterControllerInputUVE{controller, {10.0F, 3.0F, 0.0F}, 8U, 100.0F});

    ASSERT_TRUE(result.IsAcceptedUVE());
    EXPECT_TRUE(result.blocked);
    EXPECT_TRUE(result.toiUsed);
    EXPECT_NEAR(GetWorldPositionUVE(controller).x, 0.999F, 2.0e-3F);
    EXPECT_NEAR(GetWorldPositionUVE(controller).y, 3.0F, 2.0e-3F);
}

TEST_F(CharacterControllerUVETest, MoveWithToIUVE_IncompatibleLayerMaskPassesThrough) {
    const Scene::EntityUVE controller = MakeControllerEntityUVE({}, {0.5F, 0.5F, 0.5F}, 1U, 1U);
    MakeColliderEntityUVE({2.0F, 0.0F, 0.0F}, {0.05F, 2.0F, 2.0F}, 2U, 0xFFFFFFFFU);

    const CharacterControllerMoveResultUVE result = CharacterControllerUVE::MoveWithToIUVE(
        entityManager, sceneGraph, collisionSystem,
        CharacterControllerInputUVE{controller, {10.0F, 0.0F, 0.0F}, 1U, 100.0F});

    ASSERT_TRUE(result.IsAcceptedUVE());
    EXPECT_FALSE(result.blocked);
    EXPECT_FALSE(result.toiUsed);
    EXPECT_NEAR(GetWorldPositionUVE(controller).x, 10.0F, 1.0e-4F);
}

TEST_F(CharacterControllerUVETest, MoveUVE_ClampsInvalidGroundSlopeWithoutRejectingInput) {
    const Scene::EntityUVE controller = MakeControllerEntityUVE({}, {0.5F, 0.5F, 0.5F});

    const CharacterControllerMoveResultUVE result = CharacterControllerUVE::MoveUVE(
        entityManager, sceneGraph, collisionSystem,
        CharacterControllerInputUVE{controller, {0.1F, 0.0F, 0.0F}, 1U, 1.0F, -5.0F});

    ASSERT_TRUE(result.IsAcceptedUVE());
    EXPECT_TRUE(result.inputClamped);
    EXPECT_FALSE(result.grounded);
    EXPECT_NEAR(GetWorldPositionUVE(controller).x, 0.1F, 1.0e-4F);
}

TEST_F(CharacterControllerUVETest, MoveUVE_OptInPushesDynamicTargetWithBoundedNormalSpeed) {
    const Scene::EntityUVE controller = MakeControllerEntityUVE({}, {0.5F, 0.5F, 0.5F});
    const Scene::EntityUVE target = MakeColliderEntityUVE({2.0F, 0.0F, 0.0F}, {0.5F, 0.5F, 0.5F});
    entityManager.AddComponentUVE<Scene::RigidBodyComponentUVE>(target, Scene::RigidBodyComponentUVE{});

    const CharacterControllerMoveResultUVE result = CharacterControllerUVE::MoveUVE(
        entityManager, sceneGraph, collisionSystem,
        CharacterControllerInputUVE{controller, {3.0F, 0.0F, 0.0F}, 32U, 0.25F, 45.0F, 0.0F,
                                    true, 1.0F, 5.0F, 1.0F / 60.0F});

    ASSERT_TRUE(result.IsAcceptedUVE());
    EXPECT_TRUE(result.blocked);
    EXPECT_EQ(result.pushedBodyCount, 1U);
    EXPECT_FALSE(result.dynamicBodyPushClamped);
    const Scene::RigidBodyComponentUVE& pushedBody =
        entityManager.GetComponentUVE<Scene::RigidBodyComponentUVE>(target);
    EXPECT_NEAR(pushedBody.velocity.x, 5.0F, 1.0e-4F);
    EXPECT_NEAR(pushedBody.velocity.y, 0.0F, 1.0e-4F);
}

TEST_F(CharacterControllerUVETest, MoveUVE_DefaultPushPolicyLeavesDynamicTargetUnchanged) {
    const Scene::EntityUVE controller = MakeControllerEntityUVE({}, {0.5F, 0.5F, 0.5F});
    const Scene::EntityUVE target = MakeColliderEntityUVE({2.0F, 0.0F, 0.0F}, {0.5F, 0.5F, 0.5F});
    entityManager.AddComponentUVE<Scene::RigidBodyComponentUVE>(target, Scene::RigidBodyComponentUVE{});

    const CharacterControllerMoveResultUVE result = CharacterControllerUVE::MoveUVE(
        entityManager, sceneGraph, collisionSystem,
        CharacterControllerInputUVE{controller, {3.0F, 0.0F, 0.0F}, 32U, 0.25F});

    ASSERT_TRUE(result.IsAcceptedUVE());
    EXPECT_EQ(result.pushedBodyCount, 0U);
    EXPECT_FLOAT_EQ(entityManager.GetComponentUVE<Scene::RigidBodyComponentUVE>(target).velocity.x, 0.0F);
}

TEST_F(CharacterControllerUVETest, MoveUVE_DoesNotPushKinematicTargetAndClampsInvalidPushPolicy) {
    const Scene::EntityUVE controller = MakeControllerEntityUVE({}, {0.5F, 0.5F, 0.5F});
    const Scene::EntityUVE target = MakeColliderEntityUVE({2.0F, 0.0F, 0.0F}, {0.5F, 0.5F, 0.5F});
    entityManager.AddComponentUVE<Scene::RigidBodyComponentUVE>(target, Scene::RigidBodyComponentUVE{1.0F, true});

    const CharacterControllerMoveResultUVE result = CharacterControllerUVE::MoveUVE(
        entityManager, sceneGraph, collisionSystem,
        CharacterControllerInputUVE{controller, {3.0F, 0.0F, 0.0F}, 32U, 0.25F, 45.0F, 0.0F,
                                    true, -1.0F, 0.0F, -1.0F});

    ASSERT_TRUE(result.IsAcceptedUVE());
    EXPECT_TRUE(result.dynamicBodyPushClamped);
    EXPECT_EQ(result.pushedBodyCount, 0U);
    EXPECT_TRUE(entityManager.GetComponentUVE<Scene::RigidBodyComponentUVE>(target).isKinematic);
}

TEST_F(CharacterControllerUVETest, MoveUVE_RejectsDynamicBodyAndClampsInvalidBudget) {
    const Scene::EntityUVE dynamic = MakeColliderEntityUVE({5.0F, 0.0F, 0.0F}, {0.5F, 0.5F, 0.5F});
    entityManager.AddComponentUVE<Scene::RigidBodyComponentUVE>(dynamic, Scene::RigidBodyComponentUVE{});
    const CharacterControllerMoveResultUVE rejected = CharacterControllerUVE::MoveUVE(
        entityManager, sceneGraph, collisionSystem,
        CharacterControllerInputUVE{dynamic, {1.0F, 0.0F, 0.0F}, 8U, 0.25F});
    EXPECT_EQ(rejected.code, CharacterControllerMoveCodeUVE::NonKinematicBody);

    const Scene::EntityUVE controller = MakeControllerEntityUVE({}, {0.5F, 0.5F, 0.5F});
    const CharacterControllerMoveResultUVE clamped = CharacterControllerUVE::MoveUVE(
        entityManager, sceneGraph, collisionSystem,
        CharacterControllerInputUVE{controller, {0.1F, 0.0F, 0.0F}, 0U, -1.0F});
    ASSERT_TRUE(clamped.IsAcceptedUVE());
    EXPECT_TRUE(clamped.inputClamped);
    EXPECT_NEAR(GetWorldPositionUVE(controller).x, 0.1F, 1.0e-4F);
}

} // namespace
} // namespace UVE::Physics::Tests
