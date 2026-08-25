// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <gtest/gtest.h>

#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/physics/trajectory_collision_prediction_uve.h"
#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/entity_manager_uve.h"
#include "uve/scene/scene_graph_uve.h"

namespace UVE::Physics::Tests {
namespace {

class TrajectoryCollisionPredictionUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    Scene::EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    Scene::SceneGraphUVE sceneGraph;

    Scene::EntityUVE MakeBoxUVE(Math::Vector3UVE position) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE transform;
        transform.localPosition = position;
        sceneGraph.AttachTransformUVE(entityManager, entity, transform);
        sceneGraph.UpdateUVE(entityManager);
        entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(
            entity, Scene::ColliderComponentUVE{Math::Vector3UVE{1.0F, 1.0F, 1.0F}});
        return entity;
    }
};

TEST_F(TrajectoryCollisionPredictionUVETest, PredictsPerSampleCapsuleSweepAgainstSceneCollider) {
    const Scene::EntityUVE obstacle = MakeBoxUVE({0.0F, 0.0F, 0.0F});
    Core::TimeSampledTrajectoryUVE trajectory;
    trajectory.context = Core::AnimationMotionContextUVE::Slide;
    trajectory.samples = {
        {0.25, {2.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, 0.35F, 0.9F},
        {0.5, {5.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, 0.5F, 1.1F},
    };
    TrajectoryCollisionPredictionRequestUVE request;
    request.origin = {-5.0F, 0.0F, 0.0F};
    request.trajectory = trajectory;
    request.defaultCapsuleRadius = 0.25F;
    request.defaultCapsuleHalfHeight = 0.5F;

    const TrajectoryCollisionPredictionResultUVE result =
        PredictTrajectoryCollisionsUVE(entityManager, request);

    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.samples.size(), 2U);
    EXPECT_EQ(result.context, Core::AnimationMotionContextUVE::Slide);
    EXPECT_FALSE(result.samples[0].hit.has_value());
    ASSERT_TRUE(result.samples[1].hit.has_value());
    EXPECT_EQ(result.samples[1].hit->entity, obstacle);
    EXPECT_FLOAT_EQ(result.samples[1].capsuleRadius, 0.5F);
    EXPECT_FLOAT_EQ(result.samples[1].capsuleHalfHeight, 1.1F);
}

TEST_F(TrajectoryCollisionPredictionUVETest, RejectsInvalidRequestAndPreservesOutput) {
    TrajectoryCollisionPredictionResultUVE output;
    output.context = Core::AnimationMotionContextUVE::HeavyLanding;
    output.samples.push_back({});
    TrajectoryCollisionPredictionRequestUVE request;
    request.trajectory.samples.push_back(
        {0.0, {0.0F, 0.0F, 0.0F}, {}, {0.0F, 0.0F, 1.0F}, -1.0F, 0.5F});

    EXPECT_FALSE(PredictTrajectoryCollisionsUVE(entityManager, request, output));
    EXPECT_EQ(output.context, Core::AnimationMotionContextUVE::HeavyLanding);
    EXPECT_EQ(output.samples.size(), 1U);
}

} // namespace
} // namespace UVE::Physics::Tests
