// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/core/animation_clip_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace UVE::Core {

enum class AnimationInterruptionPolicyUVE : std::uint8_t {
    None = 0,
    Source,
    Target,
    SourceOrTarget,
};

struct AnimationStateUVE final {
    std::string stateId;
    std::string clipId;
    std::string syncGroupId;
    std::string layerMaskId;
    float speed = 1.0F;
};

/// Controls whether a transition already in progress may be replaced by an eligible transition
/// from its source state, target state, both, or neither. Replacement preserves deterministic
/// priority/identifier selection and starts the replacement crossfade from zero alpha.
struct AnimationTransitionUVE final {
    std::string transitionId;
    std::string sourceStateId;
    std::string targetStateId;
    float durationSeconds = 0.0F;
    float exitTime = 0.0F;
    std::int32_t priority = 0;
    AnimationInterruptionPolicyUVE interruption = AnimationInterruptionPolicyUVE::None;
};

struct AnimationStateMachineUVE final {
    static constexpr std::size_t kMaximumStatesUVE = 256U;
    static constexpr std::size_t kMaximumTransitionsUVE = 512U;

    std::vector<AnimationStateUVE> states;
    std::vector<AnimationTransitionUVE> transitions;
    std::vector<AnimationClipUVE> clips;
    std::string initialStateId;
};

enum class AnimationStateMachineValidationCodeUVE : std::uint8_t {
    Valid = 0,
    EmptyMachine,
    CapacityExceeded,
    InvalidState,
    DuplicateState,
    UnknownClip,
    InvalidTransition,
    DuplicateTransition,
    UnknownState,
    InvalidDuration,
    InvalidExitTime,
    MissingInitialState,
};

struct AnimationStateMachineValidationResultUVE final {
    AnimationStateMachineValidationCodeUVE code = AnimationStateMachineValidationCodeUVE::EmptyMachine;
    std::string identifier;
    std::string message;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return code == AnimationStateMachineValidationCodeUVE::Valid;
    }
};

struct AnimationStateMachineDebugSnapshotUVE final {
    std::string currentStateId;
    std::string targetStateId;
    std::string activeTransitionId;
    std::string syncGroupId;
    std::string layerMaskId;
    double normalizedTime = 0.0;
    double transitionAlpha = 0.0;
    std::int32_t selectedTransitionPriority = 0;
    bool transitioning = false;
};

struct AnimationStateMachineEvaluationResultUVE final {
    TransformPoseUVE pose;
    AnimationStateMachineDebugSnapshotUVE debug;
    bool usedOutput = false;
    std::string message;

    [[nodiscard]] bool IsSuccessUVE() const noexcept {
        return usedOutput;
    }
};

[[nodiscard]] AnimationStateMachineValidationResultUVE ValidateAnimationStateMachineUVE(
    const AnimationStateMachineUVE& machine) noexcept;

class AnimationStateMachineEvaluatorUVE final {
public:
    explicit AnimationStateMachineEvaluatorUVE(const AnimationStateMachineUVE& machine);
    AnimationStateMachineEvaluatorUVE(AnimationStateMachineUVE&&) = delete;

    [[nodiscard]] AnimationStateMachineEvaluationResultUVE EvaluateUVE(double deltaSeconds);
    [[nodiscard]] const AnimationStateMachineDebugSnapshotUVE& GetDebugSnapshotUVE() const noexcept;

private:
    [[nodiscard]] const AnimationStateUVE* FindStateUVE(const std::string& stateId) const noexcept;
    [[nodiscard]] const AnimationTransitionUVE* SelectTransitionUVE(
        const AnimationStateUVE& source) const noexcept;
    [[nodiscard]] const AnimationTransitionUVE* SelectInterruptionTransitionUVE(
        const AnimationTransitionUVE& activeTransition) const noexcept;
    [[nodiscard]] bool SampleStateUVE(const AnimationStateUVE& state, double timeSeconds,
                                      TransformPoseUVE& outPose) const noexcept;

    const AnimationStateMachineUVE& m_machine;
    AnimationStateMachineDebugSnapshotUVE m_debug;
};

} // namespace UVE::Core
