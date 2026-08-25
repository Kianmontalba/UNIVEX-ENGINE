// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <gtest/gtest.h>

#include "uve/plugins/motion_query_uve.h"

namespace UVE::Core::Tests {
namespace {

TimeSampledTrajectoryUVE MakeTrajectoryUVE() {
    TimeSampledTrajectoryUVE trajectory;
    trajectory.context = AnimationMotionContextUVE::Takedown;
    trajectory.samples = {
        TimeSampledTrajectorySampleUVE{0.0, {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F}, 0.4F, 0.95F},
        TimeSampledTrajectorySampleUVE{0.25, {0.5F, 0.0F, 0.0F}, {2.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, 0.38F, 0.9F},
    };
    return trajectory;
}

} // namespace

TEST(MotionQuerySharedTrajectoryUVETest, BuilderPreservesSharedContextAndCollisionShapeSamples) {
    const TransformPoseUVE previous{{0.0F, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}};
    const TransformPoseUVE current{{0.1F, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}};
    const TimeSampledTrajectoryUVE sharedTrajectory = MakeTrajectoryUVE();
    MotionQueryUVE query;

    ASSERT_TRUE(TryBuildMotionQueryUVE(previous, current, 0.1, sharedTrajectory, query));
    EXPECT_EQ(query.trajectory.context, AnimationMotionContextUVE::Takedown);
    ASSERT_EQ(query.trajectory.samples.size(), 2U);
    EXPECT_FLOAT_EQ(query.trajectory.samples[1].capsuleRadius, 0.38F);
    EXPECT_FLOAT_EQ(query.trajectory.samples[1].capsuleHalfHeight, 0.9F);
}

TEST(MotionQuerySharedTrajectoryUVETest, SharedTrajectoryStillUsesExistingSchemaValidation) {
    MotionQueryUVE query;
    query.rootVelocity = {1.0F, 0.0F, 0.0F};
    query.trajectory = MakeTrajectoryUVE();
    query.skeleton = SkeletonDefinitionUVE{"shared", {{"root", ""}}};
    query.pose = PoseBufferUVE{"shared", {{{}, {}, {1.0F, 1.0F, 1.0F}}}};
    query.evaluationContext.sampleTimeSeconds = 0.0;
    EXPECT_TRUE(ValidateMotionQueryUVE(query).IsValidUVE());
}

} // namespace UVE::Core::Tests
