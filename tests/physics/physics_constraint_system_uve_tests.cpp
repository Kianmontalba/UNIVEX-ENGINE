// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/physics/physics_constraint_system_uve.h"

#include <limits>

#include <gtest/gtest.h>

#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/physics/collision_system_uve.h"
#include "uve/physics/physics_system_uve.h"
#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/rigid_body_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"
#include "uve/scene/entity_manager_uve.h"
#include "uve/scene/scene_graph_uve.h"

namespace UVE::Physics::Tests {
namespace {

constexpr float kEpsilon = 1.0e-3F;

class PhysicsConstraintSystemUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    Scene::EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    Scene::SceneGraphUVE sceneGraph;
    CollisionSystemUVE collisionSystem;
    PhysicsConstraintSystemUVE constraintSystem;

    Scene::EntityUVE MakeBodyUVE(Math::Vector3UVE position, float mass = 1.0F) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE transform;
        transform.localPosition = position;
        sceneGraph.AttachTransformUVE(entityManager, entity, transform);
        sceneGraph.UpdateUVE(entityManager);
        entityManager.AddComponentUVE<Scene::RigidBodyComponentUVE>(entity,
                                                                     Scene::RigidBodyComponentUVE{mass, false});
        return entity;
    }

    Scene::EntityUVE MakeStaticUVE(Math::Vector3UVE position) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE transform;
        transform.localPosition = position;
        sceneGraph.AttachTransformUVE(entityManager, entity, transform);
        sceneGraph.UpdateUVE(entityManager);
        entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(entity,
                                                                    Scene::ColliderComponentUVE{{0.5F, 0.5F, 0.5F}});
        return entity;
    }

    [[nodiscard]] Math::Vector3UVE WorldPositionUVE(Scene::EntityUVE entity) const {
        return entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(entity).worldPosition;
    }
};

TEST_F(PhysicsConstraintSystemUVETest, BuildConstraintIslandPlanUVE_GroupsConnectedEdgesDeterministically) {
    const Scene::EntityUVE first = MakeBodyUVE({});
    const Scene::EntityUVE second = MakeBodyUVE({2.0F, 0.0F, 0.0F});
    const Scene::EntityUVE third = MakeBodyUVE({4.0F, 0.0F, 0.0F});
    const Scene::EntityUVE fourth = MakeBodyUVE({8.0F, 0.0F, 0.0F});
    const Scene::EntityUVE fifth = MakeBodyUVE({10.0F, 0.0F, 0.0F});
    const std::array<ConstraintIslandEdgeUVE, 3U> edges{
        ConstraintIslandEdgeUVE{first, second}, ConstraintIslandEdgeUVE{second, third},
        ConstraintIslandEdgeUVE{fourth, fifth}};

    ConstraintIslandPlanUVE plan;
    ASSERT_TRUE(BuildConstraintIslandPlanUVE(edges, plan));
    ASSERT_EQ(plan.entityCount, 5U);
    ASSERT_EQ(plan.islandCount, 2U);
    EXPECT_EQ(plan.entities[0U], first);
    EXPECT_EQ(plan.entities[1U], second);
    EXPECT_EQ(plan.entities[2U], third);
    EXPECT_EQ(plan.entities[3U], fourth);
    EXPECT_EQ(plan.entities[4U], fifth);
    EXPECT_EQ(plan.islandIndices[0U], 0U);
    EXPECT_EQ(plan.islandIndices[1U], 0U);
    EXPECT_EQ(plan.islandIndices[2U], 0U);
    EXPECT_EQ(plan.islandIndices[3U], 1U);
    EXPECT_EQ(plan.islandIndices[4U], 1U);
}

TEST_F(PhysicsConstraintSystemUVETest, BuildConstraintIslandPlanUVE_RejectsInvalidEdgeAtomically) {
    const Scene::EntityUVE first = MakeBodyUVE({});
    ConstraintIslandPlanUVE plan;
    plan.entityCount = 7U;
    plan.islandCount = 3U;
    const std::array<ConstraintIslandEdgeUVE, 1U> invalidEdges{
        ConstraintIslandEdgeUVE{first, Scene::kInvalidEntityUVE}};

    EXPECT_FALSE(BuildConstraintIslandPlanUVE(invalidEdges, plan));
    EXPECT_EQ(plan.entityCount, 7U);
    EXPECT_EQ(plan.islandCount, 3U);
}

TEST_F(PhysicsConstraintSystemUVETest, AddDistanceAndSolveUVE_MassWeightedCorrectionPreservesRestLength) {
    const Scene::EntityUVE first = MakeBodyUVE({0.0F, 0.0F, 0.0F}, 1.0F);
    const Scene::EntityUVE second = MakeBodyUVE({10.0F, 0.0F, 0.0F}, 1.0F);

    const PhysicsConstraintMutationResultUVE added = constraintSystem.AddDistanceConstraintUVE(
        DistanceConstraintUVE{first, second, {}, {}, 4.0F});
    ASSERT_TRUE(added.IsAcceptedUVE());

    const PhysicsConstraintSolveResultUVE solved = constraintSystem.SolveUVE(entityManager, sceneGraph);

    EXPECT_TRUE(solved.islandPlanValid);
    EXPECT_EQ(solved.islandCount, 1U);
    EXPECT_GT(solved.solvedConstraintCount, 0U);
    EXPECT_NEAR(WorldPositionUVE(first).x, 3.0F, kEpsilon);
    EXPECT_NEAR(WorldPositionUVE(second).x, 7.0F, kEpsilon);
}

