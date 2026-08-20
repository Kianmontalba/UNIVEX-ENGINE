// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/physics/physics_system_uve.h"

#include <cmath>
#include <limits>
#include <optional>

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

constexpr float kEpsilon = 1e-3F;

class PhysicsSystemUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    Scene::EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    Scene::SceneGraphUVE sceneGraph;
    CollisionSystemUVE collisionSystem;

    Scene::EntityUVE MakeBodyEntityUVE(Math::Vector3UVE position, Scene::RigidBodyComponentUVE rigidBody,
                                        std::optional<Math::Vector3UVE> colliderHalfExtents = std::nullopt) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE local;
        local.localPosition = position;
        sceneGraph.AttachTransformUVE(entityManager, entity, local);
        sceneGraph.UpdateUVE(entityManager);
        entityManager.AddComponentUVE<Scene::RigidBodyComponentUVE>(entity, rigidBody);
        if (colliderHalfExtents.has_value()) {
            entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(entity,
                                                                        Scene::ColliderComponentUVE{*colliderHalfExtents});
        }
        return entity;
    }

    Scene::EntityUVE MakeStaticColliderEntityUVE(Math::Vector3UVE position, Math::Vector3UVE halfExtents) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE local;
        local.localPosition = position;
        sceneGraph.AttachTransformUVE(entityManager, entity, local);
        sceneGraph.UpdateUVE(entityManager);
        entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(entity, Scene::ColliderComponentUVE{halfExtents});
        return entity;
    }

    [[nodiscard]] Math::Vector3UVE GetWorldPositionUVE(Scene::EntityUVE entity) {
        return entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(entity).worldPosition;
    }

    static void RunStepsUVE(IPhysicsSystemUVE& physicsSystem, Scene::IEntityManagerUVE& entityManager,
                            Scene::ISceneGraphUVE& sceneGraph, int steps, float fixedDeltaTimeSeconds) {
        for (int i = 0; i < steps; ++i) {
            physicsSystem.StepUVE(entityManager, sceneGraph, fixedDeltaTimeSeconds);
        }
    }
};

TEST(RigidBodyComponentUVETest, IsRigidBodyComponentValidUVE_RejectsUnsafeValuesAndPreservesZeroMass) {
    EXPECT_TRUE(Scene::IsRigidBodyComponentValidUVE(Scene::RigidBodyComponentUVE{}));

    Scene::RigidBodyComponentUVE zeroMass = {};
    zeroMass.mass = 0.0F;
    EXPECT_TRUE(Scene::IsRigidBodyComponentValidUVE(zeroMass));

    Scene::RigidBodyComponentUVE invalid = {};
    invalid.mass = -1.0F;
    EXPECT_FALSE(Scene::IsRigidBodyComponentValidUVE(invalid));
    invalid = {};
    invalid.velocity.x = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(Scene::IsRigidBodyComponentValidUVE(invalid));
    invalid = {};
    invalid.drag = -0.1F;
    EXPECT_FALSE(Scene::IsRigidBodyComponentValidUVE(invalid));
    invalid = {};
    invalid.gravityScale = std::numeric_limits<float>::infinity();
    EXPECT_FALSE(Scene::IsRigidBodyComponentValidUVE(invalid));
    invalid = {};
    invalid.gravityScale = -1.0F;
    EXPECT_FALSE(Scene::IsRigidBodyComponentValidUVE(invalid));
    invalid = {};
    invalid.angularVelocity.x = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(Scene::IsRigidBodyComponentValidUVE(invalid));
    invalid = {};
    invalid.torque.y = std::numeric_limits<float>::infinity();
    EXPECT_FALSE(Scene::IsRigidBodyComponentValidUVE(invalid));
    invalid = {};
    invalid.inverseInertia.z = -0.1F;
    EXPECT_FALSE(Scene::IsRigidBodyComponentValidUVE(invalid));
}

