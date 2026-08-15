// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/core/animation_state_machine_uve.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace UVE::Core {
namespace {

constexpr std::size_t kMaximumIdentifierBytesUVE = 128U;

[[nodiscard]] bool IsBoundedIdentifierUVE(const std::string& value) noexcept {
    return !value.empty() && value.size() <= kMaximumIdentifierBytesUVE;
}

[[nodiscard]] const AnimationClipUVE* FindClipUVE(const AnimationStateMachineUVE& machine,
                                                  const std::string& clipId) noexcept {
    const auto iterator = std::find_if(machine.clips.cbegin(), machine.clips.cend(), [&clipId](const auto& clip) {
        return clip.clipId == clipId;
    });
    return iterator == machine.clips.cend() ? nullptr : &*iterator;
}

[[nodiscard]] bool IsFiniteRangeUVE(const float value, const float minimum, const float maximum) noexcept {
    return std::isfinite(value) && value >= minimum && value <= maximum;
}

} // namespace

AnimationStateMachineValidationResultUVE ValidateAnimationStateMachineUVE(
    const AnimationStateMachineUVE& machine) noexcept {
    if (machine.states.empty() || machine.initialStateId.empty()) {
        return {AnimationStateMachineValidationCodeUVE::EmptyMachine, {},
                "Animation state machine requires states and an initial state."};
    }
    if (machine.states.size() > AnimationStateMachineUVE::kMaximumStatesUVE ||
        machine.transitions.size() > AnimationStateMachineUVE::kMaximumTransitionsUVE) {
        return {AnimationStateMachineValidationCodeUVE::CapacityExceeded, {},
                "Animation state machine exceeds the bounded state or transition limit."};
    }
    std::vector<std::string> stateIds;
    stateIds.reserve(machine.states.size());
    for (const AnimationStateUVE& state : machine.states) {
        if (!IsBoundedIdentifierUVE(state.stateId) || !IsBoundedIdentifierUVE(state.clipId) ||
            ((!IsBoundedIdentifierUVE(state.syncGroupId) && !state.syncGroupId.empty())) ||
            ((!IsBoundedIdentifierUVE(state.layerMaskId) && !state.layerMaskId.empty())) ||
            !IsFiniteRangeUVE(state.speed, 0.0F, std::numeric_limits<float>::max())) {
            return {AnimationStateMachineValidationCodeUVE::InvalidState, state.stateId,
                    "Animation state identity, optional metadata, or speed is invalid."};
        }
        if (std::find(stateIds.begin(), stateIds.end(), state.stateId) != stateIds.end()) {
            return {AnimationStateMachineValidationCodeUVE::DuplicateState, state.stateId,
                    "Animation state identifiers must be unique."};
        }
        stateIds.push_back(state.stateId);
        if (FindClipUVE(machine, state.clipId) == nullptr) {
            return {AnimationStateMachineValidationCodeUVE::UnknownClip, state.clipId,
                    "Animation state references an unknown AnimationClip."};
        }
    }
    if (std::find(stateIds.begin(), stateIds.end(), machine.initialStateId) == stateIds.end()) {
        return {AnimationStateMachineValidationCodeUVE::MissingInitialState, machine.initialStateId,
                "Animation state machine initial state is not registered."};
    }
    std::vector<std::string> transitionIds;
    transitionIds.reserve(machine.transitions.size());
    for (const AnimationTransitionUVE& transition : machine.transitions) {
        if (!IsBoundedIdentifierUVE(transition.transitionId) ||
            !IsBoundedIdentifierUVE(transition.sourceStateId) ||
            !IsBoundedIdentifierUVE(transition.targetStateId) ||
            transition.sourceStateId == transition.targetStateId) {
            return {AnimationStateMachineValidationCodeUVE::InvalidTransition, transition.transitionId,
                    "Animation transition identity or endpoints are invalid."};
        }
        if (std::find(transitionIds.begin(), transitionIds.end(), transition.transitionId) != transitionIds.end()) {
            return {AnimationStateMachineValidationCodeUVE::DuplicateTransition, transition.transitionId,
                    "Animation transition identifiers must be unique."};
        }
        if (std::find(stateIds.begin(), stateIds.end(), transition.sourceStateId) == stateIds.end() ||
            std::find(stateIds.begin(), stateIds.end(), transition.targetStateId) == stateIds.end()) {
            return {AnimationStateMachineValidationCodeUVE::UnknownState, transition.transitionId,
                    "Animation transition references an unknown state."};
        }
        if (!IsFiniteRangeUVE(transition.durationSeconds, 0.0F, std::numeric_limits<float>::max())) {
            return {AnimationStateMachineValidationCodeUVE::InvalidDuration, transition.transitionId,
                    "Animation transition duration is invalid."};
        }
        if (!IsFiniteRangeUVE(transition.exitTime, 0.0F, 1.0F)) {
            return {AnimationStateMachineValidationCodeUVE::InvalidExitTime, transition.transitionId,
                    "Animation transition exit time must be within [0, 1]."};
        }
        transitionIds.push_back(transition.transitionId);
    }
    return {AnimationStateMachineValidationCodeUVE::Valid, {}, "Animation state machine is valid."};
}