TEST_F(PhysicsConstraintSystemUVETest, SolveUVE_SkipsOverflowedFiniteConstraintGeometryAtomically) {
    const float maximum = std::numeric_limits<float>::max();
    const Scene::EntityUVE first = MakeBodyUVE({maximum, 0.0F, 0.0F});
    const Scene::EntityUVE second = MakeBodyUVE({0.0F, maximum, 0.0F});
    ASSERT_TRUE(constraintSystem.AddDistanceConstraintUVE(
                    DistanceConstraintUVE{first, second, {}, {}, 1.0F})
                    .IsAcceptedUVE());

    const PhysicsConstraintSolveResultUVE solved = constraintSystem.SolveUVE(entityManager, sceneGraph);

    EXPECT_TRUE(solved.islandPlanValid);
    EXPECT_EQ(solved.solvedConstraintCount, 0U);
    EXPECT_EQ(solved.skippedConstraintCount, 1U);
    EXPECT_FLOAT_EQ(WorldPositionUVE(first).x, maximum);
    EXPECT_FLOAT_EQ(WorldPositionUVE(second).y, maximum);
}

TEST_F(PhysicsConstraintSystemUVETest, SolveUVE_SkipsSubnormalMassInverseOverflowAtomically) {
    const float subnormalMass = std::numeric_limits<float>::denorm_min();
    const Scene::EntityUVE first = MakeBodyUVE({0.0F, 0.0F, 0.0F}, subnormalMass);
    const Scene::EntityUVE second = MakeBodyUVE({4.0F, 0.0F, 0.0F}, 1.0F);
    ASSERT_TRUE(constraintSystem.AddDistanceConstraintUVE(
                    DistanceConstraintUVE{first, second, {}, {}, 1.0F})
                    .IsAcceptedUVE());

    const PhysicsConstraintSolveResultUVE solved = constraintSystem.SolveUVE(entityManager, sceneGraph);

    EXPECT_TRUE(solved.islandPlanValid);
    EXPECT_EQ(solved.solvedConstraintCount, 0U);
    EXPECT_EQ(solved.skippedConstraintCount, 1U);
    EXPECT_FLOAT_EQ(WorldPositionUVE(first).x, 0.0F);
    EXPECT_FLOAT_EQ(WorldPositionUVE(second).x, 4.0F);
}

TEST_F(PhysicsConstraintSystemUVETest, AddHingeConstraintUVE_RejectsOverflowedFiniteAxisAtomically) {
    const Scene::EntityUVE first = MakeBodyUVE({});
    const Scene::EntityUVE second = MakeBodyUVE({2.0F, 0.0F, 0.0F});
    const float maximum = std::numeric_limits<float>::max();
    const PhysicsConstraintMutationResultUVE added = constraintSystem.AddHingeConstraintUVE(
        HingeConstraintUVE{first, second, {}, {}, Math::Vector3UVE{maximum, 0.0F, 0.0F}});

    EXPECT_EQ(added.code, PhysicsConstraintCodeUVE::InvalidConstraint);
    EXPECT_EQ(constraintSystem.GetConstraintCountUVE(), 0U);
}

TEST_F(PhysicsConstraintSystemUVETest, SolveHingeUVE_CoincidesAnchorsAndLeavesStaticBodyUnmoved) {
    const Scene::EntityUVE dynamicBody = MakeBodyUVE({0.0F, 0.0F, 0.0F}, 1.0F);
    const Scene::EntityUVE staticBody = MakeStaticUVE({6.0F, 0.0F, 0.0F});
    const Math::Vector3UVE axis{0.0F, 1.0F, 0.0F};

    const PhysicsConstraintMutationResultUVE added = constraintSystem.AddHingeConstraintUVE(
        HingeConstraintUVE{dynamicBody, staticBody, {}, {}, axis});
    ASSERT_TRUE(added.IsAcceptedUVE());

    const PhysicsConstraintSolveResultUVE solved = constraintSystem.SolveUVE(entityManager, sceneGraph);

    EXPECT_TRUE(solved.islandPlanValid);
    EXPECT_EQ(solved.islandCount, 1U);
    EXPECT_GT(solved.solvedConstraintCount, 0U);
    EXPECT_NEAR(WorldPositionUVE(dynamicBody).x, 6.0F, kEpsilon);
    EXPECT_NEAR(WorldPositionUVE(staticBody).x, 6.0F, kEpsilon);
}

