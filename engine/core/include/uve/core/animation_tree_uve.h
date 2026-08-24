// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/core/animation_clip_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace UVE::Core {

enum class AnimationTreeNodeKindUVE : std::uint8_t {
    ClipPlayer = 0,
    Blend,
    Parameter,
    State,
    Transition,
    OneShot,
    TimeScale,
    Sync,
    Subtree,
    PoseCache,
    OutputPose,
};

struct AnimationTreeNodeUVE final {
    std::uint32_t id = 0U;
    AnimationTreeNodeKindUVE kind = AnimationTreeNodeKindUVE::OutputPose;
    std::string name;
    std::string clipId;
    std::string parameterId;
    std::uint32_t inputA = 0U;
    std::uint32_t inputB = 0U;
    float weight = 0.5F;
    float timeScale = 1.0F;
    bool enabled = true;
};

struct AnimationTreeUVE final {
    static constexpr std::size_t kMaximumNodesUVE = 512U;
    static constexpr std::size_t kMaximumParametersUVE = 128U;

    std::vector<AnimationTreeNodeUVE> nodes;
    std::vector<AnimationClipUVE> clips;
};

enum class AnimationTreeValidationCodeUVE : std::uint8_t {
    Valid = 0,
    EmptyTree,
    CapacityExceeded,
    InvalidNode,
    DuplicateNode,
    UnknownInput,
    InvalidClip,
    UnknownClip,
    CycleDetected,
    InvalidParameter,
    MissingOutput,
};

struct AnimationTreeValidationResultUVE final {
    AnimationTreeValidationCodeUVE code = AnimationTreeValidationCodeUVE::EmptyTree;
    std::uint32_t nodeId = 0U;
    std::string message;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return code == AnimationTreeValidationCodeUVE::Valid;
    }
};

struct AnimationTreeParameterUVE final {
    std::string parameterId;
    float value = 0.0F;
};

struct AnimationTreeEvaluationResultUVE final {
    TransformPoseUVE pose;
    bool usedOutputNode = false;
    std::size_t evaluatedNodeCount = 0U;
    std::string message;

    [[nodiscard]] bool IsSuccessUVE() const noexcept {
        return usedOutputNode && evaluatedNodeCount > 0U;
    }
};

[[nodiscard]] AnimationTreeValidationResultUVE ValidateAnimationTreeUVE(
    const AnimationTreeUVE& tree) noexcept;

/// Evaluates shared nodes independently for distinct local times; memoization is keyed by node ID
/// and exact local evaluation time, while active recursion remains cycle-checked by node ID.
[[nodiscard]] AnimationTreeEvaluationResultUVE EvaluateAnimationTreeUVE(
    const AnimationTreeUVE& tree, double timeSeconds, const std::vector<AnimationTreeParameterUVE>& parameters = {});

} // namespace UVE::Core
