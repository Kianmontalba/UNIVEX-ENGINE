// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/core/time_pose_contract_uve.h"

#include <gtest/gtest.h>

#include <limits>

namespace UVE::Core {

TEST(UnifiedTimeContractUVETest, AdvanceUVE_UpdatesScaledDomainsAndFixedStepsDeterministically) {
    std::size_t fixedSteps = 0U;
    bool inputClamped = false;
    const UnifiedTimeStateUVE state = UnifiedTimeContractUVE::AdvanceUVE(
        {}, UnifiedTimeAdvanceInputUVE{0.1, 2.0, 0.5, 0.05, 8U, false}, fixedSteps, inputClamped);

    EXPECT_EQ(state, (UnifiedTimeStateUVE{1U, 0.1, 0.2, 0.1, 0.05, 0.0, 0.1, 0.2, 0.05, false}));
    EXPECT_EQ(fixedSteps, 2U);
    EXPECT_FALSE(inputClamped);
}

TEST(UnifiedTimeContractUVETest, AdvanceUVE_ClampsDeltaAndFixedStepBudget) {
    std::size_t fixedSteps = 0U;
    bool inputClamped = false;
    const UnifiedTimeStateUVE state = UnifiedTimeContractUVE::AdvanceUVE(
        {}, UnifiedTimeAdvanceInputUVE{1.0, 1.0, 1.0, 0.01, 100U, false}, fixedSteps, inputClamped);

    EXPECT_DOUBLE_EQ(state.realDeltaSeconds, UnifiedTimeContractUVE::kMaximumFrameDeltaSecondsUVE);
    EXPECT_EQ(fixedSteps, UnifiedTimeContractUVE::kMaximumFixedStepsPerAdvanceUVE);
    EXPECT_TRUE(inputClamped);
    EXPECT_NEAR(state.fixedAccumulatorSeconds, 0.17, 1.0e-12);
}

TEST(UnifiedTimeContractUVETest, AdvanceUVE_PauseFreezesSimulationDomainsButAdvancesRealDomain) {
    const UnifiedTimeStateUVE previous{4U, 2.0, 1.0, 0.9, 1.5, 0.02, 0.1, 0.2, 0.3, false};
    std::size_t fixedSteps = 0U;
    bool inputClamped = false;
    const UnifiedTimeStateUVE state = UnifiedTimeContractUVE::AdvanceUVE(
        previous, UnifiedTimeAdvanceInputUVE{0.1, 2.0, 2.0, 0.05, 8U, true}, fixedSteps, inputClamped);

    EXPECT_EQ(state.frameNumber, 5U);
    EXPECT_DOUBLE_EQ(state.realTimeSeconds, 2.1);
    EXPECT_DOUBLE_EQ(state.gameTimeSeconds, previous.gameTimeSeconds);
    EXPECT_DOUBLE_EQ(state.fixedTimeSeconds, previous.fixedTimeSeconds);
    EXPECT_DOUBLE_EQ(state.animationTimeSeconds, previous.animationTimeSeconds);
    EXPECT_DOUBLE_EQ(state.fixedAccumulatorSeconds, previous.fixedAccumulatorSeconds);
    EXPECT_DOUBLE_EQ(state.gameDeltaSeconds, 0.0);
    EXPECT_DOUBLE_EQ(state.animationDeltaSeconds, 0.0);
    EXPECT_EQ(fixedSteps, 0U);
    EXPECT_FALSE(inputClamped);
}

TEST(TransformPoseUVETest, TryNormalizeTransformPoseUVE_NormalizesRotationAndPreservesValues) {
    const TransformPoseUVE input{{1.0F, 2.0F, 3.0F}, {0.0F, 0.0F, 0.0F, 2.0F}, {2.0F, 3.0F, 4.0F}};
    TransformPoseUVE normalized;

    ASSERT_TRUE(TryNormalizeTransformPoseUVE(input, normalized));
    EXPECT_EQ(normalized.position, input.position);
    EXPECT_EQ(normalized.scale, input.scale);
    EXPECT_EQ(normalized.rotation, (Math::QuaternionUVE{0.0F, 0.0F, 0.0F, 1.0F}));
    EXPECT_TRUE(IsFiniteTransformPoseUVE(normalized));
}

TEST(AnimationPoseContractUVETest, ValidatesSkeletonPoseAndEvaluationContextTogether) {
    const SkeletonDefinitionUVE skeleton{
        "humanoid", {SkeletonJointUVE{"root", ""}, SkeletonJointUVE{"hand", "root"}}};
    const PoseBufferUVE pose{
        "humanoid",
        {TransformPoseUVE{{}, {}, {1.0F, 1.0F, 1.0F}},
         TransformPoseUVE{{1.0F, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}}}};
    AnimationEvaluationContextUVE context;
    context.time.animationTimeSeconds = 1.25;
    context.time.animationDeltaSeconds = 1.0 / 60.0;
    context.sampleTimeSeconds = 0.5;

    EXPECT_TRUE(ValidateSkeletonDefinitionUVE(skeleton).IsValidUVE());
    EXPECT_TRUE(ValidatePoseBufferUVE(pose, skeleton).IsValidUVE());
    EXPECT_TRUE(ValidateAnimationEvaluationContextUVE(context).IsValidUVE());
}

TEST(AnimationPoseContractUVETest, RejectsDuplicateOrUnknownJointsAndMismatchedPose) {
    const SkeletonDefinitionUVE duplicate{
        "humanoid", {SkeletonJointUVE{"root", ""}, SkeletonJointUVE{"root", ""}}};
    EXPECT_EQ(ValidateSkeletonDefinitionUVE(duplicate).code,
              AnimationContractValidationCodeUVE::DuplicateJoint);

    const SkeletonDefinitionUVE unknownParent{
        "humanoid", {SkeletonJointUVE{"root", "missing"}}};
    EXPECT_EQ(ValidateSkeletonDefinitionUVE(unknownParent).code,
              AnimationContractValidationCodeUVE::UnknownParent);

    const SkeletonDefinitionUVE skeleton{
        "humanoid", {SkeletonJointUVE{"root", ""}}};
    const PoseBufferUVE mismatched{"other", {TransformPoseUVE{}}};
    EXPECT_EQ(ValidatePoseBufferUVE(mismatched, skeleton).code,
              AnimationContractValidationCodeUVE::SkeletonMismatch);
}

TEST(AnimationPoseContractUVETest, RejectsNegativeEvaluationTime) {
    AnimationEvaluationContextUVE context;
    context.time.animationDeltaSeconds = -0.01;
    EXPECT_EQ(ValidateAnimationEvaluationContextUVE(context).code,
              AnimationContractValidationCodeUVE::InvalidTime);
}

TEST(TransformPoseUVETest, TryNormalizeTransformPoseUVE_RejectsNonFiniteAndZeroRotation) {
    TransformPoseUVE output;
    EXPECT_FALSE(TryNormalizeTransformPoseUVE(
        TransformPoseUVE{{0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F, 0.0F}, {}}, output));
    EXPECT_FALSE(TryNormalizeTransformPoseUVE(
        TransformPoseUVE{{std::numeric_limits<float>::infinity(), 0.0F, 0.0F}, {}, {}}, output));
    EXPECT_FALSE(IsFiniteTransformPoseUVE(
        TransformPoseUVE{{0.0F, 0.0F, 0.0F}, {}, {std::numeric_limits<float>::quiet_NaN(), 1.0F, 1.0F}}));
}

} // namespace UVE::Core
