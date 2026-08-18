// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/core/time_pose_contract_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace UVE::Core {

enum class ControlRigSpaceUVE : std::uint8_t {
    Local = 0,
    World,
};

struct ControlRigControlUVE final {
    std::string controlId;
    std::string parentControlId;
    ControlRigSpaceUVE space = ControlRigSpaceUVE::Local;
    TransformPoseUVE pose;
    bool enabled = true;
};

enum class ControlRigConstraintKindUVE : std::uint8_t {
    TwoBoneIK = 0,
    AimLookAt,
};

struct ControlRigConstraintUVE final {
    std::string constraintId;
    ControlRigConstraintKindUVE kind = ControlRigConstraintKindUVE::TwoBoneIK;
    std::string sourceControlId;
    std::string midControlId;
    std::string endControlId;
    std::string targetControlId;
    std::string poleControlId;
    float weight = 1.0F;
};

struct ControlRigUVE final {
    static constexpr std::size_t kMaximumControlsUVE = 256U;
    static constexpr std::size_t kMaximumConstraintsUVE = 512U;

    std::vector<ControlRigControlUVE> controls;
    std::vector<ControlRigConstraintUVE> constraints;
    SkeletonDefinitionUVE skeleton;
    PoseBufferUVE pose;
    AnimationEvaluationContextUVE evaluationContext;
};

enum class ControlRigValidationCodeUVE : std::uint8_t {
    Valid = 0,
    EmptyRig,
    CapacityExceeded,
    InvalidControl,
    DuplicateControl,
    UnknownControl,
    InvalidConstraint,
    DuplicateConstraint,
    InvalidPose,
    InvalidSkeleton,
    InvalidEvaluationTime,
};

struct ControlRigValidationResultUVE final {
    ControlRigValidationCodeUVE code = ControlRigValidationCodeUVE::EmptyRig;
    std::string identifier;
    std::string message;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return code == ControlRigValidationCodeUVE::Valid;
    }
};

struct TwoBoneIKSolveResultUVE final {
    TransformPoseUVE rootPose;
    TransformPoseUVE midPose;
    TransformPoseUVE endPose;
    bool reachable = false;
    bool targetClamped = false;

    [[nodiscard]] bool IsSuccessUVE() const noexcept {
        return reachable || targetClamped;
    }
};

struct ControlRigEvaluationResultUVE final {
    std::vector<ControlRigControlUVE> controls;
    SkeletonDefinitionUVE skeleton;
    PoseBufferUVE pose;
    AnimationEvaluationContextUVE evaluationContext;
    std::size_t appliedConstraintCount = 0U;
    bool evaluated = false;
    std::string message;

    [[nodiscard]] bool IsSuccessUVE() const noexcept {
        return evaluated;
    }
};

[[nodiscard]] ControlRigValidationResultUVE ValidateControlRigUVE(
    const ControlRigUVE& rig) noexcept;

[[nodiscard]] TransformPoseUVE BlendControlRigPoseUVE(
    const TransformPoseUVE& source, const TransformPoseUVE& target, float weight) noexcept;

[[nodiscard]] TwoBoneIKSolveResultUVE SolveTwoBoneIKUVE(
    const TransformPoseUVE& rootPose, const TransformPoseUVE& midPose,
    const TransformPoseUVE& endPose, const Math::Vector3UVE& target,
    const Math::Vector3UVE& pole, float weight) noexcept;

[[nodiscard]] bool TryMakeAimLookAtRotationUVE(
    const Math::Vector3UVE& source, const Math::Vector3UVE& target,
    const Math::Vector3UVE& up, Math::QuaternionUVE& outRotation) noexcept;

[[nodiscard]] ControlRigEvaluationResultUVE EvaluateControlRigUVE(const ControlRigUVE& rig);

} // namespace UVE::Core