TEST_F(PhysicsSystemUVETest, StepUVE_AngularStateIntegratesTorqueAndAdvancesLocalRotation) {
    PhysicsSystemUVE physicsSystem(collisionSystem, Math::Vector3UVE{});
    Scene::RigidBodyComponentUVE rigidBody;
    rigidBody.angularVelocity = Math::Vector3UVE{0.0F, 1.0F, 0.0F};
    rigidBody.torque = Math::Vector3UVE{0.0F, 1.0F, 0.0F};
    rigidBody.inverseInertia = Math::Vector3UVE{0.0F, 0.5F, 0.0F};
    const Scene::EntityUVE body = MakeBodyEntityUVE(Math::Vector3UVE{}, rigidBody);

    physicsSystem.StepUVE(entityManager, sceneGraph, 0.5F);

    const Scene::RigidBodyComponentUVE& updated =
        entityManager.GetComponentUVE<Scene::RigidBodyComponentUVE>(body);
    EXPECT_NEAR(updated.angularVelocity.y, 1.25F, kEpsilon);
    const float angle = 0.625F;
    const Math::QuaternionUVE rotation =
        entityManager.GetComponentUVE<Scene::TransformComponentUVE>(body).localRotation;
    EXPECT_NEAR(rotation.x, 0.0F, kEpsilon);
    EXPECT_NEAR(rotation.y, std::sin(angle * 0.5F), kEpsilon);
    EXPECT_NEAR(rotation.z, 0.0F, kEpsilon);
    EXPECT_NEAR(rotation.w, std::cos(angle * 0.5F), kEpsilon);
}

TEST_F(PhysicsSystemUVETest, StepUVE_AppliesGyroscopicTorqueToAngularVelocity) {
    PhysicsSystemUVE physicsSystem(collisionSystem, Math::Vector3UVE{});
    Scene::RigidBodyComponentUVE rigidBody;
    rigidBody.angularVelocity = Math::Vector3UVE{1.0F, 2.0F, 0.0F};
    rigidBody.inverseInertia = Math::Vector3UVE{2.0F, 3.0F, 4.0F};
    const Scene::EntityUVE body = MakeBodyEntityUVE(Math::Vector3UVE{}, rigidBody);

    physicsSystem.StepUVE(entityManager, sceneGraph, 0.1F);

    const Scene::RigidBodyComponentUVE& updated =
        entityManager.GetComponentUVE<Scene::RigidBodyComponentUVE>(body);
    EXPECT_NEAR(updated.angularVelocity.x, 1.0F, kEpsilon);
    EXPECT_NEAR(updated.angularVelocity.y, 2.0F, kEpsilon);
    EXPECT_NEAR(updated.angularVelocity.z, 0.13333333F, kEpsilon);
}

TEST_F(PhysicsSystemUVETest, StepUVE_ZeroAngularDefaultsPreserveIdentityRotation) {
    PhysicsSystemUVE physicsSystem(collisionSystem, Math::Vector3UVE{});
    const Scene::EntityUVE body = MakeBodyEntityUVE(
        Math::Vector3UVE{}, Scene::RigidBodyComponentUVE{});

    RunStepsUVE(physicsSystem, entityManager, sceneGraph, 3, 0.1F);

    EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(body).localRotation,
              Math::QuaternionUVE{});
}

TEST_F(PhysicsSystemUVETest, StepUVE_DynamicBodyUnderGravity_MatchesHandComputedSemiImplicitEuler) {
    PhysicsSystemUVE physicsSystem(collisionSystem, Math::Vector3UVE{0.0F, -10.0F, 0.0F});
    const Scene::EntityUVE body = MakeBodyEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Scene::RigidBodyComponentUVE{});

    RunStepsUVE(physicsSystem, entityManager, sceneGraph, 3, 0.1F);

    // Semi-implicit Euler, v0=0, g=-10, dt=0.1: v_n = g*dt*n; pos_n = g*dt^2*n(n+1)/2.
    // n=3: pos = -10 * 0.01 * 6 = -0.6.
    EXPECT_NEAR(GetWorldPositionUVE(body).y, -0.6F, kEpsilon);
    EXPECT_NEAR(entityManager.GetComponentUVE<Scene::RigidBodyComponentUVE>(body).velocity.y, -3.0F, kEpsilon);
}

TEST_F(PhysicsSystemUVETest, StepUVE_GravityScaleDouble_FallsExactlyTwiceAsFar) {
    PhysicsSystemUVE physicsSystem(collisionSystem, Math::Vector3UVE{0.0F, -10.0F, 0.0F});
    Scene::RigidBodyComponentUVE normalScale;
    normalScale.gravityScale = 1.0F;
    Scene::RigidBodyComponentUVE doubleScale;
    doubleScale.gravityScale = 2.0F;
    const Scene::EntityUVE normalBody = MakeBodyEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, normalScale);
    const Scene::EntityUVE fastBody = MakeBodyEntityUVE(Math::Vector3UVE{100.0F, 0.0F, 0.0F}, doubleScale);

    RunStepsUVE(physicsSystem, entityManager, sceneGraph, 5, 0.05F);

    const float normalDrop = -GetWorldPositionUVE(normalBody).y;
    const float fastDrop = -GetWorldPositionUVE(fastBody).y;
    ASSERT_GT(normalDrop, 0.0F);
    EXPECT_NEAR(fastDrop, normalDrop * 2.0F, kEpsilon);
}

