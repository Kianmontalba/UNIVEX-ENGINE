// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/plugins/control_rig_autorig_uve.h"

#include <algorithm>
#include <cmath>
#include <string_view>
#include <utility>

namespace UVE::Core {
namespace {

constexpr float kMinimumShapeValueUVE = 1.0e-4F;

[[nodiscard]] bool IsIdentifierUVE(const std::string& value,
                                   const std::size_t maximumBytes) noexcept {
    return !value.empty() && value.size() <= maximumBytes &&
           value.find('\0') == std::string::npos;
}

[[nodiscard]] bool IsFinitePositiveUVE(const float value) noexcept {
    return std::isfinite(value) && value > kMinimumShapeValueUVE;
}

[[nodiscard]] bool HasSuffixUVE(const std::string& value, const std::string_view suffix) noexcept {
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

[[nodiscard]] std::size_t FindJointIndexUVE(const SkeletonDefinitionUVE& skeleton,
                                             const std::string& jointId) noexcept {
    for (std::size_t index = 0U; index < skeleton.joints.size(); ++index) {
        if (skeleton.joints[index].jointId == jointId) {
            return index;
        }
    }
    return skeleton.joints.size();
}

[[nodiscard]] const TransformPoseUVE* FindReferencePoseUVE(
    const SkeletonDefinitionUVE& skeleton, const PoseBufferUVE& pose,
    const std::string& jointId) noexcept {
    const std::size_t index = FindJointIndexUVE(skeleton, jointId);
    return index < pose.localJoints.size() ? &pose.localJoints[index] : nullptr;
}

[[nodiscard]] ControlRigAutoRigSideUVE DetectSideUVE(const std::string& jointId) noexcept {
    if (jointId.find(".L") != std::string::npos || HasSuffixUVE(jointId, "_L") ||
        HasSuffixUVE(jointId, "_left")) {
        return ControlRigAutoRigSideUVE::Left;
    }
    if (jointId.find(".R") != std::string::npos || HasSuffixUVE(jointId, "_R") ||
        HasSuffixUVE(jointId, "_right")) {
        return ControlRigAutoRigSideUVE::Right;
    }
    return ControlRigAutoRigSideUVE::Center;
}

[[nodiscard]] std::string SideSuffixUVE(const ControlRigAutoRigSideUVE side) {
    switch (side) {
    case ControlRigAutoRigSideUVE::Center:
        return {};
    case ControlRigAutoRigSideUVE::Left:
        return "_l";
    case ControlRigAutoRigSideUVE::Right:
        return "_r";
    }
    return {};
}

[[nodiscard]] std::string MakeControlIdUVE(const ControlRigAutoRigRoleUVE role,
                                            const ControlRigAutoRigSideUVE side,
                                            const std::string& jointId) {
    const std::string suffix = SideSuffixUVE(side);
    switch (role) {
    case ControlRigAutoRigRoleUVE::Root:
        return "ctrl_root";
    case ControlRigAutoRigRoleUVE::Spine:
        return "ctrl_spine";
    case ControlRigAutoRigRoleUVE::Head:
        return "ctrl_head";
    case ControlRigAutoRigRoleUVE::HandIK:
        return "ctrl_hand_ik" + suffix;
    case ControlRigAutoRigRoleUVE::HandPole:
        return "ctrl_hand_pole" + suffix;
    case ControlRigAutoRigRoleUVE::FootIK:
        return "ctrl_foot_ik" + suffix;
    case ControlRigAutoRigRoleUVE::FootPole:
        return "ctrl_foot_pole" + suffix;
    case ControlRigAutoRigRoleUVE::Deform:
        return "bone." + jointId;
    }
    return {};
}

[[nodiscard]] ControlRigControlShapeUVE MakeShapeUVE(const ControlRigAutoRigRoleUVE role) noexcept {
    switch (role) {
    case ControlRigAutoRigRoleUVE::Root:
        return {ControlRigControlShapeKindUVE::Box, 0.75F, 0.08F, 1.5F};
    case ControlRigAutoRigRoleUVE::Spine:
    case ControlRigAutoRigRoleUVE::Head:
        return {ControlRigControlShapeKindUVE::Box, 0.45F, 0.06F, 0.9F};
    case ControlRigAutoRigRoleUVE::HandIK:
    case ControlRigAutoRigRoleUVE::FootIK:
        return {ControlRigControlShapeKindUVE::Circle, 0.3F, 0.05F, 0.6F};
    case ControlRigAutoRigRoleUVE::HandPole:
    case ControlRigAutoRigRoleUVE::FootPole:
        return {ControlRigControlShapeKindUVE::Arrow, 0.25F, 0.05F, 0.75F};
    case ControlRigAutoRigRoleUVE::Deform:
        return {ControlRigControlShapeKindUVE::Box, 0.1F, 0.02F, 0.2F};
    }
    return {};
}

[[nodiscard]] TransformPoseUVE MakeControlPoseUVE(const TransformPoseUVE& jointPose,
                                                   const ControlRigAutoRigRoleUVE role,
                                                   const ControlRigAutoRigSideUVE side) noexcept {
    TransformPoseUVE pose = jointPose;
    if (role == ControlRigAutoRigRoleUVE::HandPole || role == ControlRigAutoRigRoleUVE::FootPole) {
        const float sideSign = side == ControlRigAutoRigSideUVE::Right ? 1.0F : -1.0F;
        pose.position.y += 0.5F;
        pose.position.x += sideSign * 0.35F;
    }
    return pose;
}

[[nodiscard]] bool IsValidShapeUVE(const ControlRigControlShapeUVE& shape) noexcept {
    switch (shape.kind) {
    case ControlRigControlShapeKindUVE::Box:
    case ControlRigControlShapeKindUVE::Circle:
    case ControlRigControlShapeKindUVE::Arrow:
        break;
    default:
        return false;
    }
    return IsFinitePositiveUVE(shape.size) && IsFinitePositiveUVE(shape.thickness) &&
           IsFinitePositiveUVE(shape.length) && shape.thickness <= shape.size;
}

[[nodiscard]] const ControlRigControlUVE* FindRuntimeControlUVE(
    const std::vector<ControlRigControlUVE>& controls, const std::string& controlId) noexcept {
    const auto iterator = std::find_if(controls.cbegin(), controls.cend(),
                                       [&controlId](const auto& control) {
                                           return control.controlId == controlId;
                                       });
    return iterator == controls.cend() ? nullptr : &*iterator;
}

[[nodiscard]] std::string SwapSideSuffixUVE(const std::string& value) {
    std::string result = value;
    const auto swapSuffix = [&result](const std::string_view left, const std::string_view right) {
        if (HasSuffixUVE(result, left)) {
            result.replace(result.size() - left.size(), left.size(), right);
            return true;
        }
        return false;
    };
    if (swapSuffix("_l", "_r") || swapSuffix(".L", ".R")) {
        return result;
    }
    if (swapSuffix("_r", "_l") || swapSuffix(".R", ".L")) {
        return result;
    }
    return result;
}

[[nodiscard]] Math::QuaternionUVE MirrorRotationAcrossXUVE(
    const Math::QuaternionUVE& rotation) noexcept {
    Math::QuaternionUVE mirrored{rotation.x, -rotation.y, -rotation.z, rotation.w};
    Math::QuaternionUVE normalized;
    return Math::TryNormalizeUVE(mirrored, normalized) ? normalized : rotation;
}

[[nodiscard]] bool IsSameRoleBindingUVE(const ControlRigRoleBindingUVE& lhs,
                                         const ControlRigRoleBindingUVE& rhs) noexcept {
    return lhs.role == rhs.role && lhs.side == rhs.side && lhs.jointId == rhs.jointId;
}

[[nodiscard]] bool IsValidRoleUVE(const ControlRigAutoRigRoleUVE role) noexcept {
    switch (role) {
    case ControlRigAutoRigRoleUVE::Deform:
    case ControlRigAutoRigRoleUVE::Root:
    case ControlRigAutoRigRoleUVE::Spine:
    case ControlRigAutoRigRoleUVE::Head:
    case ControlRigAutoRigRoleUVE::HandIK:
    case ControlRigAutoRigRoleUVE::HandPole:
    case ControlRigAutoRigRoleUVE::FootIK:
    case ControlRigAutoRigRoleUVE::FootPole:
        return true;
    }
    return false;
}

[[nodiscard]] bool IsValidSideUVE(const ControlRigAutoRigSideUVE side) noexcept {
    switch (side) {
    case ControlRigAutoRigSideUVE::Center:
    case ControlRigAutoRigSideUVE::Left:
    case ControlRigAutoRigSideUVE::Right:
        return true;
    }
    return false;
}

} // namespace

bool BuildControlRigAutoRigUVE(const ControlRigAutoRigRequestUVE& request,
                               ControlRigAutoRigUVE& outRig) noexcept {
    if (!IsIdentifierUVE(request.rigId, ControlRigAutoRigRequestUVE::kMaximumRigIdentifierBytesUVE)) {
        return false;
    }
    if (!ValidateSkeletonDefinitionUVE(request.skeleton).IsValidUVE() ||
        !ValidatePoseBufferUVE(request.referencePose, request.skeleton).IsValidUVE() ||
        request.skeleton.joints.empty() ||
        request.skeleton.joints.size() > ControlRigAutoRigUVE::kMaximumGeneratedControlsUVE) {
        return false;
    }

    ControlRigAutoRigUVE candidate;
    candidate.rigId = request.rigId;
    candidate.skeleton = request.skeleton;
    candidate.referencePose = request.referencePose;
    candidate.runtimeRig.skeleton = request.skeleton;
    candidate.runtimeRig.pose = request.referencePose;

    const auto appendControl = [&candidate, &request](const std::string& controlId,
                                                       const std::string& parentControlId,
                                                       const ControlRigAutoRigRoleUVE role,
                                                       const ControlRigAutoRigSideUVE side,
                                                       const std::string& drivenJointId,
                                                       const TransformPoseUVE& pose,
                                                       const bool visible,
                                                       const bool selectable) {
        const ControlRigControlShapeUVE shape = MakeShapeUVE(role);
        const ControlRigGeneratedControlUVE generated{
            ControlRigControlUVE{controlId, parentControlId, ControlRigSpaceUVE::World, pose, true},
            role, side, drivenJointId, shape, visible, selectable};
        candidate.generatedControls.push_back(generated);
        candidate.roleBindings.push_back({role, side, drivenJointId});
        candidate.runtimeRig.controls.push_back(generated.control);
        (void)request;
    };

    for (const SkeletonJointUVE& joint : request.skeleton.joints) {
        const TransformPoseUVE* const pose = FindReferencePoseUVE(
            request.skeleton, request.referencePose, joint.jointId);
        if (pose == nullptr) {
            return false;
        }
        const std::string parentControlId = joint.parentJointId.empty()
            ? std::string{} : MakeControlIdUVE(ControlRigAutoRigRoleUVE::Deform,
                                                ControlRigAutoRigSideUVE::Center, joint.parentJointId);
        appendControl(MakeControlIdUVE(ControlRigAutoRigRoleUVE::Deform,
                                       ControlRigAutoRigSideUVE::Center, joint.jointId),
                      parentControlId, ControlRigAutoRigRoleUVE::Deform,
                      ControlRigAutoRigSideUVE::Center, joint.jointId, *pose, false, false);
    }

    const std::size_t rootIndex = FindJointIndexUVE(request.skeleton, request.jointMap.rootJointId);
    if (rootIndex == request.skeleton.joints.size()) {
        return false;
    }
    appendControl("ctrl_root", {}, ControlRigAutoRigRoleUVE::Root,
                  ControlRigAutoRigSideUVE::Center, request.jointMap.rootJointId,
                  request.referencePose.localJoints[rootIndex], true, true);

    const std::size_t spineIndex = FindJointIndexUVE(request.skeleton, request.jointMap.spineJointId);
    if (request.generateSpineControl && spineIndex < request.skeleton.joints.size()) {
        appendControl("ctrl_spine", "ctrl_root", ControlRigAutoRigRoleUVE::Spine,
                      ControlRigAutoRigSideUVE::Center, request.jointMap.spineJointId,
                      request.referencePose.localJoints[spineIndex], true, true);
    }

    const std::size_t headIndex = FindJointIndexUVE(request.skeleton, request.jointMap.headJointId);
    if (request.generateHeadControl && headIndex < request.skeleton.joints.size()) {
        appendControl("ctrl_head",
                      request.generateSpineControl && spineIndex < request.skeleton.joints.size()
                          ? "ctrl_spine" : "ctrl_root",
                      ControlRigAutoRigRoleUVE::Head, ControlRigAutoRigSideUVE::Center,
                      request.jointMap.headJointId,
                      request.referencePose.localJoints[headIndex], true, true);
    }

    const auto appendLimb = [&appendControl, &request](const std::string& upperName,
                                                        const std::string& lowerName,
                                                        const std::string& endName,
                                                        const ControlRigAutoRigRoleUVE ikRole,
                                                        const ControlRigAutoRigRoleUVE poleRole) {
        const std::size_t upperIndex = FindJointIndexUVE(request.skeleton, upperName);
        const std::size_t lowerIndex = FindJointIndexUVE(request.skeleton, lowerName);
        const std::size_t endIndex = FindJointIndexUVE(request.skeleton, endName);
        if (upperIndex == request.skeleton.joints.size() || lowerIndex == request.skeleton.joints.size() ||
            endIndex == request.skeleton.joints.size()) {
            return false;
        }
        const ControlRigAutoRigSideUVE side = DetectSideUVE(endName);
        const std::string suffix = SideSuffixUVE(side);
        const std::string ikId = ikRole == ControlRigAutoRigRoleUVE::HandIK
            ? "ctrl_hand_ik" + suffix : "ctrl_foot_ik" + suffix;
        const std::string poleId = poleRole == ControlRigAutoRigRoleUVE::HandPole
            ? "ctrl_hand_pole" + suffix : "ctrl_foot_pole" + suffix;
        appendControl(ikId, "ctrl_root", ikRole, side, endName,
                      MakeControlPoseUVE(request.referencePose.localJoints[endIndex], ikRole, side), true, true);
        appendControl(poleId, "ctrl_root", poleRole, side, lowerName,
                      MakeControlPoseUVE(request.referencePose.localJoints[lowerIndex], poleRole, side), true, true);
        return true;
    };

    if (!appendLimb(request.jointMap.leftUpperArmJointId, request.jointMap.leftForearmJointId,
                    request.jointMap.leftHandJointId, ControlRigAutoRigRoleUVE::HandIK,
                    ControlRigAutoRigRoleUVE::HandPole) ||
        !appendLimb(request.jointMap.rightUpperArmJointId, request.jointMap.rightForearmJointId,
                    request.jointMap.rightHandJointId, ControlRigAutoRigRoleUVE::HandIK,
                    ControlRigAutoRigRoleUVE::HandPole) ||
        !appendLimb(request.jointMap.leftThighJointId, request.jointMap.leftShinJointId,
                    request.jointMap.leftFootJointId, ControlRigAutoRigRoleUVE::FootIK,
                    ControlRigAutoRigRoleUVE::FootPole) ||
        !appendLimb(request.jointMap.rightThighJointId, request.jointMap.rightShinJointId,
                    request.jointMap.rightFootJointId, ControlRigAutoRigRoleUVE::FootIK,
                    ControlRigAutoRigRoleUVE::FootPole)) {
        return false;
    }

    candidate.runtimeRig.constraints = {
        {"hand_ik_l", ControlRigConstraintKindUVE::TwoBoneIK,
         MakeControlIdUVE(ControlRigAutoRigRoleUVE::Deform, ControlRigAutoRigSideUVE::Center,
                          request.jointMap.leftUpperArmJointId),
         MakeControlIdUVE(ControlRigAutoRigRoleUVE::Deform, ControlRigAutoRigSideUVE::Center,
                          request.jointMap.leftForearmJointId),
         MakeControlIdUVE(ControlRigAutoRigRoleUVE::Deform, ControlRigAutoRigSideUVE::Center,
                          request.jointMap.leftHandJointId),
         "ctrl_hand_ik_l", "ctrl_hand_pole_l", 1.0F},
        {"hand_ik_r", ControlRigConstraintKindUVE::TwoBoneIK,
         MakeControlIdUVE(ControlRigAutoRigRoleUVE::Deform, ControlRigAutoRigSideUVE::Center,
                          request.jointMap.rightUpperArmJointId),
         MakeControlIdUVE(ControlRigAutoRigRoleUVE::Deform, ControlRigAutoRigSideUVE::Center,
                          request.jointMap.rightForearmJointId),
         MakeControlIdUVE(ControlRigAutoRigRoleUVE::Deform, ControlRigAutoRigSideUVE::Center,
                          request.jointMap.rightHandJointId),
         "ctrl_hand_ik_r", "ctrl_hand_pole_r", 1.0F},
        {"foot_ik_l", ControlRigConstraintKindUVE::TwoBoneIK,
         MakeControlIdUVE(ControlRigAutoRigRoleUVE::Deform, ControlRigAutoRigSideUVE::Center,
                          request.jointMap.leftThighJointId),
         MakeControlIdUVE(ControlRigAutoRigRoleUVE::Deform, ControlRigAutoRigSideUVE::Center,
                          request.jointMap.leftShinJointId),
         MakeControlIdUVE(ControlRigAutoRigRoleUVE::Deform, ControlRigAutoRigSideUVE::Center,
                          request.jointMap.leftFootJointId),
         "ctrl_foot_ik_l", "ctrl_foot_pole_l", 1.0F},
        {"foot_ik_r", ControlRigConstraintKindUVE::TwoBoneIK,
         MakeControlIdUVE(ControlRigAutoRigRoleUVE::Deform, ControlRigAutoRigSideUVE::Center,
                          request.jointMap.rightThighJointId),
         MakeControlIdUVE(ControlRigAutoRigRoleUVE::Deform, ControlRigAutoRigSideUVE::Center,
                          request.jointMap.rightShinJointId),
         MakeControlIdUVE(ControlRigAutoRigRoleUVE::Deform, ControlRigAutoRigSideUVE::Center,
                          request.jointMap.rightFootJointId),
         "ctrl_foot_ik_r", "ctrl_foot_pole_r", 1.0F},
    };
    candidate.referenceGeneratedControls = candidate.generatedControls;
    candidate.referenceRuntimeControls = candidate.runtimeRig.controls;
    if (!ValidateControlRigAutoRigUVE(candidate).IsValidUVE()) {
        return false;
    }
    outRig = std::move(candidate);
    return true;
}

ControlRigAutoRigValidationResultUVE ValidateControlRigAutoRigUVE(
    const ControlRigAutoRigUVE& rig) noexcept {
    if (!IsIdentifierUVE(rig.rigId, ControlRigAutoRigRequestUVE::kMaximumRigIdentifierBytesUVE)) {
        return {ControlRigAutoRigValidationCodeUVE::InvalidIdentifier, rig.rigId,
                "Auto Rig identity is invalid."};
    }
    if (!ValidateSkeletonDefinitionUVE(rig.skeleton).IsValidUVE()) {
        return {ControlRigAutoRigValidationCodeUVE::InvalidSkeleton, rig.rigId,
                "Auto Rig skeleton is invalid."};
    }
    if (!ValidatePoseBufferUVE(rig.referencePose, rig.skeleton).IsValidUVE()) {
        return {ControlRigAutoRigValidationCodeUVE::InvalidReferencePose, rig.rigId,
                "Auto Rig reference pose is invalid."};
    }
    if (rig.generatedControls.empty() ||
        rig.generatedControls.size() > ControlRigAutoRigUVE::kMaximumGeneratedControlsUVE ||
        rig.roleBindings.size() > ControlRigAutoRigUVE::kMaximumRoleBindingsUVE ||
        rig.referenceGeneratedControls.size() != rig.generatedControls.size() ||
        rig.referenceRuntimeControls.size() != rig.runtimeRig.controls.size()) {
        return {ControlRigAutoRigValidationCodeUVE::CapacityExceeded, rig.rigId,
                "Auto Rig generated data exceeds its bounded contract."};
    }

    for (std::size_t index = 0U; index < rig.roleBindings.size(); ++index) {
        const ControlRigRoleBindingUVE& binding = rig.roleBindings[index];
        if (!IsValidRoleUVE(binding.role) || !IsValidSideUVE(binding.side) ||
            FindJointIndexUVE(rig.skeleton, binding.jointId) == rig.skeleton.joints.size()) {
            return {ControlRigAutoRigValidationCodeUVE::UnknownDrivenJoint, binding.jointId,
                    "Auto Rig role binding references an unknown joint."};
        }
        for (std::size_t prior = 0U; prior < index; ++prior) {
            if (IsSameRoleBindingUVE(binding, rig.roleBindings[prior])) {
                return {ControlRigAutoRigValidationCodeUVE::DuplicateRoleBinding, binding.jointId,
                        "Auto Rig role bindings must be unique."};
            }
        }
    }

    for (std::size_t index = 0U; index < rig.generatedControls.size(); ++index) {
        const ControlRigGeneratedControlUVE& generated = rig.generatedControls[index];
        if (!IsValidRoleUVE(generated.role) || !IsValidSideUVE(generated.side) ||
            !IsIdentifierUVE(generated.drivenJointId, kMaximumAnimationIdentifierBytesUVE) ||
            FindJointIndexUVE(rig.skeleton, generated.drivenJointId) == rig.skeleton.joints.size() ||
            !IsValidShapeUVE(generated.shape) || !IsFiniteTransformPoseUVE(generated.control.pose)) {
            return {IsValidShapeUVE(generated.shape)
                        ? ControlRigAutoRigValidationCodeUVE::UnknownDrivenJoint
                        : ControlRigAutoRigValidationCodeUVE::InvalidShape,
                    generated.control.controlId, "Generated Auto Rig control is invalid."};
        }
        if (!IsIdentifierUVE(generated.control.controlId, kMaximumAnimationIdentifierBytesUVE)) {
            return {ControlRigAutoRigValidationCodeUVE::InvalidGeneratedControl,
                    generated.control.controlId, "Generated control identity is invalid."};
        }
        for (std::size_t prior = 0U; prior < index; ++prior) {
            if (generated.control.controlId == rig.generatedControls[prior].control.controlId) {
                return {ControlRigAutoRigValidationCodeUVE::DuplicateGeneratedControl,
                        generated.control.controlId, "Generated control identifiers must be unique."};
            }
        }
    }
    if (rig.runtimeRig.skeleton != rig.skeleton || rig.runtimeRig.pose != rig.referencePose ||
        !ValidateControlRigUVE(rig.runtimeRig).IsValidUVE()) {
        return {ControlRigAutoRigValidationCodeUVE::InvalidRuntimeRig, rig.rigId,
                "Auto Rig runtime rig is invalid or out of sync."};
    }
    return {ControlRigAutoRigValidationCodeUVE::Valid, {}, "Auto Rig is valid."};
}

bool ResetControlRigAutoRigUVE(ControlRigAutoRigUVE& rig) noexcept {
    if (!ValidateControlRigAutoRigUVE(rig).IsValidUVE() ||
        rig.referenceGeneratedControls.size() != rig.generatedControls.size() ||
        rig.referenceRuntimeControls.size() != rig.runtimeRig.controls.size()) {
        return false;
    }
    rig.generatedControls = rig.referenceGeneratedControls;
    rig.runtimeRig.controls = rig.referenceRuntimeControls;
    return true;
}

bool MirrorControlRigControlsUVE(const ControlRigAutoRigUVE& rig,
                                 const std::vector<ControlRigControlUVE>& controls,
                                 std::vector<ControlRigControlUVE>& outMirroredControls) noexcept {
    if (!ValidateControlRigAutoRigUVE(rig).IsValidUVE() || controls.size() > rig.runtimeRig.controls.size()) {
        return false;
    }
    std::vector<ControlRigControlUVE> mirrored;
    mirrored.reserve(controls.size());
    for (const ControlRigControlUVE& control : controls) {
        ControlRigControlUVE copy = control;
        copy.controlId = SwapSideSuffixUVE(control.controlId);
        copy.parentControlId = SwapSideSuffixUVE(control.parentControlId);
        copy.pose.position.x = -copy.pose.position.x;
        copy.pose.rotation = MirrorRotationAcrossXUVE(copy.pose.rotation);
        mirrored.push_back(std::move(copy));
    }
    outMirroredControls = std::move(mirrored);
    return true;
}

ControlRigBakeResultUVE BakeControlRigPoseToSkeletonUVE(
    const ControlRigAutoRigUVE& rig, const ControlRigUVE& evaluatedRig) noexcept {
    ControlRigBakeResultUVE result;
    if (!ValidateControlRigAutoRigUVE(rig).IsValidUVE() ||
        !ValidateControlRigUVE(evaluatedRig).IsValidUVE() ||
        evaluatedRig.skeleton != rig.skeleton || evaluatedRig.pose.skeletonId != rig.skeleton.skeletonId) {
        result.message = "Auto Rig bake inputs are invalid or skeleton-incompatible.";
        return result;
    }

    PoseBufferUVE baked = rig.referencePose;
    for (std::size_t jointIndex = 0U; jointIndex < rig.skeleton.joints.size(); ++jointIndex) {
        const std::string controlId = MakeControlIdUVE(
            ControlRigAutoRigRoleUVE::Deform, ControlRigAutoRigSideUVE::Center,
            rig.skeleton.joints[jointIndex].jointId);
        const ControlRigControlUVE* const control = FindRuntimeControlUVE(evaluatedRig.controls, controlId);
        if (control == nullptr || !IsFiniteTransformPoseUVE(control->pose)) {
            result.message = "Auto Rig bake could not map every deform joint.";
            return result;
        }
        baked.localJoints[jointIndex] = control->pose;
        ++result.mappedJointCount;
    }
    const AnimationContractValidationResultUVE validation =
        ValidatePoseBufferUVE(baked, rig.skeleton);
    if (!validation.IsValidUVE()) {
        result.mappedJointCount = 0U;
        result.message = "Baked Auto Rig pose failed shared pose validation.";
        return result;
    }
    result.pose = std::move(baked);
    result.baked = true;
    result.message = "Auto Rig pose baked to the deform skeleton.";
    return result;
}

} // namespace UVE::Core
