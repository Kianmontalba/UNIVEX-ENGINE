// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/core/time_pose_contract_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace UVE::Core {

struct RetargetBonePoseUVE final {
    std::string boneId;
    std::string parentBoneId;
    TransformPoseUVE pose;
};

enum class RetargetMissingRootPolicyUVE : std::uint8_t {
    Reject = 0,
    Ignore,
};

struct RetargetBoneMappingUVE final {
    std::string sourceBoneId;
    std::string targetBoneId;
    Math::QuaternionUVE orientationCorrection;
};

struct RetargetIKControlUVE final {
    std::string targetBoneId;
    Math::Vector3UVE targetPosition;
    float weight = 1.0F;
};

struct AnimationRetargetingProfileUVE final {
    static constexpr std::size_t kMaximumMappingsUVE = 512U;
    static constexpr std::size_t kMaximumIKControlsUVE = 128U;

    std::string sourceRootBoneId;
    std::string targetRootBoneId;
    float translationScale = 1.0F;
    RetargetMissingRootPolicyUVE missingRootPolicy = RetargetMissingRootPolicyUVE::Reject;
    bool applyIKControls = false;
    std::vector<RetargetBoneMappingUVE> mappings;
    std::vector<RetargetIKControlUVE> ikControls;
};

enum class AnimationRetargetingValidationCodeUVE : std::uint8_t {
    Valid = 0,
    EmptyPose,
    CapacityExceeded,
    InvalidBone,
    DuplicateBone,
    InvalidProfile,
    DuplicateMapping,
    UnknownSourceBone,
    UnknownTargetBone,
    MissingRoot,
    InvalidIKControl,
};

struct AnimationRetargetingValidationResultUVE final {
    AnimationRetargetingValidationCodeUVE code = AnimationRetargetingValidationCodeUVE::EmptyPose;
    std::string identifier;
    std::string message;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return code == AnimationRetargetingValidationCodeUVE::Valid;
    }
};

struct AnimationRetargetingResultUVE final {
    std::vector<RetargetBonePoseUVE> targetPose;
    std::size_t mappedBoneCount = 0U;
    std::size_t appliedIKControlCount = 0U;
    bool ignoredMissingRoot = false;
    std::string message;

    [[nodiscard]] bool IsSuccessUVE() const noexcept {
        return !targetPose.empty() && mappedBoneCount > 0U;
    }
};

[[nodiscard]] AnimationRetargetingValidationResultUVE ValidateAnimationRetargetingUVE(
    const std::vector<RetargetBonePoseUVE>& sourceReferencePose,
    const std::vector<RetargetBonePoseUVE>& targetReferencePose,
    const AnimationRetargetingProfileUVE& profile) noexcept;

[[nodiscard]] AnimationRetargetingResultUVE RetargetAnimationPoseUVE(
    const std::vector<RetargetBonePoseUVE>& sourceReferencePose,
    const std::vector<RetargetBonePoseUVE>& sourcePose,
    const std::vector<RetargetBonePoseUVE>& targetReferencePose,
    const AnimationRetargetingProfileUVE& profile);

} // namespace UVE::Core