TEST_F(PhysicsSystemUVETest, StepUVE_KinematicBody_NeverMoves) {
    PhysicsSystemUVE physicsSystem(collisionSystem);
    Scene::RigidBodyComponentUVE rigidBody;
    rigidBody.isKinematic = true;
    rigidBody.velocity = Math::Vector3UVE{5.0F, 5.0F, 5.0F};
    const Scene::EntityUVE body = MakeBodyEntityUVE(Math::Vector3UVE{1.0F, 2.0F, 3.0F}, rigidBody);

    RunStepsUVE(physicsSystem, entityManager, sceneGraph, 10, 0.1F);

    EXPECT_EQ(GetWorldPositionUVE(body), (Math::Vector3UVE{1.0F, 2.0F, 3.0F}));
}

TEST_F(PhysicsSystemUVETest, StepUVE_Drag_DampsVelocityEachStep) {
    PhysicsSystemUVE physicsSystem(collisionSystem, Math::Vector3UVE{0.0F, 0.0F, 0.0F}); // isolate drag from gravity
    Scene::RigidBodyComponentUVE rigidBody;
    rigidBody.velocity = Math::Vector3UVE{10.0F, 0.0F, 0.0F};
    rigidBody.drag = 0.5F;
    const Scene::EntityUVE body = MakeBodyEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, rigidBody);

    physicsSystem.StepUVE(entityManager, sceneGraph, 0.1F);

    // velocity *= max(0, 1 - drag*dt) = 1 - 0.05 = 0.95.
    EXPECT_NEAR(entityManager.GetComponentUVE<Scene::RigidBodyComponentUVE>(body).velocity.x, 9.5F, kEpsilon);
}

TEST_F(PhysicsSystemUVETest, StepUVE_DynamicBodyFallingOntoStaticGround_StopsAtRestingHeight) {
    PhysicsSystemUVE physicsSystem(collisionSystem, Math::Vector3UVE{0.0F, -9.81F, 0.0F});
    MakeStaticColliderEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{5.0F, 0.5F, 5.0F}); // top at y=0.5
    const Scene::EntityUVE body =
        MakeBodyEntityUVE(Math::Vector3UVE{0.0F, 3.0F, 0.0F}, Scene::RigidBodyComponentUVE{}, Math::Vector3UVE{0.5F, 0.5F, 0.5F});

    RunStepsUVE(physicsSystem, entityManager, sceneGraph, 300, 1.0F / 60.0F);

    // Resting position: ground top (0.5) + body half-height (0.5) = 1.0.
    EXPECT_NEAR(GetWorldPositionUVE(body).y, 1.0F, 0.05F);
    EXPECT_GE(GetWorldPositionUVE(body).y, 1.0F - kEpsilon); // never sank below resting height
}

TEST_F(PhysicsSystemUVETest, StepUVE_SlidingIntoWall_PreservesTangentialVelocity) {
    PhysicsSystemUVE physicsSystem(collisionSystem, Math::Vector3UVE{0.0F, 0.0F, 0.0F}); // isolate collision response
    MakeStaticColliderEntityUVE(Math::Vector3UVE{2.0F, 0.0F, 0.0F}, Math::Vector3UVE{0.5F, 10.0F, 10.0F}); // wall
    Scene::RigidBodyComponentUVE rigidBody;
    rigidBody.velocity = Math::Vector3UVE{5.0F, 3.0F, 0.0F}; // moving into the wall (+x) and sliding (+y)
    const Scene::EntityUVE body =
        MakeBodyEntityUVE(Math::Vector3UVE{1.6F, 0.0F, 0.0F}, rigidBody, Math::Vector3UVE{0.5F, 0.5F, 0.5F});

    physicsSystem.StepUVE(entityManager, sceneGraph, 0.01F);

    const Math::Vector3UVE finalVelocity = entityManager.GetComponentUVE<Scene::RigidBodyComponentUVE>(body).velocity;
    EXPECT_NEAR(finalVelocity.x, 0.0F, kEpsilon);  // into-wall component removed
    EXPECT_NEAR(finalVelocity.y, 3.0F, kEpsilon);  // tangential (sliding) component untouched
}

