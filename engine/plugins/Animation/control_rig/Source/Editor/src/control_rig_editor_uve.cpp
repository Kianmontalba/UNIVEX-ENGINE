// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/plugins/control_rig_editor_uve.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace UVE::Core {
namespace {

[[nodiscard]] bool IsFiniteVectorUVE(const Math::Vector3UVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool IsFiniteTimeUVE(const double value) noexcept {
    return std::isfinite(value) && value >= 0.0 && value <= 86400.0;
}

[[nodiscard]] bool IsValidToolUVE(const ControlRigEditorToolUVE tool) noexcept {
    switch (tool) {
    case ControlRigEditorToolUVE::Select:
    case ControlRigEditorToolUVE::Translate:
    case ControlRigEditorToolUVE::Rotate:
        return true;
    }
    return false;
}

} // namespace

bool ControlRigEditorAuthoringSessionUVE::InitializeUVE(
    const ControlRigAutoRigRequestUVE& request) noexcept {
    ControlRigAutoRigUVE candidate;
    if (!BuildControlRigAutoRigUVE(request, candidate)) {
        m_message = "Control Rig autorig generation failed.";
        return false;
    }
    m_rig = std::move(candidate);
    m_selectedControlId.clear();
    m_bakeSamples.clear();
    m_tool = ControlRigEditorToolUVE::Select;
    m_revision = 1U;
    m_initialized = true;
    m_dirty = false;
    m_evaluated = false;
    m_message = "Control Rig authoring session initialized.";
    return true;
}

bool ControlRigEditorAuthoringSessionUVE::IsInitializedUVE() const noexcept {
    return m_initialized;
}

ControlRigGeneratedControlUVE* ControlRigEditorAuthoringSessionUVE::FindGeneratedControlUVE(
    const std::string& controlId) noexcept {
    const auto iterator = std::find_if(m_rig.generatedControls.begin(), m_rig.generatedControls.end(),
                                       [&controlId](auto& generated) {
                                           return generated.control.controlId == controlId;
                                       });
    return iterator == m_rig.generatedControls.end() ? nullptr : &*iterator;
}

const ControlRigGeneratedControlUVE* ControlRigEditorAuthoringSessionUVE::FindGeneratedControlUVE(
    const std::string& controlId) const noexcept {
    const auto iterator = std::find_if(m_rig.generatedControls.cbegin(), m_rig.generatedControls.cend(),
                                       [&controlId](const auto& generated) {
                                           return generated.control.controlId == controlId;
                                       });
    return iterator == m_rig.generatedControls.cend() ? nullptr : &*iterator;
}

ControlRigControlUVE* ControlRigEditorAuthoringSessionUVE::FindRuntimeControlUVE(
    const std::string& controlId) noexcept {
    const auto iterator = std::find_if(m_rig.runtimeRig.controls.begin(), m_rig.runtimeRig.controls.end(),
                                       [&controlId](auto& control) {
                                           return control.controlId == controlId;
                                       });
    return iterator == m_rig.runtimeRig.controls.end() ? nullptr : &*iterator;
}

void ControlRigEditorAuthoringSessionUVE::MarkDirtyUVE() noexcept {
    m_dirty = true;
    m_evaluated = false;
    if (m_revision < std::numeric_limits<std::uint64_t>::max()) {
        ++m_revision;
    }
}

bool ControlRigEditorAuthoringSessionUVE::SelectControlUVE(
    const std::string& controlId) noexcept {
    if (!m_initialized) {
        return false;
    }
    const ControlRigGeneratedControlUVE* const generated = FindGeneratedControlUVE(controlId);
    if (generated == nullptr || !generated->animatorSelectable) {
        return false;
    }
    if (m_selectedControlId == controlId) {
        return true;
    }
    m_selectedControlId = controlId;
    ++m_revision;
    m_message = "Control Rig control selected.";
    return true;
}

bool ControlRigEditorAuthoringSessionUVE::SetToolUVE(const ControlRigEditorToolUVE tool) noexcept {
    if (!m_initialized || !IsValidToolUVE(tool)) {
        return false;
    }
    if (m_tool == tool) {
        return true;
    }
    m_tool = tool;
    ++m_revision;
    m_message = "Control Rig authoring tool changed.";
    return true;
}

bool ControlRigEditorAuthoringSessionUVE::TranslateSelectedControlUVE(
    const Math::Vector3UVE& worldDelta) noexcept {
    if (!m_initialized || m_tool != ControlRigEditorToolUVE::Translate ||
        m_selectedControlId.empty() || !IsFiniteVectorUVE(worldDelta)) {
        return false;
    }
    ControlRigGeneratedControlUVE* const generated = FindGeneratedControlUVE(m_selectedControlId);
    ControlRigControlUVE* const runtime = FindRuntimeControlUVE(m_selectedControlId);
    if (generated == nullptr || runtime == nullptr || !generated->animatorSelectable) {
        return false;
    }
    TransformPoseUVE proposed = generated->control.pose;
    proposed.position += worldDelta;
    if (!IsFiniteTransformPoseUVE(proposed)) {
        return false;
    }
    generated->control.pose = proposed;
    runtime->pose = proposed;
    MarkDirtyUVE();
    m_message = "Control Rig control translated.";
    return true;
}

bool ControlRigEditorAuthoringSessionUVE::RotateSelectedControlUVE(
    const Math::QuaternionUVE& worldDelta) noexcept {
    if (!m_initialized || m_tool != ControlRigEditorToolUVE::Rotate ||
        m_selectedControlId.empty() || !Math::IsFiniteUVE(worldDelta)) {
        return false;
    }
    Math::QuaternionUVE normalizedDelta;
    if (!Math::TryNormalizeUVE(worldDelta, normalizedDelta)) {
        return false;
    }
    ControlRigGeneratedControlUVE* const generated = FindGeneratedControlUVE(m_selectedControlId);
    ControlRigControlUVE* const runtime = FindRuntimeControlUVE(m_selectedControlId);
    if (generated == nullptr || runtime == nullptr || !generated->animatorSelectable) {
        return false;
    }
    TransformPoseUVE proposed = generated->control.pose;
    proposed.rotation = Math::MultiplyUVE(normalizedDelta, proposed.rotation);
    if (!TryNormalizeTransformPoseUVE(proposed, proposed)) {
        return false;
    }
    generated->control.pose = proposed;
    runtime->pose = proposed;
    MarkDirtyUVE();
    m_message = "Control Rig control rotated.";
    return true;
}

bool ControlRigEditorAuthoringSessionUVE::ResetControlsUVE() noexcept {
    if (!m_initialized || !ResetControlRigAutoRigUVE(m_rig)) {
        return false;
    }
    m_selectedControlId.clear();
    m_bakeSamples.clear();
    m_dirty = false;
    m_evaluated = false;
    ++m_revision;
    m_message = "Control Rig controls reset to the generated reference pose.";
    return true;
}

bool ControlRigEditorAuthoringSessionUVE::MirrorControlsUVE() noexcept {
    if (!m_initialized) {
        return false;
    }
    std::vector<ControlRigControlUVE> mirroredRuntime;
    if (!MirrorControlRigControlsUVE(m_rig, m_rig.runtimeRig.controls, mirroredRuntime)) {
        return false;
    }
    std::vector<ControlRigControlUVE> mirroredGenerated;
    mirroredGenerated.reserve(m_rig.generatedControls.size());
    for (const ControlRigGeneratedControlUVE& generated : m_rig.generatedControls) {
        std::vector<ControlRigControlUVE> one{generated.control};
        std::vector<ControlRigControlUVE> mirroredOne;
        if (!MirrorControlRigControlsUVE(m_rig, one, mirroredOne) || mirroredOne.size() != 1U) {
            return false;
        }
        mirroredGenerated.push_back(std::move(mirroredOne.front()));
    }
    for (std::size_t index = 0U; index < m_rig.generatedControls.size(); ++index) {
        m_rig.generatedControls[index].control = mirroredGenerated[index];
    }
    m_rig.runtimeRig.controls = std::move(mirroredRuntime);
    m_selectedControlId.clear();
    MarkDirtyUVE();
    m_message = "Control Rig controls mirrored across the X axis.";
    return true;
}

bool ControlRigEditorAuthoringSessionUVE::EvaluateUVE() noexcept {
    if (!m_initialized) {
        return false;
    }
    const ControlRigEvaluationResultUVE result = EvaluateControlRigUVE(m_rig.runtimeRig);
    if (!result.IsSuccessUVE()) {
        m_message = result.message;
        m_evaluated = false;
        return false;
    }
    m_rig.runtimeRig.controls = result.controls;
    m_rig.runtimeRig.skeleton = result.skeleton;
    m_rig.runtimeRig.pose = result.pose;
    m_rig.runtimeRig.evaluationContext = result.evaluationContext;
    for (ControlRigGeneratedControlUVE& generated : m_rig.generatedControls) {
        const auto iterator = std::find_if(result.controls.cbegin(), result.controls.cend(),
                                           [&generated](const auto& control) {
                                               return control.controlId == generated.control.controlId;
                                           });
        if (iterator != result.controls.cend()) {
            generated.control.pose = iterator->pose;
        }
    }
    m_evaluated = true;
    m_message = result.message;
    return true;
}

bool ControlRigEditorAuthoringSessionUVE::CaptureBakeSampleUVE(const double timeSeconds) noexcept {
    if (!m_initialized || !IsFiniteTimeUVE(timeSeconds) ||
        m_bakeSamples.size() >= ControlRigBakedAnimationUVE::kMaximumSamplesUVE) {
        return false;
    }
    if (!m_bakeSamples.empty() && timeSeconds <= m_bakeSamples.back().timeSeconds) {
        return false;
    }
    m_rig.runtimeRig.evaluationContext.sampleTimeSeconds = timeSeconds;
    m_rig.runtimeRig.evaluationContext.time.animationTimeSeconds = timeSeconds;
    if (!EvaluateUVE()) {
        return false;
    }
    const ControlRigBakeResultUVE bake = BakeControlRigPoseToSkeletonUVE(m_rig, m_rig.runtimeRig);
    if (!bake.IsSuccessUVE()) {
        m_message = bake.message;
        return false;
    }
    m_bakeSamples.push_back({timeSeconds, bake.pose});
    ++m_revision;
    m_message = "Control Rig bake sample captured.";
    return true;
}

ControlRigEditorBakedAnimationResultUVE ControlRigEditorAuthoringSessionUVE::BakeAnimationUVE(
    std::string animationId) const noexcept {
    ControlRigEditorBakedAnimationResultUVE result;
    if (!m_initialized || animationId.empty() || animationId.size() > 128U || m_bakeSamples.empty() ||
        !ValidateControlRigAutoRigUVE(m_rig).IsValidUVE()) {
        result.message = "Control Rig baked animation request is invalid or empty.";
        return result;
    }
    result.animation.animationId = std::move(animationId);
    result.animation.skeleton = m_rig.skeleton;
    result.animation.samples = m_bakeSamples;
    result.baked = true;
    result.message = "Control Rig animation samples are ready for deform-skeleton playback.";
    return result;
}

ControlRigEditorValidationResultUVE ControlRigEditorAuthoringSessionUVE::ValidateUVE() const noexcept {
    if (!m_initialized) {
        return {ControlRigEditorValidationCodeUVE::Uninitialized, {},
                "Control Rig authoring session is not initialized."};
    }
    const ControlRigAutoRigValidationResultUVE rigValidation = ValidateControlRigAutoRigUVE(m_rig);
    if (!rigValidation.IsValidUVE()) {
        return {ControlRigEditorValidationCodeUVE::InvalidTransform, rigValidation.identifier,
                rigValidation.message};
    }
    if (!m_selectedControlId.empty()) {
        const ControlRigGeneratedControlUVE* const selected = FindGeneratedControlUVE(m_selectedControlId);
        if (selected == nullptr) {
            return {ControlRigEditorValidationCodeUVE::UnknownControl, m_selectedControlId,
                    "Selected Control Rig control no longer exists."};
        }
        if (!selected->animatorSelectable) {
            return {ControlRigEditorValidationCodeUVE::ControlNotSelectable, m_selectedControlId,
                    "Selected Control Rig control is not animator-selectable."};
        }
    }
    for (std::size_t index = 0U; index < m_bakeSamples.size(); ++index) {
        const ControlRigBakedPoseSampleUVE& sample = m_bakeSamples[index];
        if (!IsFiniteTimeUVE(sample.timeSeconds) ||
            (index > 0U && sample.timeSeconds <= m_bakeSamples[index - 1U].timeSeconds)) {
            return {index > 0U ? ControlRigEditorValidationCodeUVE::UnsortedSample
                              : ControlRigEditorValidationCodeUVE::InvalidSampleTime,
                    {}, "Control Rig baked samples must be finite and strictly chronological."};
        }
        if (!ValidatePoseBufferUVE(sample.pose, m_rig.skeleton).IsValidUVE()) {
            return {ControlRigEditorValidationCodeUVE::InvalidTransform, {},
                    "Control Rig baked sample pose is invalid."};
        }
    }
    return {ControlRigEditorValidationCodeUVE::Valid, {}, "Control Rig authoring session is valid."};
}

ControlRigAuthoringSnapshotUVE ControlRigEditorAuthoringSessionUVE::CaptureSnapshotUVE() const {
    ControlRigAuthoringSnapshotUVE snapshot;
    snapshot.rigId = m_rig.rigId;
    snapshot.revision = m_revision;
    snapshot.tool = m_tool;
    snapshot.selectedControlId = m_selectedControlId;
    snapshot.dirty = m_dirty;
    snapshot.evaluated = m_evaluated;
    snapshot.message = m_message;
    snapshot.viewportControls.reserve(m_rig.generatedControls.size());
    for (const ControlRigGeneratedControlUVE& generated : m_rig.generatedControls) {
        snapshot.viewportControls.push_back({generated.control.controlId, generated.role, generated.side,
                                              generated.shape, generated.control.pose,
                                              generated.control.controlId == m_selectedControlId,
                                              generated.animatorVisible, generated.animatorSelectable});
    }
    return snapshot;
}

} // namespace UVE::Core
