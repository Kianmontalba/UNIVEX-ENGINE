// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/core/animation_retargeting_uve.h"

#include <limits>

#include <gtest/gtest.h>

namespace UVE::Core {
namespace {

std::vector<RetargetBonePoseUVE> MakeSourceReferencePoseUVE() {
    return {
        RetargetBonePoseUVE{"root", {}, TransformPoseUVE{{0.0F, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}}},
        RetargetBonePoseUVE{"arm", "root", TransformPoseUVE{{1.0F, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}}},
    };
}

std::vector<RetargetBonePoseUVE> MakeTargetReferencePoseUVE() {
    return {
        RetargetBonePoseUVE{"pelvis", {}, TransformPoseUVE{{0.0F, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}}},
        RetargetBonePoseUVE{"hand", "pelvis", TransformPoseUVE{{2.0F, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}}},
    };
}

AnimationRetargetingProfileUVE MakeProfileUVE() {
    AnimationRetargetingProfileUVE profile;
    profile.sourceRootBoneId = "root";
    profile.targetRootBoneId = "pelvis";
    profile.mappings = {
        RetargetBoneMappingUVE{"root", "pelvis", {}},
        RetargetBoneMappingUVE{"arm", "hand", {}},
    };
    return profile;
}

} // namespace

TEST(AnimationRetargetingUVETest, ValidateAnimationRetargetingUVE_AcceptsOneToOneReferenceMapping) {
    const AnimationRetargetingValidationResultUVE result = ValidateAnimationRetargetingUVE(
        MakeSourceReferencePoseUVE(), MakeTargetReferencePoseUVE(), MakeProfileUVE());

    EXPECT_TRUE(result.IsValidUVE());
    EXPECT_EQ(result.code, AnimationRetargetingValidationCodeUVE::Valid);
}

TEST(AnimationRetargetingUVETest, RetargetAnimationPoseUVE_AppliesReferenceDeltaAndTranslationScale) {
    const std::vector<RetargetBonePoseUVE> sourceReference = MakeSourceReferencePoseUVE();
    std::vector<RetargetBonePoseUVE> sourceCurrent = sourceReference;
    sourceCurrent[1].pose.position = {2.0F, 1.0F, 0.0F};
    AnimationRetargetingProfileUVE profile = MakeProfileUVE();
    profile.translationScale = 2.0F;

    const AnimationRetargetingResultUVE result = RetargetAnimationPoseUVE(
        sourceReference, sourceCurrent, MakeTargetReferencePoseUVE(), profile);

    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.mappedBoneCount, 2U);
    EXPECT_NEAR(result.targetPose[1].pose.position.x, 4.0F, 1.0e-5F);
    EXPECT_NEAR(result.targetPose[1].pose.position.y, 2.0F, 1.0e-5F);
}

TEST(AnimationRetargetingUVETest, RetargetAnimationPoseUVE_AppliesOptionalIKControlWithWeight) {
    AnimationRetargetingProfileUVE profile = MakeProfileUVE();
    profile.applyIKControls = true;
    profile.ikControls = {RetargetIKControlUVE{"hand", {6.0F, 0.0F, 0.0F}, 0.5F}};

    const AnimationRetargetingResultUVE result = RetargetAnimationPoseUVE(
        MakeSourceReferencePoseUVE(), MakeSourceReferencePoseUVE(), MakeTargetReferencePoseUVE(), profile);

    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.appliedIKControlCount, 1U);
    EXPECT_NEAR(result.targetPose[1].pose.position.x, 4.0F, 1.0e-5F);
}

TEST(AnimationRetargetingUVETest, RetargetAnimationPoseUVE_RejectsNonFiniteLiveSourceAtomically) {
    std::vector<RetargetBonePoseUVE> sourceCurrent = MakeSourceReferencePoseUVE();
    sourceCurrent[1].pose.rotation.x = std::numeric_limits<float>::quiet_NaN();

    const AnimationRetargetingResultUVE result = RetargetAnimationPoseUVE(
        MakeSourceReferencePoseUVE(), sourceCurrent, MakeTargetReferencePoseUVE(), MakeProfileUVE());

    EXPECT_FALSE(result.IsSuccessUVE());
    EXPECT_TRUE(result.targetPose.empty());
    EXPECT_EQ(result.mappedBoneCount, 0U);
}

TEST(AnimationRetargetingUVETest, RetargetAnimationPoseUVE_RejectsOverflowedTranslationAtomically) {
    std::vector<RetargetBonePoseUVE> sourceCurrent = MakeSourceReferencePoseUVE();
    sourceCurrent[1].pose.position = {std::numeric_limits<float>::max(), 0.0F, 0.0F};
    AnimationRetargetingProfileUVE profile = MakeProfileUVE();
    profile.translationScale = 2.0F;

    const AnimationRetargetingResultUVE result = RetargetAnimationPoseUVE(
        MakeSourceReferencePoseUVE(), sourceCurrent, MakeTargetReferencePoseUVE(), profile);

    EXPECT_FALSE(result.IsSuccessUVE());
    EXPECT_TRUE(result.targetPose.empty());
    EXPECT_EQ(result.mappedBoneCount, 0U);
}

TEST(AnimationRetargetingUVETest, ValidateAnimationRetargetingUVE_RejectsDuplicateMappingAndMissingRootPolicy) {
    AnimationRetargetingProfileUVE duplicate = MakeProfileUVE();
    duplicate.mappings.push_back(RetargetBoneMappingUVE{"arm", "pelvis", {}});
    EXPECT_EQ(ValidateAnimationRetargetingUVE(
        MakeSourceReferencePoseUVE(), MakeTargetReferencePoseUVE(), duplicate).code,
        AnimationRetargetingValidationCodeUVE::DuplicateMapping);

    AnimationRetargetingProfileUVE missingRoot = MakeProfileUVE();
    missingRoot.sourceRootBoneId = "missing";
    EXPECT_EQ(ValidateAnimationRetargetingUVE(
        MakeSourceReferencePoseUVE(), MakeTargetReferencePoseUVE(), missingRoot).code,
        AnimationRetargetingValidationCodeUVE::MissingRoot);
    missingRoot.missingRootPolicy = RetargetMissingRootPolicyUVE::Ignore;
    EXPECT_TRUE(ValidateAnimationRetargetingUVE(
        MakeSourceReferencePoseUVE(), MakeTargetReferencePoseUVE(), missingRoot).IsValidUVE());
}

} // namespace UVE::Core
