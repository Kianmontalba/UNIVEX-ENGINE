// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/plugins/control_rig_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace UVE::Core {

enum class ControlRigAutoRigSideUVE : std::uint8_t {
    Center = 0,
    Left,
    Right,
};

enum class ControlRigAutoRigRoleUVE : std::uint8_t {
    Deform = 0,
    Root,
    Spine,
    Head,
    HandIK,
    HandPole,
    FootIK,
    FootPole,
};

enum class ControlRigControlShapeKindUVE : std::uint8_t {
    Box = 0,
    Circle,
    Arrow,
};

struct ControlRigControlShapeUVE final {
    ControlRigControlShapeKindUVE kind = ControlRigControlShapeKindUVE::Box;
    float size = 0.25F;
    float thickness = 0.025F;
    float length = 0.5F;

    [[nodiscard]] bool operator==(const ControlRigControlShapeUVE&) const noexcept = default;
};

struct ControlRigRoleBindingUVE final {
    ControlRigAutoRigRoleUVE role = ControlRigAutoRigRoleUVE::Deform;
    ControlRigAutoRigSideUVE side = ControlRigAutoRigSideUVE::Center;
    std::string jointId;

    [[nodiscard]] bool operator==(const ControlRigRoleBindingUVE&) const noexcept = default;
};

struct ControlRigGeneratedControlUVE final {
    ControlRigControlUVE control;
    ControlRigAutoRigRoleUVE role = ControlRigAutoRigRoleUVE::Deform;
    ControlRigAutoRigSideUVE side = ControlRigAutoRigSideUVE::Center;
    std::string drivenJointId;
    ControlRigControlShapeUVE shape;
    bool animatorVisible = false;
    bool animatorSelectable = false;

    [[nodiscard]] bool operator==(const ControlRigGeneratedControlUVE&) const noexcept = default;
};

struct ControlRigAutoRigJointMapUVE final {
    std::string rootJointId = "root";
    std::string spineJointId = "spine";
    std::string headJointId = "head";
    std::string leftUpperArmJointId = "upper_arm.L";
    std::string leftForearmJointId = "forearm.L";
    std::string leftHandJointId = "hand.L";
    std::string rightUpperArmJointId = "upper_arm.R";
    std::string rightForearmJointId = "forearm.R";
    std::string rightHandJointId = "hand.R";
    std::string leftThighJointId = "thigh.L";
    std::string leftShinJointId = "shin.L";
    std::string leftFootJointId = "foot.L";
    std::string rightThighJointId = "thigh.R";
    std::string rightShinJointId = "shin.R";
    std::string rightFootJointId = "foot.R";

    [[nodiscard]] bool operator==(const ControlRigAutoRigJointMapUVE&) const noexcept = default;
};

struct ControlRigAutoRigRequestUVE final {
    static constexpr std::size_t kMaximumRigIdentifierBytesUVE = 128U;

    std::string rigId;
    ControlRigAutoRigJointMapUVE jointMap;
    SkeletonDefinitionUVE skeleton;
    PoseBufferUVE referencePose;
    bool generateSpineControl = true;
    bool generateHeadControl = true;

    [[nodiscard]] bool operator==(const ControlRigAutoRigRequestUVE&) const noexcept = default;
};

struct ControlRigAutoRigUVE final {
    static constexpr std::size_t kMaximumGeneratedControlsUVE = 512U;
    static constexpr std::size_t kMaximumRoleBindingsUVE = 512U;

    std::string rigId;
    SkeletonDefinitionUVE skeleton;
    PoseBufferUVE referencePose;
    std::vector<ControlRigRoleBindingUVE> roleBindings;
    std::vector<ControlRigGeneratedControlUVE> generatedControls;
    std::vector<ControlRigGeneratedControlUVE> referenceGeneratedControls;
    std::vector<ControlRigControlUVE> referenceRuntimeControls;
    ControlRigUVE runtimeRig;

    [[nodiscard]] bool operator==(const ControlRigAutoRigUVE&) const noexcept = default;
};

enum class ControlRigAutoRigValidationCodeUVE : std::uint8_t {
    Valid = 0,
    InvalidIdentifier,
    InvalidSkeleton,
    InvalidReferencePose,
    CapacityExceeded,
    DuplicateRoleBinding,
    DuplicateGeneratedControl,
    InvalidGeneratedControl,
    InvalidShape,
    UnknownDrivenJoint,
    InvalidRuntimeRig,
};

struct ControlRigAutoRigValidationResultUVE final {
    ControlRigAutoRigValidationCodeUVE code = ControlRigAutoRigValidationCodeUVE::InvalidIdentifier;
    std::string identifier;
    std::string message;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return code == ControlRigAutoRigValidationCodeUVE::Valid;
    }
};

struct ControlRigBakeResultUVE final {
    PoseBufferUVE pose;
    std::size_t mappedJointCount = 0U;
    bool baked = false;
    std::string message;

    [[nodiscard]] bool IsSuccessUVE() const noexcept {
        return baked && mappedJointCount > 0U;
    }
};

[[nodiscard]] bool BuildControlRigAutoRigUVE(const ControlRigAutoRigRequestUVE& request,
                                             ControlRigAutoRigUVE& outRig) noexcept;

[[nodiscard]] ControlRigAutoRigValidationResultUVE ValidateControlRigAutoRigUVE(
    const ControlRigAutoRigUVE& rig) noexcept;

[[nodiscard]] bool ResetControlRigAutoRigUVE(ControlRigAutoRigUVE& rig) noexcept;

[[nodiscard]] bool MirrorControlRigControlsUVE(
    const ControlRigAutoRigUVE& rig, const std::vector<ControlRigControlUVE>& controls,
    std::vector<ControlRigControlUVE>& outMirroredControls) noexcept;

[[nodiscard]] ControlRigBakeResultUVE BakeControlRigPoseToSkeletonUVE(
    const ControlRigAutoRigUVE& rig, const ControlRigUVE& evaluatedRig) noexcept;

} // namespace UVE::Core
