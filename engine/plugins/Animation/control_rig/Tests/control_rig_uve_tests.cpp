// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/plugins/control_rig_uve.h"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

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

TEST(ControlRigUVETest, ValidateControlRigUVE_RejectsUnknownConstraintKind) {
    ControlRigUVE rig = MakeRigUVE();
    rig.constraints[0].kind = static_cast<ControlRigConstraintKindUVE>(0xFFU);
    EXPECT_EQ(ValidateControlRigUVE(rig).code, ControlRigValidationCodeUVE::InvalidConstraint);

    const ControlRigEvaluationResultUVE result = EvaluateControlRigUVE(rig);
    EXPECT_FALSE(result.IsSuccessUVE());
    EXPECT_EQ(result.appliedConstraintCount, 0U);
}

TEST(ControlRigUVETest, ValidateControlRigUVE_RejectsUnknownControlSpace) {
    ControlRigUVE rig = MakeRigUVE();
    rig.controls[0].space = static_cast<ControlRigSpaceUVE>(0xFFU);
    EXPECT_EQ(ValidateControlRigUVE(rig).code, ControlRigValidationCodeUVE::InvalidControl);

    const ControlRigEvaluationResultUVE result = EvaluateControlRigUVE(rig);
    EXPECT_FALSE(result.IsSuccessUVE());
    EXPECT_EQ(result.appliedConstraintCount, 0U);
}