AnimationStateMachineEvaluatorUVE::AnimationStateMachineEvaluatorUVE(
    const AnimationStateMachineUVE& machine) : m_machine(machine) {
    m_debug.currentStateId = machine.initialStateId;
}

AnimationStateMachineEvaluationResultUVE AnimationStateMachineEvaluatorUVE::EvaluateUVE(
    const double deltaSeconds) {
    AnimationStateMachineEvaluationResultUVE result;
    if (!ValidateAnimationStateMachineUVE(m_machine).IsValidUVE() || !std::isfinite(deltaSeconds) || deltaSeconds < 0.0) {
        result.message = "Animation state-machine evaluation rejected invalid input.";
        result.debug = m_debug;
        return result;
    }
    const AnimationStateUVE* current = FindStateUVE(m_debug.currentStateId);
    if (current == nullptr) {
        result.message = "Animation state-machine current state is unavailable.";
        result.debug = m_debug;
        return result;
    }
    const double clampedDelta = std::min(deltaSeconds, 0.25);
    m_debug.normalizedTime += clampedDelta * static_cast<double>(current->speed);
    m_debug.normalizedTime = std::fmod(m_debug.normalizedTime, 1.0);

    if (!m_debug.transitioning) {
        const AnimationTransitionUVE* transition = SelectTransitionUVE(*current);
        if (transition != nullptr) {
            m_debug.activeTransitionId = transition->transitionId;
            m_debug.targetStateId = transition->targetStateId;
            m_debug.transitionAlpha = transition->durationSeconds <= 0.0F ? 1.0 : 0.0;
            m_debug.selectedTransitionPriority = transition->priority;
            m_debug.transitioning = transition->durationSeconds > 0.0F;
            if (!m_debug.transitioning) {
                m_debug.currentStateId = transition->targetStateId;
                m_debug.targetStateId.clear();
                m_debug.activeTransitionId.clear();
                m_debug.normalizedTime = 0.0;
                current = FindStateUVE(m_debug.currentStateId);
            }
        }
    }
    if (m_debug.transitioning) {
        const auto transitionIterator =
            std::find_if(m_machine.transitions.cbegin(), m_machine.transitions.cend(), [this](const auto& candidate) {
                return candidate.transitionId == m_debug.activeTransitionId;
            });
        if (transitionIterator == m_machine.transitions.cend()) {
            result.message = "Animation state-machine active transition disappeared.";
            result.debug = m_debug;
            return result;
        }
        const AnimationTransitionUVE& transition = *transitionIterator;
        m_debug.transitionAlpha = std::min(1.0, m_debug.transitionAlpha +
            (transition.durationSeconds <= 0.0F ? 1.0 : clampedDelta / transition.durationSeconds));
        if (m_debug.transitionAlpha >= 1.0) {
            m_debug.currentStateId = m_debug.targetStateId;
            m_debug.targetStateId.clear();
            m_debug.activeTransitionId.clear();
            m_debug.transitioning = false;
            m_debug.transitionAlpha = 0.0;
            m_debug.normalizedTime = 0.0;
            current = FindStateUVE(m_debug.currentStateId);
        }
    }

    TransformPoseUVE currentPose;
    if (!SampleStateUVE(*current, m_debug.normalizedTime, currentPose)) {
        result.message = "Animation state-machine current clip sampling failed.";
        result.debug = m_debug;
        return result;
    }
    result.pose = currentPose;
    if (m_debug.transitioning) {
        const AnimationStateUVE* target = FindStateUVE(m_debug.targetStateId);
        TransformPoseUVE targetPose;
        if (target == nullptr || !SampleStateUVE(*target, m_debug.normalizedTime, targetPose)) {
            result.message = "Animation state-machine target clip sampling failed.";
            result.debug = m_debug;
            return result;
        }
        const float alpha = static_cast<float>(m_debug.transitionAlpha);
        result.pose.position = currentPose.position * (1.0F - alpha) + targetPose.position * alpha;
        result.pose.scale = currentPose.scale * (1.0F - alpha) + targetPose.scale * alpha;
        result.pose.rotation = Math::QuaternionUVE{
            currentPose.rotation.x * (1.0F - alpha) + targetPose.rotation.x * alpha,
            currentPose.rotation.y * (1.0F - alpha) + targetPose.rotation.y * alpha,
            currentPose.rotation.z * (1.0F - alpha) + targetPose.rotation.z * alpha,
            currentPose.rotation.w * (1.0F - alpha) + targetPose.rotation.w * alpha,
        };
        TransformPoseUVE normalized;
        if (!TryNormalizeTransformPoseUVE(result.pose, normalized)) {
            result.message = "Animation state-machine blend produced an invalid pose.";
            result.debug = m_debug;
            return result;
        }
        result.pose = normalized;
    }
    m_debug.syncGroupId = current->syncGroupId;
    m_debug.layerMaskId = current->layerMaskId;
    result.debug = m_debug;
    result.usedOutput = true;
    result.message = "Animation state-machine evaluated successfully.";
    return result;
}

