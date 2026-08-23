// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_continuity_uve.h"

#include <gtest/gtest.h>

#include <limits>

namespace UVE::Plugins {
namespace {
UVE::Core::TransformPoseUVE MakeCurrentPoseUVE() {
    UVE::Core::TransformPoseUVE pose;
    pose.position = UVE::Math::Vector3UVE{2.0F, 4.0F, 6.0F};
    pose.rotation = UVE::Math::QuaternionUVE{0.1F, 0.2F, 0.3F, 0.9F};
    pose.scale = UVE::Math::Vector3UVE{2.0F, 4.0F, 6.0F};
    return pose;
}

UVE::Core::PoseSampleUVE MakePreviousSampleUVE(const double timeSeconds) {
    UVE::Core::PoseSampleUVE sample;
    sample.timeSeconds = timeSeconds;
    sample.pose.position = UVE::Math::Vector3UVE{0.0F, 2.0F, 4.0F};
    sample.pose.rotation = UVE::Math::QuaternionUVE{0.9F, 0.3F, 0.2F, 0.1F};
    sample.pose.scale = UVE::Math::Vector3UVE{1.0F, 2.0F, 3.0F};
    return sample;
}
} // namespace

TEST(MotionQueryContinuityUVETest, DisabledUVE_PreservesCurrentPose) {
    const auto current = MakeCurrentPoseUVE();
    const auto result = ApplyMotionQueryContinuityUVE(
        current, nullptr, 1.0, MotionQueryContinuitySettingsUVE{});
    EXPECT_EQ(result.code, MotionQueryContinuityCodeUVE::Disabled);
    EXPECT_TRUE(result.IsAcceptedUVE());
    EXPECT_EQ(result.pose, current);
    EXPECT_FALSE(result.WasAppliedUVE());
}

TEST(MotionQueryContinuityUVETest, BlendUVE_AveragesTranslationAndScaleButKeepsCurrentRotation) {
    const auto current = MakeCurrentPoseUVE();
    const auto previous = MakePreviousSampleUVE(0.9);
    MotionQueryContinuitySettingsUVE settings;
    settings.policy = MotionQueryContinuityPolicyUVE::BlendPreviousWithinWindow;
    settings.maximumPreviousAgeSeconds = 0.2;
    const auto result = ApplyMotionQueryContinuityUVE(current, &previous, 1.0, settings);
    EXPECT_EQ(result.code, MotionQueryContinuityCodeUVE::Applied);
    EXPECT_TRUE(result.WasAppliedUVE());
    EXPECT_DOUBLE_EQ(result.previousAgeSeconds, 0.1);
    EXPECT_EQ(result.pose.position, (UVE::Math::Vector3UVE{1.0F, 3.0F, 5.0F}));
    EXPECT_EQ(result.pose.scale, (UVE::Math::Vector3UVE{1.5F, 3.0F, 4.5F}));
    EXPECT_EQ(result.pose.rotation, current.rotation);
}

TEST(MotionQueryContinuityUVETest, BlendUVE_PreservesFiniteExtremePositionAndScale) {
    const float maximumValue = std::numeric_limits<float>::max();
    UVE::Core::TransformPoseUVE current = MakeCurrentPoseUVE();
    current.position = UVE::Math::Vector3UVE{maximumValue, maximumValue, maximumValue};
    current.scale = UVE::Math::Vector3UVE{maximumValue, maximumValue, maximumValue};
    UVE::Core::PoseSampleUVE previous = MakePreviousSampleUVE(0.9);
    previous.pose.position = UVE::Math::Vector3UVE{maximumValue, maximumValue, maximumValue};
    previous.pose.scale = UVE::Math::Vector3UVE{maximumValue, maximumValue, maximumValue};

    MotionQueryContinuitySettingsUVE settings;
    settings.policy = MotionQueryContinuityPolicyUVE::BlendPreviousWithinWindow;
    settings.maximumPreviousAgeSeconds = 0.2;
    const auto result = ApplyMotionQueryContinuityUVE(current, &previous, 1.0, settings);
    ASSERT_EQ(result.code, MotionQueryContinuityCodeUVE::Applied);
    EXPECT_TRUE(result.IsAcceptedUVE());
    EXPECT_EQ(result.pose.position, current.position);
    EXPECT_EQ(result.pose.scale, current.scale);
    EXPECT_EQ(result.pose.rotation, current.rotation);
}

TEST(MotionQueryContinuityUVETest, BlendUVE_ReportsNoPreviousOrStaleWithoutChangingPose) {
    const auto current = MakeCurrentPoseUVE();
    MotionQueryContinuitySettingsUVE settings;
    settings.policy = MotionQueryContinuityPolicyUVE::BlendPreviousWithinWindow;
    settings.maximumPreviousAgeSeconds = 0.1;
    const auto noPrevious = ApplyMotionQueryContinuityUVE(current, nullptr, 1.0, settings);
    EXPECT_EQ(noPrevious.code, MotionQueryContinuityCodeUVE::NoPreviousSample);
    EXPECT_EQ(noPrevious.pose, current);
    const auto previous = MakePreviousSampleUVE(0.5);
    const auto stale = ApplyMotionQueryContinuityUVE(current, &previous, 1.0, settings);
    EXPECT_EQ(stale.code, MotionQueryContinuityCodeUVE::PreviousSampleTooOld);
    EXPECT_EQ(stale.pose, current);
    EXPECT_DOUBLE_EQ(stale.previousAgeSeconds, 0.5);
}

TEST(MotionQueryContinuityUVETest, InvalidSettingsUVE_RejectsWithoutChangingPose) {
    const auto current = MakeCurrentPoseUVE();
    MotionQueryContinuitySettingsUVE settings;
    settings.policy = MotionQueryContinuityPolicyUVE::BlendPreviousWithinWindow;
    settings.maximumPreviousAgeSeconds = -1.0;
    const auto result = ApplyMotionQueryContinuityUVE(
        current, nullptr, 1.0, settings);
    EXPECT_EQ(result.code, MotionQueryContinuityCodeUVE::InvalidSettings);
    EXPECT_FALSE(result.IsAcceptedUVE());
    EXPECT_EQ(result.pose, current);
}
} // namespace UVE::Plugins
