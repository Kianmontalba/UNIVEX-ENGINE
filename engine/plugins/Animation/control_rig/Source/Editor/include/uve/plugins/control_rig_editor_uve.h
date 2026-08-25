// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/plugins/control_rig_autorig_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace UVE::Core {

enum class ControlRigEditorToolUVE : std::uint8_t {
    Select = 0,
    Translate,
    Rotate,
};

struct ControlRigViewportControlFactUVE final {
    std::string controlId;
    ControlRigAutoRigRoleUVE role = ControlRigAutoRigRoleUVE::Deform;
    ControlRigAutoRigSideUVE side = ControlRigAutoRigSideUVE::Center;
    ControlRigControlShapeUVE shape;
    TransformPoseUVE pose;
    bool selected = false;
    bool visible = false;
    bool selectable = false;

    [[nodiscard]] bool operator==(const ControlRigViewportControlFactUVE&) const noexcept = default;
};

struct ControlRigAuthoringSnapshotUVE final {
    std::string rigId;
    std::uint64_t revision = 0U;
    ControlRigEditorToolUVE tool = ControlRigEditorToolUVE::Select;
    std::string selectedControlId;
    std::vector<ControlRigViewportControlFactUVE> viewportControls;
    bool dirty = false;
    bool evaluated = false;
    std::string message;

    [[nodiscard]] bool operator==(const ControlRigAuthoringSnapshotUVE&) const noexcept = default;
};

struct ControlRigBakedPoseSampleUVE final {
    double timeSeconds = 0.0;
    PoseBufferUVE pose;

    [[nodiscard]] bool operator==(const ControlRigBakedPoseSampleUVE&) const noexcept = default;
};

struct ControlRigBakedAnimationUVE final {
    static constexpr std::size_t kMaximumSamplesUVE = 4096U;

    std::string animationId;
    SkeletonDefinitionUVE skeleton;
    std::vector<ControlRigBakedPoseSampleUVE> samples;

    [[nodiscard]] bool operator==(const ControlRigBakedAnimationUVE&) const noexcept = default;
};

enum class ControlRigEditorValidationCodeUVE : std::uint8_t {
    Valid = 0,
    Uninitialized,
    UnknownControl,
    ControlNotSelectable,
    InvalidTransform,
    InvalidSampleTime,
    UnsortedSample,
    CapacityExceeded,
    EmptyAnimation,
};

struct ControlRigEditorValidationResultUVE final {
    ControlRigEditorValidationCodeUVE code = ControlRigEditorValidationCodeUVE::Uninitialized;
    std::string identifier;
    std::string message;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return code == ControlRigEditorValidationCodeUVE::Valid;
    }
};

struct ControlRigEditorBakedAnimationResultUVE final {
    ControlRigBakedAnimationUVE animation;
    bool baked = false;
    std::string message;

    [[nodiscard]] bool IsSuccessUVE() const noexcept {
        return baked && !animation.samples.empty();
    }
};

class ControlRigEditorAuthoringSessionUVE final {
public:
    [[nodiscard]] bool InitializeUVE(const ControlRigAutoRigRequestUVE& request) noexcept;
    [[nodiscard]] bool IsInitializedUVE() const noexcept;

    [[nodiscard]] bool SelectControlUVE(const std::string& controlId) noexcept;
    [[nodiscard]] bool SetToolUVE(ControlRigEditorToolUVE tool) noexcept;
    [[nodiscard]] bool TranslateSelectedControlUVE(const Math::Vector3UVE& worldDelta) noexcept;
    [[nodiscard]] bool RotateSelectedControlUVE(const Math::QuaternionUVE& worldDelta) noexcept;
    [[nodiscard]] bool ResetControlsUVE() noexcept;
    [[nodiscard]] bool MirrorControlsUVE() noexcept;

    [[nodiscard]] bool EvaluateUVE() noexcept;
    [[nodiscard]] bool CaptureBakeSampleUVE(double timeSeconds) noexcept;
    [[nodiscard]] ControlRigEditorBakedAnimationResultUVE BakeAnimationUVE(
        std::string animationId) const noexcept;

    [[nodiscard]] ControlRigEditorValidationResultUVE ValidateUVE() const noexcept;
    [[nodiscard]] ControlRigAuthoringSnapshotUVE CaptureSnapshotUVE() const;

private:
    [[nodiscard]] ControlRigGeneratedControlUVE* FindGeneratedControlUVE(
        const std::string& controlId) noexcept;
    [[nodiscard]] const ControlRigGeneratedControlUVE* FindGeneratedControlUVE(
        const std::string& controlId) const noexcept;
    [[nodiscard]] ControlRigControlUVE* FindRuntimeControlUVE(
        const std::string& controlId) noexcept;
    void MarkDirtyUVE() noexcept;

    ControlRigAutoRigUVE m_rig;
    std::string m_selectedControlId;
    std::vector<ControlRigBakedPoseSampleUVE> m_bakeSamples;
    ControlRigEditorToolUVE m_tool = ControlRigEditorToolUVE::Select;
    std::uint64_t m_revision = 0U;
    bool m_initialized = false;
    bool m_dirty = false;
    bool m_evaluated = false;
    std::string m_message;
};

} // namespace UVE::Core
