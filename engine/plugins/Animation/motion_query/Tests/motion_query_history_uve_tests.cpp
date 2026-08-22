// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/plugins/motion_query_history_uve.h"

#include <gtest/gtest.h>

#include <utility>

namespace UVE::Core {
namespace {

TransformPoseUVE MakePoseUVE(float x, float y = 0.0F, float z = 0.0F) {
    return TransformPoseUVE{{x, y, z}, {}, {1.0F, 1.0F, 1.0F}};
}

MotionQueryHistoryFrameUVE MakeFrameUVE(double timeSeconds, float x) {
    MotionQueryHistoryFrameUVE frame;
    frame.sample.timeSeconds = timeSeconds;
    frame.sample.pose = MakePoseUVE(x, 2.0F, 3.0F);
    frame.query.rootVelocity = {x, 0.0F, 0.0F};
    frame.query.facingDirection = {0.0F, 0.0F, 1.0F};
    frame.query.trajectory = {MotionTrajectorySampleUVE{0.25, {x, 0.0F, 0.0F}}};
    frame.attributes = {MotionQueryHistoryAttributeUVE{"speed", MotionQueryHistoryAttributeKindUVE::RootSpeed, x}};
    return frame;
}

} // namespace

TEST(MotionQueryHistoryUVETest, AppendFrameAndProviderUVE_SelectsLatestAtOrBeforeSample) {
    MotionQueryHistoryBufferUVE history;
    ASSERT_TRUE(history.AppendFrameUVE(MakeFrameUVE(0.0, 0.0F)).IsAcceptedUVE());
    ASSERT_TRUE(history.AppendFrameUVE(MakeFrameUVE(0.5, 1.0F)).IsAcceptedUVE());
    ASSERT_TRUE(history.AppendFrameUVE(MakeFrameUVE(1.0, 2.0F)).IsAcceptedUVE());

    PoseSampleUVE sample;
    ASSERT_TRUE(history.TryGetPoseAtOrBeforeUVE(0.75, sample));
    EXPECT_DOUBLE_EQ(sample.timeSeconds, 0.5);
    EXPECT_FLOAT_EQ(sample.pose.position.x, 1.0F);
    EXPECT_FALSE(history.TryGetPoseAtOrBeforeUVE(-1.0, sample));
    EXPECT_TRUE(history.TryGetPoseAtOrBeforeUVE(0.0, sample));
}

TEST(MotionQueryHistoryUVETest, AppendFrameUVE_RejectsNonMonotonicAndDuplicateAttributes) {
    MotionQueryHistoryBufferUVE history;
    ASSERT_TRUE(history.AppendFrameUVE(MakeFrameUVE(1.0, 1.0F)).IsAcceptedUVE());
    EXPECT_EQ(history.AppendFrameUVE(MakeFrameUVE(0.5, 0.5F)).code,
              MotionQueryHistoryResultCodeUVE::NonMonotonicTime);

    MotionQueryHistoryFrameUVE duplicate = MakeFrameUVE(2.0, 2.0F);
    duplicate.attributes.push_back(duplicate.attributes.front());
    EXPECT_EQ(history.AppendFrameUVE(std::move(duplicate)).code,
              MotionQueryHistoryResultCodeUVE::InvalidFrame);
}

TEST(MotionQueryHistoryUVETest, AppendNotifyUVE_AssignsContiguousSequenceAndRejectsInvalidOrder) {
    MotionQueryHistoryBufferUVE history;
    ASSERT_TRUE(history.AppendNotifyUVE(
        MotionQueryHistoryNotifyUVE{"footstep", 0.25, 0U, "left"}).IsAcceptedUVE());
    ASSERT_EQ(history.GetNotifiesUVE().front().sequence, 1U);

    EXPECT_EQ(history.AppendNotifyUVE(
                  MotionQueryHistoryNotifyUVE{"footstep", 0.1, 0U, "right"})
                  .code,
              MotionQueryHistoryResultCodeUVE::NonMonotonicTime);
    EXPECT_EQ(history.AppendNotifyUVE(
                  MotionQueryHistoryNotifyUVE{"footstep", 0.5, 3U, "right"})
                  .code,
              MotionQueryHistoryResultCodeUVE::NonMonotonicSequence);
}

TEST(MotionQueryHistoryUVETest, MirrorFrameUVE_MirrorsSelectedAxisWithoutOwningOriginal) {
    const MotionQueryHistoryFrameUVE original = MakeFrameUVE(1.0, 2.0F);
    MotionQueryHistoryFrameUVE mirrored;
    ASSERT_TRUE(TryMirrorMotionQueryHistoryFrameUVE(
        original, MotionQueryMirrorSettingsUVE{MotionQueryMirrorAxisUVE::X}, mirrored));

    EXPECT_FLOAT_EQ(original.sample.pose.position.x, 2.0F);
    EXPECT_FLOAT_EQ(mirrored.sample.pose.position.x, -2.0F);
    EXPECT_FLOAT_EQ(mirrored.query.rootVelocity.x, -2.0F);
    EXPECT_FLOAT_EQ(mirrored.query.trajectory.front().relativePosition.x, -2.0F);
}

TEST(MotionQueryHistoryUVETest, MirrorFrameUVE_UnknownAxisRejectsAndPreservesOutput) {
    const MotionQueryHistoryFrameUVE original = MakeFrameUVE(1.0, 2.0F);
    MotionQueryHistoryFrameUVE output = MakeFrameUVE(3.0, 4.0F);
    const MotionQueryHistoryFrameUVE before = output;
    EXPECT_FALSE(TryMirrorMotionQueryHistoryFrameUVE(
        original, MotionQueryMirrorSettingsUVE{static_cast<MotionQueryMirrorAxisUVE>(0xFFU)}, output));
    EXPECT_DOUBLE_EQ(output.sample.timeSeconds, before.sample.timeSeconds);
    EXPECT_FLOAT_EQ(output.sample.pose.position.x, before.sample.pose.position.x);
    EXPECT_FLOAT_EQ(output.query.rootVelocity.x, before.query.rootVelocity.x);
    EXPECT_EQ(output.attributes.size(), before.attributes.size());
}

TEST(MotionQueryHistoryUVETest, ClearUVE_RemovesFramesAndNotifies) {
    MotionQueryHistoryBufferUVE history;
    ASSERT_TRUE(history.AppendFrameUVE(MakeFrameUVE(0.0, 0.0F)).IsAcceptedUVE());
    ASSERT_TRUE(history.AppendNotifyUVE(
        MotionQueryHistoryNotifyUVE{"marker", 0.0, 0U, "start"}).IsAcceptedUVE());
    history.ClearUVE();
    EXPECT_TRUE(history.GetFramesUVE().empty());
    EXPECT_TRUE(history.GetNotifiesUVE().empty());
}

} // namespace UVE::Core
