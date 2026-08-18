// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/plugins/control_rig_uve.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace UVE::Core {
namespace {

constexpr std::size_t kMaximumIdentifierBytesUVE = 128U;
constexpr float kEpsilonUVE = 1.0e-5F;

[[nodiscard]] bool IsIdentifierUVE(const std::string& value) noexcept {
    return !value.empty() && value.size() <= kMaximumIdentifierBytesUVE;
}

[[nodiscard]] const ControlRigControlUVE* FindControlUVE(
    const std::vector<ControlRigControlUVE>& controls, const std::string& controlId) noexcept {
    const auto iterator = std::find_if(controls.cbegin(), controls.cend(), [&controlId](const auto& control) {
        return control.controlId == controlId;
    });
    return iterator == controls.cend() ? nullptr : &*iterator;
}

[[nodiscard]] Math::Vector3UVE NormalizeVectorUVE(const Math::Vector3UVE& value,
                                                   const Math::Vector3UVE& fallback) noexcept {
    const float lengthSquared = Math::LengthSquaredUVE(value);
    if (!std::isfinite(lengthSquared) || lengthSquared <= kEpsilonUVE * kEpsilonUVE) {
        return fallback;
    }
    return Math::NormalizeUVE(value);
}

} // namespace

ControlRigValidationResultUVE ValidateControlRigUVE(const ControlRigUVE& rig) noexcept {
    if (rig.controls.empty()) {
        return {ControlRigValidationCodeUVE::EmptyRig, {}, "Control Rig requires at least one control."};
    }
    if (rig.controls.size() > ControlRigUVE::kMaximumControlsUVE ||
        rig.constraints.size() > ControlRigUVE::kMaximumConstraintsUVE) {
        return {ControlRigValidationCodeUVE::CapacityExceeded, {},
                "Control Rig exceeds the bounded control or constraint limit."};
    }
    const AnimationContractValidationResultUVE skeletonValidation =
        ValidateSkeletonDefinitionUVE(rig.skeleton);
    if (!skeletonValidation.IsValidUVE()) {
        return {ControlRigValidationCodeUVE::InvalidSkeleton, {}, skeletonValidation.message};
    }
    const AnimationContractValidationResultUVE poseValidation =
        ValidatePoseBufferUVE(rig.pose, rig.skeleton);
    if (!poseValidation.IsValidUVE()) {
        return {ControlRigValidationCodeUVE::InvalidPose, {}, poseValidation.message};
    }
    const AnimationContractValidationResultUVE timeValidation =
        ValidateAnimationEvaluationContextUVE(rig.evaluationContext);
    if (!timeValidation.IsValidUVE()) {
        return {ControlRigValidationCodeUVE::InvalidEvaluationTime, {}, timeValidation.message};
    }
    std::vector<std::string> controlIds;
    controlIds.reserve(rig.controls.size());
    for (const ControlRigControlUVE& control : rig.controls) {
        if (!IsIdentifierUVE(control.controlId) ||
            !IsFiniteTransformPoseUVE(control.pose)) {
            return {ControlRigValidationCodeUVE::InvalidControl, control.controlId,
                    "Control identity or pose is invalid."};
        }
        TransformPoseUVE normalized;
        if (!TryNormalizeTransformPoseUVE(control.pose, normalized)) {
            return {ControlRigValidationCodeUVE::InvalidPose, control.controlId,
                    "Control rotation must be finite and normalizable."};
        }
        if (std::find(controlIds.begin(), controlIds.end(), control.controlId) != controlIds.end()) {
            return {ControlRigValidationCodeUVE::DuplicateControl, control.controlId,
                    "Control identifiers must be unique."};
        }
        controlIds.push_back(control.controlId);
    }
    for (const ControlRigControlUVE& control : rig.controls) {
        if (!control.parentControlId.empty() &&
            FindControlUVE(rig.controls, control.parentControlId) == nullptr) {
            return {ControlRigValidationCodeUVE::UnknownControl, control.parentControlId,
                    "Control parent references an unknown control."};
        }
    }
    std::vector<std::string> constraintIds;
    constraintIds.reserve(rig.constraints.size());
    for (const ControlRigConstraintUVE& constraint : rig.constraints) {
        if (!IsIdentifierUVE(constraint.constraintId) ||
            !std::isfinite(constraint.weight) || constraint.weight < 0.0F || constraint.weight > 1.0F) {
            return {ControlRigValidationCodeUVE::InvalidConstraint, constraint.constraintId,
                    "Constraint identity or weight is invalid."};
        }
        if (constraint.kind == ControlRigConstraintKindUVE::SpringPosition &&
            (!std::isfinite(constraint.stiffness) || constraint.stiffness < 0.0F ||
             constraint.stiffness > 64.0F || !std::isfinite(constraint.damping) ||
             constraint.damping < 0.0F || constraint.damping > 1.0F)) {
            return {ControlRigValidationCodeUVE::InvalidConstraint, constraint.constraintId,
                    "Spring constraint stiffness or damping is outside its stable bounded range."};
        }
        if (std::find(constraintIds.begin(), constraintIds.end(), constraint.constraintId) != constraintIds.end()) {
            return {ControlRigValidationCodeUVE::DuplicateConstraint, constraint.constraintId,
                    "Constraint identifiers must be unique."};
        }
        constraintIds.push_back(constraint.constraintId);
        const bool isTwoBone = constraint.kind == ControlRigConstraintKindUVE::TwoBoneIK;
        const bool endpointsValid = IsIdentifierUVE(constraint.sourceControlId) &&
            (isTwoBone ? IsIdentifierUVE(constraint.midControlId) && IsIdentifierUVE(constraint.endControlId)
                       : true) && IsIdentifierUVE(constraint.targetControlId);
        if (!endpointsValid || FindControlUVE(rig.controls, constraint.sourceControlId) == nullptr ||
            FindControlUVE(rig.controls, constraint.targetControlId) == nullptr ||
            (isTwoBone && (FindControlUVE(rig.controls, constraint.midControlId) == nullptr ||
                           FindControlUVE(rig.controls, constraint.endControlId) == nullptr)) ||
            (isTwoBone && (!constraint.poleControlId.empty() &&
                           FindControlUVE(rig.controls, constraint.poleControlId) == nullptr))) {
            return {ControlRigValidationCodeUVE::UnknownControl, constraint.constraintId,
                    "Constraint references an unknown or missing control."};
        }
    }
    return {ControlRigValidationCodeUVE::Valid, {}, "Control Rig is valid."};
}