TEST_F(PhysicsConstraintSystemUVETest, SolveUVE_ProcessesDisconnectedConstraintIslandsDeterministically) {
    const Scene::EntityUVE first = MakeBodyUVE({0.0F, 0.0F, 0.0F});
    const Scene::EntityUVE second = MakeBodyUVE({10.0F, 0.0F, 0.0F});
    const Scene::EntityUVE third = MakeBodyUVE({20.0F, 0.0F, 0.0F});
    const Scene::EntityUVE fourth = MakeBodyUVE({30.0F, 0.0F, 0.0F});
    ASSERT_TRUE(constraintSystem.AddDistanceConstraintUVE(
                    DistanceConstraintUVE{first, second, {}, {}, 4.0F})
                    .IsAcceptedUVE());
    ASSERT_TRUE(constraintSystem.AddDistanceConstraintUVE(
                    DistanceConstraintUVE{third, fourth, {}, {}, 4.0F})
                    .IsAcceptedUVE());

    const PhysicsConstraintSolveResultUVE solved = constraintSystem.SolveUVE(entityManager, sceneGraph);

    EXPECT_TRUE(solved.islandPlanValid);
    EXPECT_EQ(solved.islandCount, 2U);
    EXPECT_EQ(solved.solvedConstraintCount, 2U);
    EXPECT_NEAR(WorldPositionUVE(first).x, 3.0F, kEpsilon);
    EXPECT_NEAR(WorldPositionUVE(second).x, 7.0F, kEpsilon);
    EXPECT_NEAR(WorldPositionUVE(third).x, 23.0F, kEpsilon);
    EXPECT_NEAR(WorldPositionUVE(fourth).x, 27.0F, kEpsilon);
}

TEST_F(PhysicsConstraintSystemUVETest, RemoveConstraintUVE_RejectsStaleGenerationAfterSlotReuse) {
    const Scene::EntityUVE first = MakeBodyUVE({});
    const Scene::EntityUVE second = MakeBodyUVE({2.0F, 0.0F, 0.0F});
    const DistanceConstraintUVE descriptor{first, second, {}, {}, 1.0F};

    const PhysicsConstraintMutationResultUVE firstAdd = constraintSystem.AddDistanceConstraintUVE(descriptor);
    ASSERT_TRUE(firstAdd.IsAcceptedUVE());
    ASSERT_TRUE(constraintSystem.RemoveConstraintUVE(firstAdd.handle).IsAcceptedUVE());
    const PhysicsConstraintMutationResultUVE secondAdd = constraintSystem.AddDistanceConstraintUVE(descriptor);
    ASSERT_TRUE(secondAdd.IsAcceptedUVE());
    EXPECT_EQ(secondAdd.handle.index, firstAdd.handle.index);
    EXPECT_NE(secondAdd.handle.generation, firstAdd.handle.generation);

    EXPECT_EQ(constraintSystem.RemoveConstraintUVE(firstAdd.handle).code,
              PhysicsConstraintCodeUVE::StaleGeneration);
}

TEST_F(PhysicsConstraintSystemUVETest, AddDistanceConstraintUVE_EnforcesBoundedCapacity) {
    const Scene::EntityUVE first = MakeBodyUVE({});
    const Scene::EntityUVE second = MakeBodyUVE({2.0F, 0.0F, 0.0F});
    const DistanceConstraintUVE descriptor{first, second, {}, {}, 1.0F};

    for (std::size_t index = 0U; index < PhysicsConstraintSystemUVE::kMaximumConstraintsUVE; ++index) {
        EXPECT_TRUE(constraintSystem.AddDistanceConstraintUVE(descriptor).IsAcceptedUVE());
    }
    EXPECT_EQ(constraintSystem.GetConstraintCountUVE(), PhysicsConstraintSystemUVE::kMaximumConstraintsUVE);
    EXPECT_EQ(constraintSystem.AddDistanceConstraintUVE(descriptor).code,
              PhysicsConstraintCodeUVE::CapacityExceeded);
}

TEST_F(PhysicsConstraintSystemUVETest, PhysicsSystemStepUVE_InvokesAttachedConstraintSolver) {
    const Scene::EntityUVE first = MakeBodyUVE({0.0F, 0.0F, 0.0F});
    const Scene::EntityUVE second = MakeBodyUVE({4.0F, 0.0F, 0.0F});
    ASSERT_TRUE(constraintSystem.AddDistanceConstraintUVE(
                    DistanceConstraintUVE{first, second, {}, {}, 2.0F})
                    .IsAcceptedUVE());

    PhysicsSystemUVE physicsSystem(collisionSystem, Math::Vector3UVE{});
    physicsSystem.SetConstraintSystemUVE(&constraintSystem);
    physicsSystem.StepUVE(entityManager, sceneGraph, 0.0F);

    EXPECT_NEAR(WorldPositionUVE(first).x, 1.0F, kEpsilon);
    EXPECT_NEAR(WorldPositionUVE(second).x, 3.0F, kEpsilon);
}

} // namespace
} // namespace UVE::Physics::Tests
