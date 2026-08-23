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

[[nodiscard]] bool IsValidControlRigSpaceUVE(const ControlRigSpaceUVE space) noexcept {
    switch (space) {
    case ControlRigSpaceUVE::Local:
    case ControlRigSpaceUVE::World:
        return true;
    }
    return false;
}

[[nodiscard]] bool IsValidControlRigConstraintKindUVE(const ControlRigConstraintKindUVE kind) noexcept {
    switch (kind) {
    case ControlRigConstraintKindUVE::TwoBoneIK:
    case ControlRigConstraintKindUVE::AimLookAt:
    case ControlRigConstraintKindUVE::SpringPosition:
        return true;
    }
    return false;
}

[[nodiscard]] bool IsFiniteVectorUVE(const Math::Vector3UVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] Math::Vector3UVE NormalizeVectorUVE(const Math::Vector3UVE& value,
                                                   const Math::Vector3UVE& fallback) noexcept {
    if (!IsFiniteVectorUVE(value)) {
        return fallback;
    }
    const double x = static_cast<double>(value.x);
    const double y = static_cast<double>(value.y);
    const double z = static_cast<double>(value.z);
    const double scale = std::max(std::fabs(x), std::max(std::fabs(y), std::fabs(z)));
    if (!std::isfinite(scale) || scale == 0.0) {
        return fallback;
    }
    const double scaledLengthSquared = (x / scale) * (x / scale) +
                                       (y / scale) * (y / scale) +
                                       (z / scale) * (z / scale);
    if (!std::isfinite(scaledLengthSquared) ||
        scaledLengthSquared <= static_cast<double>(kEpsilonUVE * kEpsilonUVE)) {
        return fallback;
    }
    return Math::NormalizeUVE(value);
}

[[nodiscard]] Math::Vector3UVE NormalizeDifferenceVectorUVE(
    const Math::Vector3UVE& lhs, const Math::Vector3UVE& rhs,
    const Math::Vector3UVE& fallback) noexcept {
    if (!IsFiniteVectorUVE(lhs) || !IsFiniteVectorUVE(rhs)) {
        return fallback;
    }
    const double x = static_cast<double>(lhs.x) - static_cast<double>(rhs.x);
    const double y = static_cast<double>(lhs.y) - static_cast<double>(rhs.y);
    const double z = static_cast<double>(lhs.z) - static_cast<double>(rhs.z);
    const double scale = std::max(std::fabs(x), std::max(std::fabs(y), std::fabs(z)));
    if (!std::isfinite(scale) || scale == 0.0) {
        return fallback;
    }
    return NormalizeVectorUVE(
        {static_cast<float>(x / scale), static_cast<float>(y / scale), static_cast<float>(z / scale)},
        fallback);
}

[[nodiscard]] double LengthDifferenceUVE(const Math::Vector3UVE& lhs,
                                         const Math::Vector3UVE& rhs) noexcept {
    if (!IsFiniteVectorUVE(lhs) || !IsFiniteVectorUVE(rhs)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const double x = static_cast<double>(lhs.x) - static_cast<double>(rhs.x);
    const double y = static_cast<double>(lhs.y) - static_cast<double>(rhs.y);
    const double z = static_cast<double>(lhs.z) - static_cast<double>(rhs.z);
    const double scale = std::max(std::fabs(x), std::max(std::fabs(y), std::fabs(z)));
    if (!std::isfinite(scale)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (scale == 0.0) {
        return 0.0;
    }
    const double scaledLengthSquared = (x / scale) * (x / scale) +
                                       (y / scale) * (y / scale) +
                                       (z / scale) * (z / scale);
    if (!std::isfinite(scaledLengthSquared)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const double length = scale * std::sqrt(scaledLengthSquared);
    return std::isfinite(length) ? length : std::numeric_limits<double>::quiet_NaN();
}

[[nodiscard]] bool TryNarrowFiniteVectorUVE(const double x, const double y, const double z,
                                             Math::Vector3UVE& outVector) noexcept {
    constexpr double kMaximumFloatUVE = static_cast<double>(std::numeric_limits<float>::max());
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) ||
        std::fabs(x) > kMaximumFloatUVE || std::fabs(y) > kMaximumFloatUVE ||
        std::fabs(z) > kMaximumFloatUVE) {
        return false;
    }
    outVector = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)};
    return IsFiniteVectorUVE(outVector);
}