TransformPoseUVE BlendControlRigPoseUVE(const TransformPoseUVE& source,
                                         const TransformPoseUVE& target,
                                         const float weight) noexcept {
    const float factor = std::isfinite(weight) ? std::clamp(weight, 0.0F, 1.0F) : 0.0F;
    TransformPoseUVE blended;
    blended.position = source.position * (1.0F - factor) + target.position * factor;
    blended.scale = source.scale * (1.0F - factor) + target.scale * factor;
    blended.rotation = Math::QuaternionUVE{
        source.rotation.x * (1.0F - factor) + target.rotation.x * factor,
        source.rotation.y * (1.0F - factor) + target.rotation.y * factor,
        source.rotation.z * (1.0F - factor) + target.rotation.z * factor,
        source.rotation.w * (1.0F - factor) + target.rotation.w * factor,
    };
    TransformPoseUVE normalized;
    return TryNormalizeTransformPoseUVE(blended, normalized) ? normalized : source;
}

TwoBoneIKSolveResultUVE SolveTwoBoneIKUVE(const TransformPoseUVE& rootPose,
                                          const TransformPoseUVE& midPose,
                                          const TransformPoseUVE& endPose,
                                          const Math::Vector3UVE& target,
                                          const Math::Vector3UVE& pole,
                                          const float weight) noexcept {
    TwoBoneIKSolveResultUVE result{rootPose, midPose, endPose, false, false};
    if (!IsFiniteTransformPoseUVE(rootPose) || !IsFiniteTransformPoseUVE(midPose) ||
        !IsFiniteTransformPoseUVE(endPose) || !std::isfinite(weight)) {
        return result;
    }
    const Math::Vector3UVE root = rootPose.position;
    const float firstLength = Math::LengthUVE(midPose.position - root);
    const float secondLength = Math::LengthUVE(endPose.position - midPose.position);
    if (!std::isfinite(firstLength) || !std::isfinite(secondLength) ||
        firstLength <= kEpsilonUVE || secondLength <= kEpsilonUVE) {
        return result;
    }
    const Math::Vector3UVE targetOffset = target - root;
    const float requestedDistance = Math::LengthUVE(targetOffset);
    if (!std::isfinite(requestedDistance)) {
        return result;
    }
    const float minimumDistance = std::abs(firstLength - secondLength) + kEpsilonUVE;
    const float maximumDistance = firstLength + secondLength - kEpsilonUVE;
    const float solvedDistance = std::clamp(requestedDistance, minimumDistance, maximumDistance);
    result.reachable = requestedDistance >= minimumDistance && requestedDistance <= maximumDistance;
    result.targetClamped = !result.reachable;

    const Math::Vector3UVE fallbackDirection{1.0F, 0.0F, 0.0F};
    const Math::Vector3UVE direction = NormalizeVectorUVE(targetOffset, fallbackDirection);
    const Math::Vector3UVE poleOffset = pole - root;
    const Math::Vector3UVE projectedPole = poleOffset - direction * Math::DotUVE(poleOffset, direction);
    const Math::Vector3UVE bendDirection = NormalizeVectorUVE(projectedPole, {0.0F, 1.0F, 0.0F});
    const float cosine = std::clamp((firstLength * firstLength + solvedDistance * solvedDistance -
                                     secondLength * secondLength) /
                                        (2.0F * firstLength * solvedDistance), -1.0F, 1.0F);
    const float along = firstLength * cosine;
    const float bend = firstLength * std::sqrt(std::max(0.0F, 1.0F - cosine * cosine));
    const Math::Vector3UVE solvedMid = root + direction * along + bendDirection * bend;
    const Math::Vector3UVE solvedEnd = root + direction * solvedDistance;
    TransformPoseUVE solvedMidPose = midPose;
    solvedMidPose.position = solvedMid;
    TransformPoseUVE solvedEndPose = endPose;
    solvedEndPose.position = solvedEnd;
    result.midPose = BlendControlRigPoseUVE(midPose, solvedMidPose, weight);
    result.endPose = BlendControlRigPoseUVE(endPose, solvedEndPose, weight);
    result.rootPose.position = rootPose.position;
    return result;
}