TEST_F(PhysicsSystemUVETest, StepUVE_SameScenarioTwice_ProducesDeterministicResults) {
    const auto RunScenarioUVE = []() {
        Memory::MemoryManagerUVE localMemoryManager;
        Events::EventSystemUVE localEventSystem;
        Scene::EntityManagerUVE localEntityManager(localMemoryManager.GetDefaultAllocatorUVE(), localEventSystem);
        Scene::SceneGraphUVE localSceneGraph;
        CollisionSystemUVE localCollisionSystem;
        PhysicsSystemUVE localPhysicsSystem(localCollisionSystem, Math::Vector3UVE{0.0F, -9.81F, 0.0F});

        const Scene::EntityUVE groundEntity = localEntityManager.CreateEntityUVE();
        Scene::TransformComponentUVE groundTransform;
        groundTransform.localPosition = Math::Vector3UVE{0.0F, 0.0F, 0.0F};
        localSceneGraph.AttachTransformUVE(localEntityManager, groundEntity, groundTransform);
        localSceneGraph.UpdateUVE(localEntityManager);
        localEntityManager.AddComponentUVE<Scene::ColliderComponentUVE>(
            groundEntity, Scene::ColliderComponentUVE{Math::Vector3UVE{5.0F, 0.5F, 5.0F}});

        const Scene::EntityUVE bodyEntity = localEntityManager.CreateEntityUVE();
        Scene::TransformComponentUVE bodyTransform;
        bodyTransform.localPosition = Math::Vector3UVE{0.0F, 3.0F, 0.0F};
        localSceneGraph.AttachTransformUVE(localEntityManager, bodyEntity, bodyTransform);
        localSceneGraph.UpdateUVE(localEntityManager);
        localEntityManager.AddComponentUVE<Scene::RigidBodyComponentUVE>(bodyEntity, Scene::RigidBodyComponentUVE{});
        localEntityManager.AddComponentUVE<Scene::ColliderComponentUVE>(
            bodyEntity, Scene::ColliderComponentUVE{Math::Vector3UVE{0.5F, 0.5F, 0.5F}});

        for (int i = 0; i < 120; ++i) {
            localPhysicsSystem.StepUVE(localEntityManager, localSceneGraph, 1.0F / 60.0F);
        }
        return localEntityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(bodyEntity).worldPosition;
    };

    const Math::Vector3UVE firstRun = RunScenarioUVE();
    const Math::Vector3UVE secondRun = RunScenarioUVE();

    EXPECT_EQ(firstRun, secondRun);
}

TEST_F(PhysicsSystemUVETest, StepUVE_StackedDynamicBoxesOnStaticGround_SettleToFiniteNonOverlappingState) {
    PhysicsSystemUVE physicsSystem(collisionSystem, Math::Vector3UVE{0.0F, -9.81F, 0.0F});
    MakeStaticColliderEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{5.0F, 0.5F, 5.0F}); // top at y=0.5
    const Scene::EntityUVE lower =
        MakeBodyEntityUVE(Math::Vector3UVE{0.0F, 2.0F, 0.0F}, Scene::RigidBodyComponentUVE{}, Math::Vector3UVE{0.5F, 0.5F, 0.5F});
    const Scene::EntityUVE upper =
        MakeBodyEntityUVE(Math::Vector3UVE{0.05F, 4.0F, 0.0F}, Scene::RigidBodyComponentUVE{}, Math::Vector3UVE{0.5F, 0.5F, 0.5F});

    RunStepsUVE(physicsSystem, entityManager, sceneGraph, 400, 1.0F / 60.0F);

    const Math::Vector3UVE lowerPosition = GetWorldPositionUVE(lower);
    const Math::Vector3UVE upperPosition = GetWorldPositionUVE(upper);

    EXPECT_TRUE(std::isfinite(lowerPosition.y));
    EXPECT_TRUE(std::isfinite(upperPosition.y));
    EXPECT_GE(lowerPosition.y, 1.0F - 0.05F);           // resting on ground (top 0.5 + half-height 0.5), not sunk in
    EXPECT_GE(upperPosition.y, lowerPosition.y + 1.0F - 0.05F); // resting on lower box, not sunk into it
}