[[nodiscard]] Math::Vector3UVE NormalizeProjectedPoleUVE(
    const Math::Vector3UVE& poleOffset, const Math::Vector3UVE& direction,
    const Math::Vector3UVE& fallback) noexcept {
    const double poleX = static_cast<double>(poleOffset.x);
    const double poleY = static_cast<double>(poleOffset.y);
    const double poleZ = static_cast<double>(poleOffset.z);
    const double directionX = static_cast<double>(direction.x);
    const double directionY = static_cast<double>(direction.y);
    const double directionZ = static_cast<double>(direction.z);
    const double dot = poleX * directionX + poleY * directionY + poleZ * directionZ;
    const double projectedX = poleX - directionX * dot;
    const double projectedY = poleY - directionY * dot;
    const double projectedZ = poleZ - directionZ * dot;
    const double scale = std::max(std::fabs(projectedX),
                                  std::max(std::fabs(projectedY), std::fabs(projectedZ)));
    if (!std::isfinite(dot) || !std::isfinite(scale) || scale == 0.0) {
        return fallback;
    }
    return NormalizeVectorUVE(
        {static_cast<float>(projectedX / scale), static_cast<float>(projectedY / scale),
         static_cast<float>(projectedZ / scale)},
        fallback);
}

[[nodiscard]] bool TryMakeBoneDirectionRotationUVE(const Math::Vector3UVE& direction,
                                                      Math::QuaternionUVE& outRotation) noexcept {
    const Math::Vector3UVE normalizedDirection = NormalizeVectorUVE(direction, {});
    if (Math::LengthSquaredUVE(normalizedDirection) <= kEpsilonUVE * kEpsilonUVE) {
        return false;
    }
    const Math::Vector3UVE boneAxis{1.0F, 0.0F, 0.0F};
    const float dot = std::clamp(Math::DotUVE(boneAxis, normalizedDirection), -1.0F, 1.0F);
    const Math::Vector3UVE cross = Math::CrossUVE(boneAxis, normalizedDirection);
    if (Math::LengthSquaredUVE(cross) <= kEpsilonUVE * kEpsilonUVE) {
        if (dot >= 0.0F) {
            outRotation = {};
            return true;
        }
        return Math::TryMakeAxisAngleUVE({0.0F, 1.0F, 0.0F}, 3.14159265358979323846F, outRotation);
    }
    return Math::TryMakeAxisAngleUVE(NormalizeVectorUVE(cross, {0.0F, 0.0F, 1.0F}),
                                       std::acos(dot), outRotation);
}