bool TryMakeAimLookAtRotationUVE(const Math::Vector3UVE& source, const Math::Vector3UVE& target,
                                 const Math::Vector3UVE& up,
                                 Math::QuaternionUVE& outRotation) noexcept {
    const Math::Vector3UVE forward{0.0F, 0.0F, -1.0F};
    const Math::Vector3UVE direction = NormalizeVectorUVE(target - source, {});
    if (Math::LengthSquaredUVE(direction) <= kEpsilonUVE * kEpsilonUVE) {
        return false;
    }
    const float dot = std::clamp(Math::DotUVE(forward, direction), -1.0F, 1.0F);
    Math::Vector3UVE axis = Math::CrossUVE(forward, direction);
    if (Math::LengthUVE(axis) <= kEpsilonUVE) {
        axis = NormalizeVectorUVE(up, {0.0F, 1.0F, 0.0F});
    }
    return Math::TryMakeAxisAngleUVE(axis, static_cast<float>(std::acos(dot)), outRotation);
}

SpringPositionSolveResultUVE SolveSpringPositionUVE(const TransformPoseUVE& source,
                                                    const Math::Vector3UVE& target,
                                                    const double deltaSeconds,
                                                    const float stiffness,
                                                    const float damping,
                                                    const float weight) noexcept {
    SpringPositionSolveResultUVE result{source, 0.0F, false};
    if (!IsFiniteTransformPoseUVE(source) || !std::isfinite(target.x) || !std::isfinite(target.y) ||
        !std::isfinite(target.z) || !std::isfinite(deltaSeconds) || deltaSeconds < 0.0 ||
        deltaSeconds > 0.25 || !std::isfinite(stiffness) || stiffness < 0.0F || stiffness > 64.0F ||
        !std::isfinite(damping) || damping < 0.0F || damping > 1.0F || !std::isfinite(weight) ||
        weight < 0.0F || weight > 1.0F) {
        return result;
    }
    const float response = static_cast<float>(1.0 - std::exp(-static_cast<double>(stiffness) * deltaSeconds));
    const float dampedResponse = std::clamp(response * (1.0F - 0.5F * damping) * weight, 0.0F, 1.0F);
    TransformPoseUVE targetPose = source;
    targetPose.position = target;
    result.pose = BlendControlRigPoseUVE(source, targetPose, dampedResponse);
    result.response = dampedResponse;
    result.applied = true;
    return result;
}