TEST_F(PhysicsSystemUVETest, StepUVE_RestitutionHalf_ReflectsAndScalesNormalVelocity) {
    PhysicsSystemUVE physicsSystem(collisionSystem, Math::Vector3UVE{0.0F, 0.0F, 0.0F}); // isolate collision response
    const Scene::EntityUVE ground = MakeStaticColliderEntityUVE(Math::Vector3UVE{2.0F, 0.0F, 0.0F}, Math::Vector3UVE{0.5F, 10.0F, 10.0F});
    entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(ground).restitution = 0.5F;
    Scene::RigidBodyComponentUVE rigidBody;
    rigidBody.velocity = Math::Vector3UVE{5.0F, 0.0F, 0.0F}; // moving straight into the wall (+x)
    const Scene::EntityUVE body =
        MakeBodyEntityUVE(Math::Vector3UVE{1.6F, 0.0F, 0.0F}, rigidBody, Math::Vector3UVE{0.5F, 0.5F, 0.5F});
    entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(body).restitution = 0.5F;

    physicsSystem.StepUVE(entityManager, sceneGraph, 0.01F);

    // Combined restitution = average(0.5, 0.5) = 0.5. New normal velocity = -restitution *
    // intoSurface = -0.5 * 5.0 = -2.5 (reflected, scaled — not simply zeroed).
    const float finalVelocityX = entityManager.GetComponentUVE<Scene::RigidBodyComponentUVE>(body).velocity.x;
    EXPECT_NEAR(finalVelocityX, -2.5F, kEpsilon);
}

TEST_F(PhysicsSystemUVETest, StepUVE_FrictionOne_FullyDampsTangentialVelocity) {
    PhysicsSystemUVE physicsSystem(collisionSystem, Math::Vector3UVE{0.0F, 0.0F, 0.0F}); // isolate collision response
    const Scene::EntityUVE ground = MakeStaticColliderEntityUVE(Math::Vector3UVE{2.0F, 0.0F, 0.0F}, Math::Vector3UVE{0.5F, 10.0F, 10.0F});
    entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(ground).friction = 1.0F;
    Scene::RigidBodyComponentUVE rigidBody;
    rigidBody.velocity = Math::Vector3UVE{5.0F, 3.0F, 0.0F}; // moving into the wall (+x) and sliding (+y)
    const Scene::EntityUVE body =
        MakeBodyEntityUVE(Math::Vector3UVE{1.6F, 0.0F, 0.0F}, rigidBody, Math::Vector3UVE{0.5F, 0.5F, 0.5F});
    entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(body).friction = 1.0F;

    physicsSystem.StepUVE(entityManager, sceneGraph, 0.01F);

    // Combined friction = average(1.0, 1.0) = 1.0 -> frictionFactor = 0 -> tangential fully damped.
    const Math::Vector3UVE finalVelocity = entityManager.GetComponentUVE<Scene::RigidBodyComponentUVE>(body).velocity;
    EXPECT_NEAR(finalVelocity.x, 0.0F, kEpsilon);
    EXPECT_NEAR(finalVelocity.y, 0.0F, kEpsilon);
}

TEST_F(PhysicsSystemUVETest, StepUVE_StaticVsStaticOverlap_NeitherMovesAndNoDivideByZero) {
    PhysicsSystemUVE physicsSystem(collisionSystem);
    const Scene::EntityUVE a = MakeStaticColliderEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    const Scene::EntityUVE b = MakeStaticColliderEntityUVE(Math::Vector3UVE{0.5F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});

    EXPECT_NO_FATAL_FAILURE(RunStepsUVE(physicsSystem, entityManager, sceneGraph, 5, 1.0F / 60.0F));

    EXPECT_EQ(GetWorldPositionUVE(a), (Math::Vector3UVE{0.0F, 0.0F, 0.0F}));
    EXPECT_EQ(GetWorldPositionUVE(b), (Math::Vector3UVE{0.5F, 0.0F, 0.0F}));
}

#if UVE_DEBUG
TEST_F(PhysicsSystemUVETest, StepUVE_InvalidRigidBodyParameters_Asserts) {
    Scene::RigidBodyComponentUVE invalid = {};
    invalid.drag = -1.0F;
    const Scene::EntityUVE body = MakeBodyEntityUVE(Math::Vector3UVE{}, invalid);
    PhysicsSystemUVE physicsSystem(collisionSystem);

    ASSERT_NE(body, Scene::kInvalidEntityUVE);
    EXPECT_DEATH({ physicsSystem.StepUVE(entityManager, sceneGraph, 1.0F / 60.0F); }, "");
}
#endif

} // namespace
} // namespace UVE::Physics::Tests