[[nodiscard]] bool TryClampAimLookAtAngleUVE(const Math::QuaternionUVE& rotation,
                                              const float minDegrees, const float maxDegrees,
                                              Math::QuaternionUVE& outRotation) noexcept {
    constexpr float kPiUVE = 3.14159265358979323846F;
    if (!Math::IsFiniteUVE(rotation) || !std::isfinite(minDegrees) || !std::isfinite(maxDegrees) ||
        minDegrees < 0.0F || maxDegrees < minDegrees || maxDegrees > 180.0F) {
        return false;
    }
    Math::QuaternionUVE normalized;
    if (!Math::TryNormalizeUVE(rotation, normalized)) {
        return false;
    }
    const float halfAngle = std::acos(std::clamp(std::fabs(normalized.w), 0.0F, 1.0F));
    const float angleDegrees = (2.0F * halfAngle * 180.0F) / kPiUVE;
    if (angleDegrees >= minDegrees && angleDegrees <= maxDegrees) {
        outRotation = normalized;
        return true;
    }
    const Math::Vector3UVE axis = NormalizeVectorUVE(
        {normalized.x, normalized.y, normalized.z}, {0.0F, 1.0F, 0.0F});
    const float clampedDegrees = std::clamp(angleDegrees, minDegrees, maxDegrees);
    return Math::TryMakeAxisAngleUVE(axis, clampedDegrees * kPiUVE / 180.0F, outRotation);
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
        if (!IsValidControlRigSpaceUVE(control.space)) {
            return {ControlRigValidationCodeUVE::InvalidControl, control.controlId,
                    "Control space is unknown."};
        }
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
        if (!IsValidControlRigConstraintKindUVE(constraint.kind)) {
            return {ControlRigValidationCodeUVE::InvalidConstraint, constraint.constraintId,
                    "Constraint kind is unknown."};
        }
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
        if (constraint.kind == ControlRigConstraintKindUVE::AimLookAt &&
            (!std::isfinite(constraint.minAimAngleDegrees) || !std::isfinite(constraint.maxAimAngleDegrees) ||
             constraint.minAimAngleDegrees < 0.0F || constraint.maxAimAngleDegrees < constraint.minAimAngleDegrees ||
             constraint.maxAimAngleDegrees > 180.0F)) {
            return {ControlRigValidationCodeUVE::InvalidConstraint, constraint.constraintId,
                    "Aim/look-at angle limits must be finite and ordered within 0 to 180 degrees."};
        }
        if (std::find(constraintIds.begin(), constraintIds.end(), constraint.constraintId) != constraintIds.end()) {
            return {ControlRigValidationCodeUVE::DuplicateConstraint, constraint.constraintId,
                    "Constraint identifiers must be unique."};
        }
        constraintIds.push_back(constraint.constraintId);
        const bool isTwoBone = constraint.kind == ControlRigConstraintKindUVE::TwoBoneIK;
        const bool isAimLookAt = constraint.kind == ControlRigConstraintKindUVE::AimLookAt;
        const bool endpointsValid = IsIdentifierUVE(constraint.sourceControlId) &&
            (isTwoBone ? IsIdentifierUVE(constraint.midControlId) && IsIdentifierUVE(constraint.endControlId)
                       : true) && IsIdentifierUVE(constraint.targetControlId);
        if (!endpointsValid || FindControlUVE(rig.controls, constraint.sourceControlId) == nullptr ||
            FindControlUVE(rig.controls, constraint.targetControlId) == nullptr ||
            (isTwoBone && (FindControlUVE(rig.controls, constraint.midControlId) == nullptr ||
                           FindControlUVE(rig.controls, constraint.endControlId) == nullptr)) ||
            ((isTwoBone || isAimLookAt) && (!constraint.poleControlId.empty() &&
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
        !IsFiniteTransformPoseUVE(endPose) || !IsFiniteVectorUVE(target) || !IsFiniteVectorUVE(pole) ||
        !std::isfinite(weight)) {
        return result;
    }
    const Math::Vector3UVE root = rootPose.position;
    const double firstLength = LengthDifferenceUVE(midPose.position, root);
    const double secondLength = LengthDifferenceUVE(endPose.position, midPose.position);
    if (!std::isfinite(firstLength) || !std::isfinite(secondLength) ||
        firstLength <= static_cast<double>(kEpsilonUVE) || secondLength <= static_cast<double>(kEpsilonUVE)) {
        return result;
    }
    const double requestedDistance = LengthDifferenceUVE(target, root);
    if (!std::isfinite(requestedDistance)) {
        return result;
    }
    const double minimumDistance = std::abs(firstLength - secondLength) + static_cast<double>(kEpsilonUVE);
    const double maximumDistance = firstLength + secondLength - static_cast<double>(kEpsilonUVE);
    if (!std::isfinite(minimumDistance) || !std::isfinite(maximumDistance) ||
        minimumDistance > maximumDistance || maximumDistance <= static_cast<double>(kEpsilonUVE)) {
        return result;
    }
    const double solvedDistance = std::clamp(requestedDistance, minimumDistance, maximumDistance);
    const bool reachable = requestedDistance >= minimumDistance && requestedDistance <= maximumDistance;
    const bool targetClamped = !reachable;
    const Math::Vector3UVE fallbackDirection{1.0F, 0.0F, 0.0F};
    const Math::Vector3UVE direction = NormalizeDifferenceVectorUVE(target, root, fallbackDirection);
    const Math::Vector3UVE poleOffset = pole - root;
    if (!IsFiniteVectorUVE(poleOffset)) {
        return result;
    }
    const Math::Vector3UVE bendDirection = NormalizeProjectedPoleUVE(
        poleOffset, direction, {0.0F, 1.0F, 0.0F});
    const double cosine = std::clamp((firstLength * firstLength + solvedDistance * solvedDistance -
                                      secondLength * secondLength) /
                                         (2.0 * firstLength * solvedDistance), -1.0, 1.0);
    const double along = firstLength * cosine;
    const double bend = firstLength * std::sqrt(std::max(0.0, 1.0 - cosine * cosine));
    if (!std::isfinite(cosine) || !std::isfinite(along) || !std::isfinite(bend)) {
        return result;
    }
    Math::Vector3UVE solvedMid;
    Math::Vector3UVE solvedEnd;
    if (!TryNarrowFiniteVectorUVE(
            static_cast<double>(root.x) + static_cast<double>(direction.x) * along +
                static_cast<double>(bendDirection.x) * bend,
            static_cast<double>(root.y) + static_cast<double>(direction.y) * along +
                static_cast<double>(bendDirection.y) * bend,
            static_cast<double>(root.z) + static_cast<double>(direction.z) * along +
                static_cast<double>(bendDirection.z) * bend,
            solvedMid) ||
        !TryNarrowFiniteVectorUVE(
            static_cast<double>(root.x) + static_cast<double>(direction.x) * solvedDistance,
            static_cast<double>(root.y) + static_cast<double>(direction.y) * solvedDistance,
            static_cast<double>(root.z) + static_cast<double>(direction.z) * solvedDistance,
            solvedEnd)) {
        return result;
    }
    TransformPoseUVE solvedMidPose = midPose;
    solvedMidPose.position = solvedMid;
    TransformPoseUVE solvedEndPose = endPose;
    solvedEndPose.position = solvedEnd;
    Math::QuaternionUVE rootRotation;
    Math::QuaternionUVE midRotation;
    if (TryMakeBoneDirectionRotationUVE(solvedMid - root, rootRotation)) {
        TransformPoseUVE solvedRootPose = rootPose;
        solvedRootPose.rotation = rootRotation;
        result.rootPose = BlendControlRigPoseUVE(rootPose, solvedRootPose, weight);
    }
    if (TryMakeBoneDirectionRotationUVE(solvedEnd - solvedMid, midRotation)) {
        solvedMidPose.rotation = midRotation;
    }
    result.midPose = BlendControlRigPoseUVE(midPose, solvedMidPose, weight);
    result.endPose = BlendControlRigPoseUVE(endPose, solvedEndPose, weight);
    result.reachable = reachable;
    result.targetClamped = targetClamped;
    return result;
}

bool TryMakeAimLookAtRotationUVE(const Math::Vector3UVE& source, const Math::Vector3UVE& target,
                                 const Math::Vector3UVE& up,
                                 Math::QuaternionUVE& outRotation) noexcept {
    const Math::Vector3UVE forward{0.0F, 0.0F, -1.0F};
    const Math::Vector3UVE direction = NormalizeDifferenceVectorUVE(target, source, {});
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
                Math::Vector3UVE up = {0.0F, 1.0F, 0.0F};
                if (!constraint.poleControlId.empty()) {
                    const auto pole = findMutable(constraint.poleControlId);
                    if (pole != result.controls.end()) {
                        const Math::Vector3UVE poleDirection =
                            NormalizeDifferenceVectorUVE(pole->pose.position, source->pose.position, {});
                        if (Math::LengthSquaredUVE(poleDirection) > kEpsilonUVE * kEpsilonUVE) {
                            up = Math::NormalizeUVE(poleDirection);
                        }
                    }
                }
                if (!TryMakeAimLookAtRotationUVE(source->pose.position, target->pose.position, up, rotation)) {
                    result.message = "Control Rig aim/look-at solve failed.";
                    return result;
                }
                if (!TryClampAimLookAtAngleUVE(rotation, constraint.minAimAngleDegrees,
                                               constraint.maxAimAngleDegrees, rotation)) {
                    result.message = "Control Rig aim/look-at angle limit is invalid.";
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