ControlRigEvaluationResultUVE EvaluateControlRigUVE(const ControlRigUVE& rig) {
    ControlRigEvaluationResultUVE result;
    const ControlRigValidationResultUVE validation = ValidateControlRigUVE(rig);
    if (!validation.IsValidUVE()) {
        result.message = validation.message;
        return result;
    }
    result.controls = rig.controls;
    result.skeleton = rig.skeleton;
    result.pose = rig.pose;
    result.evaluationContext = rig.evaluationContext;
    for (const ControlRigConstraintUVE& constraint : rig.constraints) {
        auto findMutable = [&result](const std::string& id) {
            return std::find_if(result.controls.begin(), result.controls.end(), [&id](auto& control) {
                return control.controlId == id;
            });
        };
        if (constraint.kind == ControlRigConstraintKindUVE::TwoBoneIK) {
            const auto root = findMutable(constraint.sourceControlId);
            const auto mid = findMutable(constraint.midControlId);
            const auto end = findMutable(constraint.endControlId);
            const auto target = findMutable(constraint.targetControlId);
            const auto pole = constraint.poleControlId.empty()
                ? result.controls.end() : findMutable(constraint.poleControlId);
            const Math::Vector3UVE polePosition = pole == result.controls.end()
                ? root->pose.position + Math::Vector3UVE{0.0F, 1.0F, 0.0F} : pole->pose.position;
            const TwoBoneIKSolveResultUVE solved = SolveTwoBoneIKUVE(
                root->pose, mid->pose, end->pose, target->pose.position, polePosition, constraint.weight);
            if (!solved.IsSuccessUVE()) {
                result.message = "Control Rig two-bone IK solve failed.";
                return result;
            }
            mid->pose = solved.midPose;
            end->pose = solved.endPose;
        } else {
            const auto source = findMutable(constraint.sourceControlId);
            const auto target = findMutable(constraint.targetControlId);
            if (constraint.kind == ControlRigConstraintKindUVE::AimLookAt) {
                Math::QuaternionUVE rotation;
                if (!TryMakeAimLookAtRotationUVE(source->pose.position, target->pose.position,
                                                 {0.0F, 1.0F, 0.0F}, rotation)) {
                    result.message = "Control Rig aim/look-at solve failed.";
                    return result;
                }
                source->pose.rotation = rotation;
            } else {
                const SpringPositionSolveResultUVE solved = SolveSpringPositionUVE(
                    source->pose, target->pose.position, rig.evaluationContext.time.animationDeltaSeconds,
                    constraint.stiffness, constraint.damping, constraint.weight);
                if (!solved.IsSuccessUVE()) {
                    result.message = "Control Rig spring-position solve failed.";
                    return result;
                }
                source->pose = solved.pose;
            }
        }
        ++result.appliedConstraintCount;
    }
    result.evaluated = true;
    result.message = "Control Rig evaluated successfully.";
    return result;
}

} // namespace UVE::Core
