// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <cmath>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include "uve/core/future_trajectory_contract_uve.h"

namespace UVE::Core::Tests {
namespace {

TimeSampledTrajectorySampleUVE MakeSampleUVE(double offsetSeconds) {
    return TimeSampledTrajectorySampleUVE{offsetSeconds,
                                          Math::Vector3UVE{static_cast<float>(offsetSeconds), 0.0F, 0.0F},
                                          Math::Vector3UVE{1.0F, 0.0F, 0.0F},
                                          Math::Vector3UVE{1.0F, 0.0F, 0.0F}, 0.35F, 0.9F};
}

} // namespace

TEST(FutureTrajectoryContractUVETest, BuildsAndPreservesAllAnimationContexts) {
    const std::vector<AnimationMotionContextUVE> contexts{
        AnimationMotionContextUVE::Locomotion, AnimationMotionContextUVE::Turn,
        AnimationMotionContextUVE::Hop, AnimationMotionContextUVE::Slide,
        AnimationMotionContextUVE::Jump, AnimationMotionContextUVE::Fall,
        AnimationMotionContextUVE::LightLanding, AnimationMotionContextUVE::HeavyLanding,
        AnimationMotionContextUVE::Takedown, AnimationMotionContextUVE::Ragdoll,
        AnimationMotionContextUVE::Combat, AnimationMotionContextUVE::Interaction,
        AnimationMotionContextUVE::Custom};
    for (const AnimationMotionContextUVE context : contexts) {
        TimeSampledTrajectoryUVE trajectory;
        ASSERT_TRUE(TryBuildTimeSampledTrajectoryUVE(context, {MakeSampleUVE(0.0), MakeSampleUVE(0.25)}, trajectory));
        EXPECT_EQ(trajectory.context, context);
        EXPECT_EQ(trajectory.samples.size(), 2U);
        EXPECT_FLOAT_EQ(trajectory.samples.front().capsuleRadius, 0.35F);
    }
}

TEST(FutureTrajectoryContractUVETest, RejectsInvalidOrderingShapeAndNonFiniteValues) {
    TimeSampledTrajectoryUVE trajectory;
    trajectory.samples = {MakeSampleUVE(0.25), MakeSampleUVE(0.0)};
    EXPECT_EQ(ValidateTimeSampledTrajectoryUVE(trajectory).code,
              TimeSampledTrajectoryValidationCodeUVE::UnsortedSamples);

    trajectory.samples = {MakeSampleUVE(0.0)};
    trajectory.samples.front().capsuleRadius = 0.0F;
    EXPECT_EQ(ValidateTimeSampledTrajectoryUVE(trajectory).code,
              TimeSampledTrajectoryValidationCodeUVE::InvalidShape);

    trajectory.samples.front().offsetSeconds = std::numeric_limits<double>::quiet_NaN();
    EXPECT_EQ(ValidateTimeSampledTrajectoryUVE(trajectory).code,
              TimeSampledTrajectoryValidationCodeUVE::InvalidTime);
}

TEST(FutureTrajectoryContractUVETest, RejectsCapacityOverflowAndPreservesOutputOnFailure) {
    TimeSampledTrajectoryUVE output;
    output.context = AnimationMotionContextUVE::HeavyLanding;
    output.samples = {MakeSampleUVE(0.0)};
    std::vector<TimeSampledTrajectorySampleUVE> tooMany;
    tooMany.reserve(TimeSampledTrajectoryUVE::kMaximumSamplesUVE + 1U);
    for (std::size_t index = 0U; index <= TimeSampledTrajectoryUVE::kMaximumSamplesUVE; ++index) {
        tooMany.push_back(MakeSampleUVE(static_cast<double>(index)));
    }
    EXPECT_FALSE(TryBuildTimeSampledTrajectoryUVE(AnimationMotionContextUVE::Ragdoll, tooMany, output));
    EXPECT_EQ(output.context, AnimationMotionContextUVE::HeavyLanding);
    EXPECT_EQ(output.samples.size(), 1U);
}

} // namespace UVE::Core::Tests