TEST(ControlRigUVETest, BlendControlRigPoseUVE_ClampsWeightAndNormalizesRotation) {
    const TransformPoseUVE source{{0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F, 1.0F},
                                  {1.0F, 1.0F, 1.0F}};
    const TransformPoseUVE target{{10.0F, 4.0F, 0.0F}, {0.0F, 1.0F, 0.0F, 0.0F},
                                  {3.0F, 5.0F, 7.0F}};

    const TransformPoseUVE half = BlendControlRigPoseUVE(source, target, 0.5F);
    EXPECT_NEAR(half.position.x, 5.0F, 1.0e-4F);
    EXPECT_NEAR(half.position.y, 2.0F, 1.0e-4F);
    EXPECT_NEAR(half.scale.x, 2.0F, 1.0e-4F);
    EXPECT_NEAR(half.scale.y, 3.0F, 1.0e-4F);
    EXPECT_NEAR(half.scale.z, 4.0F, 1.0e-4F);
    EXPECT_NEAR(Math::LengthSquaredUVE(half.rotation), 1.0F, 1.0e-4F);

    EXPECT_EQ(BlendControlRigPoseUVE(source, target, -1.0F), source);
    EXPECT_EQ(BlendControlRigPoseUVE(source, target, 2.0F), target);
    EXPECT_EQ(BlendControlRigPoseUVE(source, target,
                                     std::numeric_limits<float>::quiet_NaN()), source);
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

TEST(ControlRigUVETest, SolveTwoBoneIKUVE_PreservesFiniteExtremeGeometry) {
    const float maximum = std::numeric_limits<float>::max();
    const TransformPoseUVE root{{-maximum, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}};
    const TransformPoseUVE mid{{0.0F, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}};
    const TransformPoseUVE end{{maximum, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}};
    const TwoBoneIKSolveResultUVE result = SolveTwoBoneIKUVE(
        root, mid, end, {0.0F, 1.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, 1.0F);

    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_TRUE(result.reachable);
    EXPECT_FALSE(result.targetClamped);
    EXPECT_TRUE(std::isfinite(result.midPose.position.x));
    EXPECT_TRUE(std::isfinite(result.midPose.position.y));
    EXPECT_TRUE(std::isfinite(result.endPose.position.x));
    EXPECT_TRUE(std::isfinite(result.endPose.position.y));
    EXPECT_NEAR(result.endPose.position.x, 0.0F, maximum * 1.0e-6F);
}

TEST(ControlRigUVETest, SolveTwoBoneIKUVE_RejectsOverflowedPoleOffset) {
    const float maximum = std::numeric_limits<float>::max();
    const TransformPoseUVE root{{-maximum, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}};
    const TransformPoseUVE mid{{-maximum * 0.5F, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}};
    const TransformPoseUVE end{{0.0F, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}};
    const TwoBoneIKSolveResultUVE result = SolveTwoBoneIKUVE(
        root, mid, end, {0.0F, 0.0F, 0.0F}, {maximum, 0.0F, 0.0F}, 1.0F);

    EXPECT_FALSE(result.IsSuccessUVE());
    EXPECT_EQ(result.rootPose, root);
    EXPECT_EQ(result.midPose, mid);
    EXPECT_EQ(result.endPose, end);
}

TEST(ControlRigUVETest, SolveTwoBoneIKUVE_PreservesFiniteExtremePoleDirection) {
    const float maximum = std::numeric_limits<float>::max();
    const TransformPoseUVE root{{0.0F, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}};
    const TransformPoseUVE mid{{1.0F, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}};
    const TransformPoseUVE end{{2.0F, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}};
    const TwoBoneIKSolveResultUVE result = SolveTwoBoneIKUVE(
        root, mid, end, {1.0F, 1.0F, 0.0F}, {maximum * 0.5F, maximum, 0.0F}, 1.0F);

    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_TRUE(result.reachable);
    EXPECT_FALSE(result.targetClamped);
    EXPECT_NEAR(result.midPose.position.x, 0.0F, 1.0e-4F);
    EXPECT_NEAR(result.midPose.position.y, 1.0F, 1.0e-4F);
}

TEST(ControlRigUVETest, SolveTwoBoneIKUVE_AlignsRootAndMidRotationsToSolvedBones) {
    const ControlRigUVE rig = MakeRigUVE();
    const TwoBoneIKSolveResultUVE result = SolveTwoBoneIKUVE(
        rig.controls[0].pose, rig.controls[1].pose, rig.controls[2].pose,
        rig.controls[3].pose.position, rig.controls[4].pose.position, 1.0F);

    ASSERT_TRUE(result.IsSuccessUVE());
    const Math::Vector3UVE rootDirection = Math::NormalizeUVE(
        result.midPose.position - result.rootPose.position);
    const Math::Vector3UVE midDirection = Math::NormalizeUVE(
        result.endPose.position - result.midPose.position);
    const Math::Vector3UVE rootAxis = Math::RotateVectorUVE(
        result.rootPose.rotation, {1.0F, 0.0F, 0.0F});
    const Math::Vector3UVE midAxis = Math::RotateVectorUVE(
        result.midPose.rotation, {1.0F, 0.0F, 0.0F});
    EXPECT_NEAR(rootAxis.x, rootDirection.x, 1.0e-4F);
    EXPECT_NEAR(rootAxis.y, rootDirection.y, 1.0e-4F);
    EXPECT_NEAR(rootAxis.z, rootDirection.z, 1.0e-4F);
    EXPECT_NEAR(midAxis.x, midDirection.x, 1.0e-4F);
    EXPECT_NEAR(midAxis.y, midDirection.y, 1.0e-4F);
    EXPECT_NEAR(midAxis.z, midDirection.z, 1.0e-4F);
}

TEST(ControlRigUVETest, SolveTwoBoneIKUVE_UsesBoundedWeightedPoseBlend) {
    const ControlRigUVE rig = MakeRigUVE();
    const TwoBoneIKSolveResultUVE result = SolveTwoBoneIKUVE(
        rig.controls[0].pose, rig.controls[1].pose, rig.controls[2].pose,
        rig.controls[3].pose.position, rig.controls[4].pose.position, 0.5F);

    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_NEAR(result.midPose.position.x, 0.5F, 1.0e-4F);
    EXPECT_NEAR(result.midPose.position.y, 0.5F, 1.0e-4F);
    EXPECT_NEAR(result.endPose.position.x, 1.5F, 1.0e-4F);
    EXPECT_NEAR(result.endPose.position.y, 0.5F, 1.0e-4F);
}

TEST(ControlRigUVETest, TryMakeAimLookAtRotationUVE_AcceptsFiniteExtremeDirection) {
    Math::QuaternionUVE rotation;
    const float maximum = std::numeric_limits<float>::max();
    ASSERT_TRUE(TryMakeAimLookAtRotationUVE(
        {0.0F, 0.0F, maximum}, {0.0F, 0.0F, -maximum}, {0.0F, 1.0F, 0.0F}, rotation));
    EXPECT_NEAR(Math::LengthSquaredUVE(rotation), 1.0F, 1.0e-4F);
}

TEST(ControlRigUVETest, TryMakeAimLookAtRotationUVE_AlignsForwardAxis) {
    Math::QuaternionUVE rotation;
    ASSERT_TRUE(TryMakeAimLookAtRotationUVE(
        {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, -1.0F}, {0.0F, 1.0F, 0.0F}, rotation));
    EXPECT_EQ(rotation, (Math::QuaternionUVE{0.0F, 0.0F, 0.0F, 1.0F}));
}

TEST(ControlRigUVETest, EvaluateControlRigUVE_UsesOptionalAimLookAtPoleVector) {
    ControlRigUVE rig = MakeRigUVE();
    rig.controls[3].pose.position = {0.0F, -1.0F, -1.0F};
    rig.controls[4].pose.position = {1.0F, 0.0F, 0.0F};
    rig.constraints[0] = ControlRigConstraintUVE{
        "aim", ControlRigConstraintKindUVE::AimLookAt, "root", {}, {}, "target", "pole", 1.0F};

    Math::QuaternionUVE expected;
    ASSERT_TRUE(TryMakeAimLookAtRotationUVE(rig.controls[0].pose.position,
                                             rig.controls[3].pose.position,
                                             {1.0F, 0.0F, 0.0F}, expected));
    const ControlRigEvaluationResultUVE result = EvaluateControlRigUVE(rig);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.appliedConstraintCount, 1U);
    EXPECT_NEAR(result.controls[0].pose.rotation.x, expected.x, 1.0e-5F);
    EXPECT_NEAR(result.controls[0].pose.rotation.y, expected.y, 1.0e-5F);
    EXPECT_NEAR(result.controls[0].pose.rotation.z, expected.z, 1.0e-5F);
    EXPECT_NEAR(result.controls[0].pose.rotation.w, expected.w, 1.0e-5F);
}

TEST(ControlRigUVETest, EvaluateControlRigUVE_ClampsAimLookAtAngle) {
    ControlRigUVE rig = MakeRigUVE();
    rig.controls[3].pose.position = {1.0F, 0.0F, 0.0F};
    rig.constraints[0] = ControlRigConstraintUVE{
        "aim", ControlRigConstraintKindUVE::AimLookAt, "root", {}, {}, "target", {}, 1.0F, 8.0F, 0.0F,
        0.0F, 30.0F};

    Math::QuaternionUVE expected;
    ASSERT_TRUE(Math::TryMakeAxisAngleUVE({0.0F, -1.0F, 0.0F}, 3.14159265358979323846F / 6.0F, expected));
    const ControlRigEvaluationResultUVE result = EvaluateControlRigUVE(rig);
    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.controls[0].pose.rotation, expected);
}

