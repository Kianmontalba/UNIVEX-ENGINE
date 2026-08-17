// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/plugins/control_rig_uve.h"

#include <gtest/gtest.h>

namespace UVE::Core {
namespace {

ControlRigUVE MakeRigUVE() {
    ControlRigUVE rig;
    rig.controls = {
        ControlRigControlUVE{"root", {}, ControlRigSpaceUVE::World,
                             TransformPoseUVE{{0.0F, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}}, true},
        ControlRigControlUVE{"mid", "root", ControlRigSpaceUVE::World,
                             TransformPoseUVE{{1.0F, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}}, true},
        ControlRigControlUVE{"end", "mid", ControlRigSpaceUVE::World,
                             TransformPoseUVE{{2.0F, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}}, true},
        ControlRigControlUVE{"target", {}, ControlRigSpaceUVE::World,
                             TransformPoseUVE{{1.0F, 1.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}}, true},
        ControlRigControlUVE{"pole", {}, ControlRigSpaceUVE::World,
                             TransformPoseUVE{{0.0F, 1.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}}, true},
    };
    rig.constraints = {
        ControlRigConstraintUVE{"arm_ik", ControlRigConstraintKindUVE::TwoBoneIK,
                                "root", "mid", "end", "target", "pole", 1.0F},
    };
    rig.skeleton = SkeletonDefinitionUVE{
        "arm", {SkeletonJointUVE{"root", ""}, SkeletonJointUVE{"mid", "root"},
                 SkeletonJointUVE{"end", "mid"}}};
    rig.pose = PoseBufferUVE{
        "arm", {rig.controls[0].pose, rig.controls[1].pose, rig.controls[2].pose}};
    rig.evaluationContext.time.animationTimeSeconds = 0.5;
    rig.evaluationContext.time.animationDeltaSeconds = 1.0 / 60.0;
    rig.evaluationContext.sampleTimeSeconds = 0.5;
    return rig;
}

} // namespace

TEST(ControlRigUVETest, ValidateControlRigUVE_AcceptsControlsSpacesAndTwoBoneConstraint) {
    const ControlRigValidationResultUVE result = ValidateControlRigUVE(MakeRigUVE());
    EXPECT_TRUE(result.IsValidUVE());
    EXPECT_EQ(result.code, ControlRigValidationCodeUVE::Valid);
}

TEST(ControlRigUVETest, SolveTwoBoneIKUVE_ReachesTargetWithPoleVectorDeterministically) {
    const ControlRigUVE rig = MakeRigUVE();
    const TransformPoseUVE& root = rig.controls[0].pose;
    const TransformPoseUVE& mid = rig.controls[1].pose;
    const TransformPoseUVE& end = rig.controls[2].pose;
    const TwoBoneIKSolveResultUVE result = SolveTwoBoneIKUVE(
        root, mid, end, rig.controls[3].pose.position, rig.controls[4].pose.position, 1.0F);

    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_TRUE(result.reachable);
    EXPECT_FALSE(result.targetClamped);
    EXPECT_NEAR(result.midPose.position.x, 0.0F, 1.0e-4F);
    EXPECT_NEAR(result.midPose.position.y, 1.0F, 1.0e-4F);
    EXPECT_NEAR(result.endPose.position.x, 1.0F, 1.0e-4F);
    EXPECT_NEAR(result.endPose.position.y, 1.0F, 1.0e-4F);
}

TEST(ControlRigUVETest, TryMakeAimLookAtRotationUVE_AlignsForwardAxis) {
    Math::QuaternionUVE rotation;
    ASSERT_TRUE(TryMakeAimLookAtRotationUVE(
        {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, -1.0F}, {0.0F, 1.0F, 0.0F}, rotation));
    EXPECT_EQ(rotation, (Math::QuaternionUVE{0.0F, 0.0F, 0.0F, 1.0F}));
}

TEST(ControlRigUVETest, EvaluateControlRigUVE_ReturnsCopiedControlsWithAppliedConstraint) {
    const ControlRigEvaluationResultUVE result = EvaluateControlRigUVE(MakeRigUVE());

    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.controls.size(), 5U);
    EXPECT_EQ(result.appliedConstraintCount, 1U);
    EXPECT_NEAR(result.controls[1].pose.position.y, 1.0F, 1.0e-4F);
    EXPECT_NEAR(result.controls[2].pose.position.x, 1.0F, 1.0e-4F);
    EXPECT_EQ(result.skeleton.skeletonId, "arm");
    EXPECT_EQ(result.pose.skeletonId, "arm");
    EXPECT_DOUBLE_EQ(result.evaluationContext.sampleTimeSeconds, 0.5);
}

TEST(ControlRigUVETest, ValidateControlRigUVE_RejectsSharedPoseMismatch) {
    ControlRigUVE rig = MakeRigUVE();
    rig.pose.skeletonId = "other";
    EXPECT_EQ(ValidateControlRigUVE(rig).code, ControlRigValidationCodeUVE::InvalidPose);

    rig = MakeRigUVE();
    rig.evaluationContext.time.animationDeltaSeconds = -0.1;
    EXPECT_EQ(ValidateControlRigUVE(rig).code, ControlRigValidationCodeUVE::InvalidEvaluationTime);
}

TEST(ControlRigUVETest, ValidateControlRigUVE_RejectsUnknownParentAndInvalidWeight) {
    ControlRigUVE unknownParent = MakeRigUVE();
    unknownParent.controls[1].parentControlId = "missing";
    EXPECT_EQ(ValidateControlRigUVE(unknownParent).code, ControlRigValidationCodeUVE::UnknownControl);

    ControlRigUVE invalidWeight = MakeRigUVE();
    invalidWeight.constraints[0].weight = 2.0F;
    EXPECT_EQ(ValidateControlRigUVE(invalidWeight).code, ControlRigValidationCodeUVE::InvalidConstraint);
}

} // namespace UVE::Core