const AnimationStateMachineDebugSnapshotUVE& AnimationStateMachineEvaluatorUVE::GetDebugSnapshotUVE() const noexcept {
    return m_debug;
}

const AnimationStateUVE* AnimationStateMachineEvaluatorUVE::FindStateUVE(const std::string& stateId) const noexcept {
    const auto iterator = std::find_if(m_machine.states.cbegin(), m_machine.states.cend(), [&stateId](const auto& state) {
        return state.stateId == stateId;
    });
    return iterator == m_machine.states.cend() ? nullptr : &*iterator;
}

const AnimationTransitionUVE* AnimationStateMachineEvaluatorUVE::SelectTransitionUVE(
    const AnimationStateUVE& source) const noexcept {
    const AnimationTransitionUVE* selected = nullptr;
    for (const AnimationTransitionUVE& transition : m_machine.transitions) {
        if (transition.sourceStateId != source.stateId ||
            m_debug.normalizedTime + 1.0e-9 < static_cast<double>(transition.exitTime)) {
            continue;
        }
        if (selected == nullptr || transition.priority > selected->priority ||
            (transition.priority == selected->priority && transition.transitionId < selected->transitionId)) {
            selected = &transition;
        }
    }
    return selected;
}

bool AnimationStateMachineEvaluatorUVE::SampleStateUVE(const AnimationStateUVE& state,
                                                       const double timeSeconds,
                                                       TransformPoseUVE& outPose) const noexcept {
    const AnimationClipUVE* clip = FindClipUVE(m_machine, state.clipId);
    return clip != nullptr && TrySampleAnimationClipUVE(*clip, timeSeconds, true, outPose);
}

} // namespace UVE::Core