TEST(ControlRigUVETest, ValidateControlRigUVE_RejectsInvalidAimLookAtAngleRange) {
    ControlRigUVE rig = MakeRigUVE();
    rig.constraints[0] = ControlRigConstraintUVE{
        "aim", ControlRigConstraintKindUVE::AimLookAt, "root", {}, {}, "target", {}, 1.0F, 8.0F, 0.0F,
        90.0F, 30.0F};
    EXPECT_EQ(ValidateControlRigUVE(rig).code, ControlRigValidationCodeUVE::InvalidConstraint);
}

TEST(ControlRigUVETest, ValidateControlRigUVE_RejectsMissingAimLookAtPole) {
    ControlRigUVE rig = MakeRigUVE();
    rig.constraints[0] = ControlRigConstraintUVE{
        "aim", ControlRigConstraintKindUVE::AimLookAt, "root", {}, {}, "target", "missing", 1.0F};
    EXPECT_EQ(ValidateControlRigUVE(rig).code, ControlRigValidationCodeUVE::UnknownControl);
}

TEST(ControlRigUVETest, SolveSpringPositionUVE_UsesBoundedFiniteWeightedResponse) {
    const TransformPoseUVE source{{0.0F, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}};
    const SpringPositionSolveResultUVE result = SolveSpringPositionUVE(
        source, {10.0F, 0.0F, 0.0F}, 1.0 / 60.0, 8.0F, 0.0F, 1.0F);

    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_GT(result.response, 0.0F);
    EXPECT_LT(result.response, 1.0F);
    EXPECT_GT(result.pose.position.x, 0.0F);
    EXPECT_LT(result.pose.position.x, 10.0F);
    EXPECT_EQ(result.pose.rotation, source.rotation);
    EXPECT_EQ(result.pose.scale, source.scale);
}

TEST(ControlRigUVETest, EvaluateControlRigUVE_AppliesCopiedSpringPositionConstraint) {
    ControlRigUVE rig = MakeRigUVE();
    rig.constraints[0] = ControlRigConstraintUVE{
        "spring", ControlRigConstraintKindUVE::SpringPosition, "root", {}, {}, "target", {}, 1.0F, 8.0F, 0.0F};
    const ControlRigEvaluationResultUVE result = EvaluateControlRigUVE(rig);

    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.appliedConstraintCount, 1U);
    EXPECT_GT(result.controls[0].pose.position.x, 0.0F);
    EXPECT_LT(result.controls[0].pose.position.x, rig.controls[3].pose.position.x);
    EXPECT_EQ(rig.controls[0].pose.position, (Math::Vector3UVE{0.0F, 0.0F, 0.0F}));
}

TEST(ControlRigUVETest, ValidateControlRigUVE_RejectsUnstableSpringParameters) {
    ControlRigUVE rig = MakeRigUVE();
    rig.constraints[0] = ControlRigConstraintUVE{
        "spring", ControlRigConstraintKindUVE::SpringPosition, "root", {}, {}, "target", {}, 1.0F, 65.0F, 0.0F};
    EXPECT_EQ(ValidateControlRigUVE(rig).code, ControlRigValidationCodeUVE::InvalidConstraint);

    EXPECT_FALSE(SolveSpringPositionUVE(rig.controls[0].pose, rig.controls[3].pose.position,
                                        0.5, 8.0F, 0.0F, 1.0F)
                     .IsSuccessUVE());
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
