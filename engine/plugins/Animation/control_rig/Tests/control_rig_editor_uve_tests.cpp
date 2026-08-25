// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/plugins/control_rig_editor_uve.h"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

namespace UVE::Core {
namespace {

ControlRigAutoRigRequestUVE MakeRequestUVE() {
    ControlRigAutoRigRequestUVE request;
    request.rigId = "editor_autorig";
    request.skeleton = SkeletonDefinitionUVE{
        "humanoid",
        {{"root", ""}, {"spine", "root"}, {"upper_arm.L", "spine"},
         {"forearm.L", "upper_arm.L"}, {"hand.L", "forearm.L"},
         {"upper_arm.R", "spine"}, {"forearm.R", "upper_arm.R"}, {"hand.R", "forearm.R"},
         {"thigh.L", "root"}, {"shin.L", "thigh.L"}, {"foot.L", "shin.L"},
         {"thigh.R", "root"}, {"shin.R", "thigh.R"}, {"foot.R", "shin.R"}, {"head", "spine"}}};
    request.referencePose.skeletonId = request.skeleton.skeletonId;
    for (std::size_t index = 0U; index < request.skeleton.joints.size(); ++index) {
        request.referencePose.localJoints.push_back(
            TransformPoseUVE{{0.0F, static_cast<float>(index), 0.0F}, {}, {1.0F, 1.0F, 1.0F}});
    }
    return request;
}

} // namespace

TEST(ControlRigEditorUVETest, InitializeAndSnapshotExposeAnimatorSelectableControlFacts) {
    ControlRigEditorAuthoringSessionUVE session;
    ASSERT_TRUE(session.InitializeUVE(MakeRequestUVE()));
    EXPECT_TRUE(session.IsInitializedUVE());
    EXPECT_TRUE(session.ValidateUVE().IsValidUVE());

    const ControlRigAuthoringSnapshotUVE snapshot = session.CaptureSnapshotUVE();
    EXPECT_EQ(snapshot.rigId, "editor_autorig");
    EXPECT_EQ(snapshot.revision, 1U);
    EXPECT_EQ(snapshot.tool, ControlRigEditorToolUVE::Select);
    EXPECT_FALSE(snapshot.dirty);
    EXPECT_FALSE(snapshot.viewportControls.empty());
}

TEST(ControlRigEditorUVETest, TranslateAndRotateRequireMatchingAnimatorTool) {
    ControlRigEditorAuthoringSessionUVE session;
    ASSERT_TRUE(session.InitializeUVE(MakeRequestUVE()));
    EXPECT_FALSE(session.TranslateSelectedControlUVE({1.0F, 0.0F, 0.0F}));
    ASSERT_TRUE(session.SelectControlUVE("ctrl_hand_ik_l"));
    ASSERT_TRUE(session.SetToolUVE(ControlRigEditorToolUVE::Translate));
    ASSERT_TRUE(session.TranslateSelectedControlUVE({1.0F, 2.0F, 3.0F}));
    EXPECT_TRUE(session.CaptureSnapshotUVE().dirty);

    ASSERT_TRUE(session.SetToolUVE(ControlRigEditorToolUVE::Rotate));
    ASSERT_TRUE(session.RotateSelectedControlUVE({0.0F, 0.0F, 0.0F, 1.0F}));
    EXPECT_TRUE(session.ValidateUVE().IsValidUVE());
}

TEST(ControlRigEditorUVETest, ResetAndMirrorClearSelectionAndRestoreAuthoringState) {
    ControlRigEditorAuthoringSessionUVE session;
    ASSERT_TRUE(session.InitializeUVE(MakeRequestUVE()));
    ASSERT_TRUE(session.SelectControlUVE("ctrl_hand_ik_l"));
    ASSERT_TRUE(session.SetToolUVE(ControlRigEditorToolUVE::Translate));
    ASSERT_TRUE(session.TranslateSelectedControlUVE({4.0F, 0.0F, 0.0F}));
    ASSERT_TRUE(session.MirrorControlsUVE());
    EXPECT_TRUE(session.CaptureSnapshotUVE().selectedControlId.empty());
    EXPECT_TRUE(session.CaptureSnapshotUVE().dirty);
    ASSERT_TRUE(session.ResetControlsUVE());
    EXPECT_FALSE(session.CaptureSnapshotUVE().dirty);
    EXPECT_TRUE(session.CaptureSnapshotUVE().viewportControls.size() > 0U);
}

TEST(ControlRigEditorUVETest, BakeCapturesStrictlyChronologicalSkeletonMatchedSamples) {
    ControlRigEditorAuthoringSessionUVE session;
    ASSERT_TRUE(session.InitializeUVE(MakeRequestUVE()));
    ASSERT_TRUE(session.CaptureBakeSampleUVE(0.0));
    ASSERT_TRUE(session.CaptureBakeSampleUVE(1.0 / 30.0));
    EXPECT_FALSE(session.CaptureBakeSampleUVE(1.0 / 30.0));
    EXPECT_FALSE(session.CaptureBakeSampleUVE(-1.0));

    const ControlRigEditorBakedAnimationResultUVE baked = session.BakeAnimationUVE("walk_cycle");
    ASSERT_TRUE(baked.IsSuccessUVE());
    EXPECT_EQ(baked.animation.animationId, "walk_cycle");
    EXPECT_EQ(baked.animation.skeleton.skeletonId, "humanoid");
    EXPECT_EQ(baked.animation.samples.size(), 2U);
    EXPECT_LT(baked.animation.samples[0].timeSeconds, baked.animation.samples[1].timeSeconds);
}

TEST(ControlRigEditorUVETest, InvalidSessionAndNonFiniteTransformsFailClosed) {
    ControlRigEditorAuthoringSessionUVE session;
    EXPECT_FALSE(session.SelectControlUVE("ctrl_root"));
    EXPECT_EQ(session.ValidateUVE().code, ControlRigEditorValidationCodeUVE::Uninitialized);
    EXPECT_FALSE(session.BakeAnimationUVE("empty").IsSuccessUVE());

    ASSERT_TRUE(session.InitializeUVE(MakeRequestUVE()));
    ASSERT_TRUE(session.SelectControlUVE("ctrl_hand_ik_l"));
    ASSERT_TRUE(session.SetToolUVE(ControlRigEditorToolUVE::Translate));
    EXPECT_FALSE(session.TranslateSelectedControlUVE(
        {std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F}));
}

} // namespace UVE::Core
