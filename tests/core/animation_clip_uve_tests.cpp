// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/core/animation_clip_uve.h"

#include <gtest/gtest.h>

namespace UVE::Core {
namespace {

AnimationClipUVE MakeClipUVE() {
    AnimationClipUVE clip;
    clip.clipId = "walk";
    clip.durationSeconds = 1.0;
    clip.samples = {
        PoseSampleUVE{0.0, TransformPoseUVE{{0.0F, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}}},
        PoseSampleUVE{1.0, TransformPoseUVE{{10.0F, 2.0F, 0.0F}, {}, {2.0F, 2.0F, 2.0F}}},
    };
    clip.events = {AnimationEventUVE{0.2, "step_left"}, AnimationEventUVE{0.8, "step_right"}};
    return clip;
}

} // namespace

TEST(AnimationClipUVETest, ValidateAnimationClipUVE_AcceptsBoundedSortedClip) {
    const AnimationClipUVE clip = MakeClipUVE();
    const AnimationClipValidationResultUVE result = ValidateAnimationClipUVE(clip);

    EXPECT_TRUE(result.IsValidUVE());
    EXPECT_EQ(result.code, AnimationClipValidationCodeUVE::Valid);
}

TEST(AnimationClipUVETest, TrySampleAnimationClipUVE_InterpolatesAndLoopsDeterministically) {
    const AnimationClipUVE clip = MakeClipUVE();
    TransformPoseUVE midpoint;
    TransformPoseUVE looped;

    ASSERT_TRUE(TrySampleAnimationClipUVE(clip, 0.5, false, midpoint));
    EXPECT_EQ(midpoint.position, (Math::Vector3UVE{5.0F, 1.0F, 0.0F}));
    EXPECT_EQ(midpoint.scale, (Math::Vector3UVE{1.5F, 1.5F, 1.5F}));
    ASSERT_TRUE(TrySampleAnimationClipUVE(clip, 1.25, true, looped));
    EXPECT_EQ(looped.position, (Math::Vector3UVE{2.5F, 0.5F, 0.0F}));
}

TEST(AnimationClipUVETest, CollectAnimationEventsUVE_ReturnsEventsAcrossLoopBoundaryInOrder) {
    const AnimationClipUVE clip = MakeClipUVE();
    const std::vector<AnimationEventUVE> events = CollectAnimationEventsUVE(clip, 0.7, 1.2, true);

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(events[0].eventId, "step_right");
    EXPECT_EQ(events[1].eventId, "step_left");
}

TEST(AnimationClipUVETest, ValidateAnimationClipUVE_RejectsUnsortedOrInvalidData) {
    AnimationClipUVE clip = MakeClipUVE();
    clip.samples[1].timeSeconds = -0.1;
    EXPECT_EQ(ValidateAnimationClipUVE(clip).code, AnimationClipValidationCodeUVE::InvalidSampleTime);

    clip = MakeClipUVE();
    clip.events[0].eventId.clear();
    EXPECT_EQ(ValidateAnimationClipUVE(clip).code, AnimationClipValidationCodeUVE::InvalidEventIdentifier);

    clip = MakeClipUVE();
    clip.samples[1].pose.rotation = Math::QuaternionUVE{0.0F, 0.0F, 0.0F, 0.0F};
    EXPECT_EQ(ValidateAnimationClipUVE(clip).code, AnimationClipValidationCodeUVE::InvalidPose);
}

} // namespace UVE::Core
