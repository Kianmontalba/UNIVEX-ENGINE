// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/core/animation_retargeting_uve.h"

#include <algorithm>
#include <cmath>

namespace UVE::Core {
namespace {

constexpr std::size_t kMaximumIdentifierBytesUVE = 128U;

[[nodiscard]] bool IsIdentifierUVE(const std::string& value) noexcept {
    return !value.empty() && value.size() <= kMaximumIdentifierBytesUVE;
}

[[nodiscard]] const RetargetBonePoseUVE* FindBoneUVE(
    const std::vector<RetargetBonePoseUVE>& pose, const std::string& boneId) noexcept {
    const auto iterator = std::find_if(pose.cbegin(), pose.cend(), [&boneId](const auto& bone) {
        return bone.boneId == boneId;
    });
    return iterator == pose.cend() ? nullptr : &*iterator;
}

[[nodiscard]] RetargetBonePoseUVE* FindMutableBoneUVE(
    std::vector<RetargetBonePoseUVE>& pose, const std::string& boneId) noexcept {
    const auto iterator = std::find_if(pose.begin(), pose.end(), [&boneId](const auto& bone) {
        return bone.boneId == boneId;
    });
    return iterator == pose.end() ? nullptr : &*iterator;
}

[[nodiscard]] Math::Vector3UVE BlendVectorUVE(const Math::Vector3UVE& left,
                                               const Math::Vector3UVE& right,
                                               const float weight) noexcept {
    const float factor = std::clamp(weight, 0.0F, 1.0F);
    return left * (1.0F - factor) + right * factor;
}

} // namespace

AnimationRetargetingValidationResultUVE ValidateAnimationRetargetingUVE(
    const std::vector<RetargetBonePoseUVE>& sourceReferencePose,
    const std::vector<RetargetBonePoseUVE>& targetReferencePose,
    const AnimationRetargetingProfileUVE& profile) noexcept {
    if (sourceReferencePose.empty() || targetReferencePose.empty()) {
        return {AnimationRetargetingValidationCodeUVE::EmptyPose, {},
                "Retargeting requires source and target reference poses."};
    }
    if (profile.mappings.empty() || profile.mappings.size() > AnimationRetargetingProfileUVE::kMaximumMappingsUVE ||
        profile.ikControls.size() > AnimationRetargetingProfileUVE::kMaximumIKControlsUVE) {
        return {AnimationRetargetingValidationCodeUVE::CapacityExceeded, {},
                "Retargeting mapping or IK-control count exceeds the bounded contract."};
    }
    std::vector<std::string> sourceIds;
    std::vector<std::string> targetIds;
    for (const RetargetBonePoseUVE& bone : sourceReferencePose) {
        if (!IsIdentifierUVE(bone.boneId) || !IsFiniteTransformPoseUVE(bone.pose)) {
            return {AnimationRetargetingValidationCodeUVE::InvalidBone, bone.boneId,
                    "Source reference bone identity or pose is invalid."};
        }
        TransformPoseUVE normalized;
        if (!TryNormalizeTransformPoseUVE(bone.pose, normalized)) {
            return {AnimationRetargetingValidationCodeUVE::InvalidBone, bone.boneId,
                    "Source reference bone rotation is not normalizable."};
        }
        if (std::find(sourceIds.begin(), sourceIds.end(), bone.boneId) != sourceIds.end()) {
            return {AnimationRetargetingValidationCodeUVE::DuplicateBone, bone.boneId,
                    "Source reference bone identifiers must be unique."};
        }
        sourceIds.push_back(bone.boneId);
    }
    for (const RetargetBonePoseUVE& bone : targetReferencePose) {
        if (!IsIdentifierUVE(bone.boneId) || !IsFiniteTransformPoseUVE(bone.pose)) {
            return {AnimationRetargetingValidationCodeUVE::InvalidBone, bone.boneId,
                    "Target reference bone identity or pose is invalid."};
        }
        TransformPoseUVE normalized;
        if (!TryNormalizeTransformPoseUVE(bone.pose, normalized)) {
            return {AnimationRetargetingValidationCodeUVE::InvalidBone, bone.boneId,
                    "Target reference bone rotation is not normalizable."};
        }
        if (std::find(targetIds.begin(), targetIds.end(), bone.boneId) != targetIds.end()) {
            return {AnimationRetargetingValidationCodeUVE::DuplicateBone, bone.boneId,
                    "Target reference bone identifiers must be unique."};
        }
        targetIds.push_back(bone.boneId);
    }
    if (!std::isfinite(profile.translationScale) || profile.translationScale <= 0.0F) {
        return {AnimationRetargetingValidationCodeUVE::InvalidProfile, {},
                "Retargeting translation scale must be finite and positive."};
    }
    if (profile.sourceRootBoneId.empty() || profile.targetRootBoneId.empty()) {
        return {AnimationRetargetingValidationCodeUVE::MissingRoot, {},
                "Retargeting profile requires source and target root identifiers."};
    }
    const bool sourceRootMissing = FindBoneUVE(sourceReferencePose, profile.sourceRootBoneId) == nullptr;
    const bool targetRootMissing = FindBoneUVE(targetReferencePose, profile.targetRootBoneId) == nullptr;
    if ((sourceRootMissing || targetRootMissing) &&
        profile.missingRootPolicy == RetargetMissingRootPolicyUVE::Reject) {
        return {AnimationRetargetingValidationCodeUVE::MissingRoot, {},
                "Retargeting profile root is missing under the Reject policy."};
    }
    std::vector<std::string> mappedSources;
    std::vector<std::string> mappedTargets;
    for (const RetargetBoneMappingUVE& mapping : profile.mappings) {
        if (!IsIdentifierUVE(mapping.sourceBoneId) || !IsIdentifierUVE(mapping.targetBoneId) ||
            !Math::IsFiniteUVE(mapping.orientationCorrection)) {
            return {AnimationRetargetingValidationCodeUVE::InvalidProfile, mapping.sourceBoneId,
                    "Retargeting mapping identity or orientation correction is invalid."};
        }
        Math::QuaternionUVE normalized;
        if (!Math::TryNormalizeUVE(mapping.orientationCorrection, normalized)) {
            return {AnimationRetargetingValidationCodeUVE::InvalidProfile, mapping.sourceBoneId,
                    "Retargeting orientation correction is not normalizable."};
        }
        if (std::find(mappedSources.begin(), mappedSources.end(), mapping.sourceBoneId) != mappedSources.end() ||
            std::find(mappedTargets.begin(), mappedTargets.end(), mapping.targetBoneId) != mappedTargets.end()) {
            return {AnimationRetargetingValidationCodeUVE::DuplicateMapping, mapping.sourceBoneId,
                    "Retargeting mappings must be one-to-one."};
        }
        if (FindBoneUVE(sourceReferencePose, mapping.sourceBoneId) == nullptr && !sourceRootMissing) {
            return {AnimationRetargetingValidationCodeUVE::UnknownSourceBone, mapping.sourceBoneId,
                    "Retargeting mapping references an unknown source bone."};
        }
        if (FindBoneUVE(targetReferencePose, mapping.targetBoneId) == nullptr) {
            return {AnimationRetargetingValidationCodeUVE::UnknownTargetBone, mapping.targetBoneId,
                    "Retargeting mapping references an unknown target bone."};
        }
        mappedSources.push_back(mapping.sourceBoneId);
        mappedTargets.push_back(mapping.targetBoneId);
    }
    for (const RetargetIKControlUVE& control : profile.ikControls) {
        if (!IsIdentifierUVE(control.targetBoneId) || !std::isfinite(control.weight) ||
            control.weight < 0.0F || control.weight > 1.0F ||
            FindBoneUVE(targetReferencePose, control.targetBoneId) == nullptr) {
            return {AnimationRetargetingValidationCodeUVE::InvalidIKControl, control.targetBoneId,
                    "Retargeting IK control identity, weight, or target bone is invalid."};
        }
        if (!std::isfinite(control.targetPosition.x) || !std::isfinite(control.targetPosition.y) ||
            !std::isfinite(control.targetPosition.z)) {
            return {AnimationRetargetingValidationCodeUVE::InvalidIKControl, control.targetBoneId,
                    "Retargeting IK control position must be finite."};
        }
    }
    return {AnimationRetargetingValidationCodeUVE::Valid, {}, "Animation retargeting profile is valid."};
}

AnimationRetargetingResultUVE RetargetAnimationPoseUVE(
    const std::vector<RetargetBonePoseUVE>& sourceReferencePose,
    const std::vector<RetargetBonePoseUVE>& sourcePose,
    const std::vector<RetargetBonePoseUVE>& targetReferencePose,
    const AnimationRetargetingProfileUVE& profile) {
    AnimationRetargetingResultUVE result;
    const AnimationRetargetingValidationResultUVE validation =
        ValidateAnimationRetargetingUVE(sourceReferencePose, targetReferencePose, profile);
    if (!validation.IsValidUVE()) {
        result.message = validation.message;
        return result;
    }
    result.targetPose = targetReferencePose;
    for (const RetargetBoneMappingUVE& mapping : profile.mappings) {
        const RetargetBonePoseUVE* sourceReference = FindBoneUVE(sourceReferencePose, mapping.sourceBoneId);
        const RetargetBonePoseUVE* sourceCurrent = FindBoneUVE(sourcePose, mapping.sourceBoneId);
        RetargetBonePoseUVE* targetReference = FindMutableBoneUVE(result.targetPose, mapping.targetBoneId);
        if (sourceReference == nullptr || sourceCurrent == nullptr || targetReference == nullptr) {
            if (profile.missingRootPolicy == RetargetMissingRootPolicyUVE::Ignore) {
                result.ignoredMissingRoot = true;
                continue;
            }
            result.message = "Retargeting source or target mapping was unavailable.";
            return result;
        }
        Math::QuaternionUVE sourceReferenceInverse;
        if (!Math::TryInverseUVE(sourceReference->pose.rotation, sourceReferenceInverse)) {
            result.message = "Retargeting source reference rotation inversion failed.";
            return result;
        }
        const Math::QuaternionUVE sourceDelta = Math::MultiplyUVE(
            sourceCurrent->pose.rotation, sourceReferenceInverse);
        targetReference->pose.rotation = Math::MultiplyUVE(
            targetReference->pose.rotation, Math::MultiplyUVE(mapping.orientationCorrection, sourceDelta));
        const Math::Vector3UVE sourceDeltaPosition = sourceCurrent->pose.position - sourceReference->pose.position;
        targetReference->pose.position += sourceDeltaPosition * profile.translationScale;
        ++result.mappedBoneCount;
    }
    if (profile.applyIKControls) {
        for (const RetargetIKControlUVE& control : profile.ikControls) {
            RetargetBonePoseUVE* target = FindMutableBoneUVE(result.targetPose, control.targetBoneId);
            if (target != nullptr) {
                target->pose.position = BlendVectorUVE(target->pose.position, control.targetPosition, control.weight);
                ++result.appliedIKControlCount;
            }
        }
    }
    result.message = "Animation retargeting evaluated successfully.";
    return result;
}

} // namespace UVE::Core
