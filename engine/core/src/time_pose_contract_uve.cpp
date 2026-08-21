// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/core/time_pose_contract_uve.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace UVE::Core {
namespace {

[[nodiscard]] double NonNegativeScaleUVE(const double value) noexcept {
    return std::isfinite(value) && value >= 0.0 ? value : 0.0;
}

[[nodiscard]] bool IsFiniteVectorUVE(const Math::Vector3UVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool IsAnimationIdentifierValidUVE(const std::string& identifier, const bool allowEmpty) noexcept {
    return (allowEmpty || !identifier.empty()) &&
           identifier.size() <= kMaximumAnimationIdentifierBytesUVE &&
           identifier.find('\0') == std::string::npos;
}

[[nodiscard]] bool IsFiniteUnifiedTimeStateUVE(const UnifiedTimeStateUVE& state) noexcept {
    return std::isfinite(state.realTimeSeconds) && std::isfinite(state.gameTimeSeconds) &&
           std::isfinite(state.fixedTimeSeconds) && std::isfinite(state.animationTimeSeconds) &&
           std::isfinite(state.fixedAccumulatorSeconds) && std::isfinite(state.realDeltaSeconds) &&
           std::isfinite(state.gameDeltaSeconds) && std::isfinite(state.animationDeltaSeconds);
}

} // namespace

UnifiedTimeStateUVE UnifiedTimeContractUVE::AdvanceUVE(
    const UnifiedTimeStateUVE& previous, UnifiedTimeAdvanceInputUVE input,
    std::size_t& outFixedSteps, bool& outInputClamped) noexcept {
    UnifiedTimeStateUVE next = previous;
    outFixedSteps = 0U;
    outInputClamped = false;

    double realDelta = input.realDeltaSeconds;
    if (!std::isfinite(realDelta) || realDelta < 0.0) {
        realDelta = 0.0;
        outInputClamped = true;
    }
    if (realDelta > kMaximumFrameDeltaSecondsUVE) {
        realDelta = kMaximumFrameDeltaSecondsUVE;
        outInputClamped = true;
    }
    double fixedStep = input.fixedStepSeconds;
    if (!std::isfinite(fixedStep) || fixedStep <= 0.0) {
        fixedStep = 1.0 / 60.0;
        outInputClamped = true;
    }
    const std::size_t maximumFixedSteps = std::min(
        input.maximumFixedSteps, kMaximumFixedStepsPerAdvanceUVE);
    if (input.maximumFixedSteps > kMaximumFixedStepsPerAdvanceUVE) {
        outInputClamped = true;
    }
    const double gameScale = NonNegativeScaleUVE(input.gameTimeScale);
    const double animationScale = NonNegativeScaleUVE(input.animationTimeScale);
    if (!std::isfinite(input.gameTimeScale) || input.gameTimeScale < 0.0 ||
        !std::isfinite(input.animationTimeScale) || input.animationTimeScale < 0.0) {
        outInputClamped = true;
    }

    next.frameNumber = previous.frameNumber == std::numeric_limits<std::uint64_t>::max()
        ? previous.frameNumber : previous.frameNumber + 1U;
    next.realDeltaSeconds = realDelta;
    next.gameDeltaSeconds = input.paused ? 0.0 : realDelta * gameScale;
    next.animationDeltaSeconds = input.paused ? 0.0 : realDelta * animationScale;
    next.realTimeSeconds += realDelta;
    if (!input.paused) {
        next.gameTimeSeconds += next.gameDeltaSeconds;
        next.animationTimeSeconds += next.animationDeltaSeconds;
        next.fixedAccumulatorSeconds += realDelta;
        while (outFixedSteps < maximumFixedSteps && next.fixedAccumulatorSeconds >= fixedStep) {
            next.fixedAccumulatorSeconds -= fixedStep;
            next.fixedTimeSeconds += fixedStep;
            ++outFixedSteps;
        }
    }
    next.paused = input.paused;
    if (!IsFiniteUnifiedTimeStateUVE(next)) {
        outFixedSteps = 0U;
        outInputClamped = true;
        return previous;
    }
    return next;
}

AnimationContractValidationResultUVE ValidateSkeletonDefinitionUVE(
    const SkeletonDefinitionUVE& skeleton) noexcept {
    if (skeleton.skeletonId.empty() && skeleton.joints.empty()) {
        return {AnimationContractValidationCodeUVE::Valid, 0U, "empty skeleton contract"};
    }
    if (!IsAnimationIdentifierValidUVE(skeleton.skeletonId, false)) {
        return {AnimationContractValidationCodeUVE::InvalidIdentifier, 0U,
                "skeleton identifier is empty or exceeds its bound"};
    }
    if (skeleton.joints.empty() || skeleton.joints.size() > kMaximumSkeletonJointsUVE) {
        return {AnimationContractValidationCodeUVE::CapacityExceeded, 0U,
                "skeleton joint count is empty or exceeds its bound"};
    }
    for (std::size_t index = 0U; index < skeleton.joints.size(); ++index) {
        const SkeletonJointUVE& joint = skeleton.joints[index];
        if (!IsAnimationIdentifierValidUVE(joint.jointId, false) ||
            !IsAnimationIdentifierValidUVE(joint.parentJointId, true)) {
            return {AnimationContractValidationCodeUVE::InvalidIdentifier, index,
                    "skeleton joint identifier exceeds its bound"};
        }
        if (std::find_if(skeleton.joints.cbegin(), skeleton.joints.cbegin() + static_cast<std::ptrdiff_t>(index),
                         [&joint](const SkeletonJointUVE& previous) {
                             return previous.jointId == joint.jointId;
                         }) != skeleton.joints.cbegin() + static_cast<std::ptrdiff_t>(index)) {
            return {AnimationContractValidationCodeUVE::DuplicateJoint, index,
                    "skeleton joint identifiers must be unique"};
        }
    }
    for (std::size_t index = 0U; index < skeleton.joints.size(); ++index) {
        const std::string& parentId = skeleton.joints[index].parentJointId;
        if (!parentId.empty() && std::find_if(
                skeleton.joints.cbegin(), skeleton.joints.cend(), [&parentId](const SkeletonJointUVE& joint) {
                    return joint.jointId == parentId;
                }) == skeleton.joints.cend()) {
            return {AnimationContractValidationCodeUVE::UnknownParent, index,
                    "skeleton joint parent references an unknown joint"};
        }
    }
    return {AnimationContractValidationCodeUVE::Valid, 0U, "valid skeleton contract"};
}

AnimationContractValidationResultUVE ValidateSkeletonRetargetMapUVE(
    const SkeletonDefinitionUVE& sourceSkeleton, const SkeletonDefinitionUVE& targetSkeleton,
    const std::vector<SkeletonRetargetMapEntryUVE>& map) noexcept {
    const AnimationContractValidationResultUVE sourceValidation =
        ValidateSkeletonDefinitionUVE(sourceSkeleton);
    if (!sourceValidation.IsValidUVE()) {
        return sourceValidation;
    }
    const AnimationContractValidationResultUVE targetValidation =
        ValidateSkeletonDefinitionUVE(targetSkeleton);
    if (!targetValidation.IsValidUVE()) {
        return targetValidation;
    }
    if (sourceSkeleton.skeletonId.empty() || targetSkeleton.skeletonId.empty() ||
        map.size() > kMaximumSkeletonJointsUVE) {
        return {AnimationContractValidationCodeUVE::CapacityExceeded, 0U,
                "retarget map requires bounded non-empty source and target skeletons"};
    }
    for (std::size_t index = 0U; index < map.size(); ++index) {
        const SkeletonRetargetMapEntryUVE& entry = map[index];
        if (!IsAnimationIdentifierValidUVE(entry.sourceJointId, false) ||
            !IsAnimationIdentifierValidUVE(entry.targetJointId, false)) {
            return {AnimationContractValidationCodeUVE::InvalidIdentifier, index,
                    "retarget map joint identifiers are invalid"};
        }
        const auto sourceIt = std::find_if(sourceSkeleton.joints.cbegin(), sourceSkeleton.joints.cend(),
                                           [&entry](const SkeletonJointUVE& joint) {
                                               return joint.jointId == entry.sourceJointId;
                                           });
        const auto targetIt = std::find_if(targetSkeleton.joints.cbegin(), targetSkeleton.joints.cend(),
                                           [&entry](const SkeletonJointUVE& joint) {
                                               return joint.jointId == entry.targetJointId;
                                           });
        if (sourceIt == sourceSkeleton.joints.cend() || targetIt == targetSkeleton.joints.cend()) {
            return {AnimationContractValidationCodeUVE::SkeletonMismatch, index,
                    "retarget map references an unknown source or target joint"};
        }
        if (std::find_if(map.cbegin(), map.cbegin() + static_cast<std::ptrdiff_t>(index),
                         [&entry](const SkeletonRetargetMapEntryUVE& previous) {
                             return previous.sourceJointId == entry.sourceJointId ||
                                    previous.targetJointId == entry.targetJointId;
                         }) != map.cbegin() + static_cast<std::ptrdiff_t>(index)) {
            return {AnimationContractValidationCodeUVE::DuplicateJoint, index,
                    "retarget map source and target joints must be unique"};
        }
    }
    return {AnimationContractValidationCodeUVE::Valid, 0U, "valid skeleton retarget map"};
}

AnimationContractValidationResultUVE ValidatePoseBufferUVE(
    const PoseBufferUVE& pose, const SkeletonDefinitionUVE& skeleton) noexcept {
    const AnimationContractValidationResultUVE skeletonValidation = ValidateSkeletonDefinitionUVE(skeleton);
    if (!skeletonValidation.IsValidUVE()) {
        return skeletonValidation;
    }
    if (!IsAnimationIdentifierValidUVE(pose.skeletonId, true)) {
        return {AnimationContractValidationCodeUVE::InvalidIdentifier, 0U,
                "pose buffer skeleton identifier is empty, oversized, or contains NUL"};
    }
    if (skeleton.skeletonId.empty() && pose.skeletonId.empty() && pose.localJoints.empty()) {
        return {AnimationContractValidationCodeUVE::Valid, 0U, "empty pose contract"};
    }
    if (pose.skeletonId != skeleton.skeletonId || pose.localJoints.size() != skeleton.joints.size()) {
        return {AnimationContractValidationCodeUVE::SkeletonMismatch, 0U,
                "pose buffer skeleton identity or joint count does not match"};
    }
    for (std::size_t index = 0U; index < pose.localJoints.size(); ++index) {
        TransformPoseUVE normalized;
        if (!TryNormalizeTransformPoseUVE(pose.localJoints[index], normalized)) {
            return {AnimationContractValidationCodeUVE::InvalidPose, index,
                    "pose buffer contains a non-finite or non-normalizable joint transform"};
        }
    }
    return {AnimationContractValidationCodeUVE::Valid, 0U, "valid pose contract"};
}

AnimationContractValidationResultUVE ValidateAnimationEvaluationContextUVE(
    const AnimationEvaluationContextUVE& context) noexcept {
    const UnifiedTimeStateUVE& time = context.time;
    const auto finiteNonNegative = [](const double value) noexcept {
        return std::isfinite(value) && value >= 0.0;
    };
    if (!finiteNonNegative(time.realTimeSeconds) || !finiteNonNegative(time.gameTimeSeconds) ||
        !finiteNonNegative(time.fixedTimeSeconds) || !finiteNonNegative(time.animationTimeSeconds) ||
        !finiteNonNegative(time.fixedAccumulatorSeconds) || !finiteNonNegative(time.realDeltaSeconds) ||
        !finiteNonNegative(time.gameDeltaSeconds) || !finiteNonNegative(time.animationDeltaSeconds) ||
        !finiteNonNegative(context.sampleTimeSeconds)) {
        return {AnimationContractValidationCodeUVE::InvalidTime, 0U,
                "animation evaluation time contains a non-finite or negative value"};
    }
    return {AnimationContractValidationCodeUVE::Valid, 0U, "valid evaluation context"};
}

bool IsFiniteTransformPoseUVE(const TransformPoseUVE& pose) noexcept {
    return IsFiniteVectorUVE(pose.position) && IsFiniteVectorUVE(pose.scale) &&
           Math::IsFiniteUVE(pose.rotation);
}

bool TryNormalizeTransformPoseUVE(const TransformPoseUVE& pose,
                                  TransformPoseUVE& outNormalized) noexcept {
    if (!IsFiniteTransformPoseUVE(pose)) {
        return false;
    }
    Math::QuaternionUVE normalizedRotation;
    if (!Math::TryNormalizeUVE(pose.rotation, normalizedRotation)) {
        return false;
    }
    outNormalized = pose;
    outNormalized.rotation = normalizedRotation;
    return true;
}

} // namespace UVE::Core
