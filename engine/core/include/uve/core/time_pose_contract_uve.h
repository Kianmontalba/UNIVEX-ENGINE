// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/math/quaternion_uve.h"
#include "uve/math/vector3_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace UVE::Core {

struct UnifiedTimeStateUVE final {
    std::uint64_t frameNumber = 0U;
    double realTimeSeconds = 0.0;
    double gameTimeSeconds = 0.0;
    double fixedTimeSeconds = 0.0;
    double animationTimeSeconds = 0.0;
    double fixedAccumulatorSeconds = 0.0;
    double realDeltaSeconds = 0.0;
    double gameDeltaSeconds = 0.0;
    double animationDeltaSeconds = 0.0;
    bool paused = false;

    [[nodiscard]] bool operator==(const UnifiedTimeStateUVE&) const = default;
};

struct UnifiedTimeAdvanceInputUVE final {
    double realDeltaSeconds = 0.0;
    double gameTimeScale = 1.0;
    double animationTimeScale = 1.0;
    double fixedStepSeconds = 1.0 / 60.0;
    std::size_t maximumFixedSteps = 8U;
    bool paused = false;
};

struct UnifiedTimeAdvanceResultUVE final {
    UnifiedTimeStateUVE state;
    std::size_t fixedSteps = 0U;
    bool inputClamped = false;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return state.realDeltaSeconds >= 0.0 && state.gameDeltaSeconds >= 0.0 &&
               state.animationDeltaSeconds >= 0.0;
    }
};

// Shared time-domain state is a pure transition contract. The engine timer remains responsible
// for wall-clock sampling; this contract only advances copied values for game/fixed/animation users.
class UnifiedTimeContractUVE final {
public:
    static constexpr double kMaximumFrameDeltaSecondsUVE = 0.25;
    static constexpr std::size_t kMaximumFixedStepsPerAdvanceUVE = 8U;

    [[nodiscard]] static UnifiedTimeStateUVE AdvanceUVE(
        const UnifiedTimeStateUVE& previous, UnifiedTimeAdvanceInputUVE input,
        std::size_t& outFixedSteps, bool& outInputClamped) noexcept;
};

struct TransformPoseUVE final {
    Math::Vector3UVE position;
    Math::QuaternionUVE rotation;
    Math::Vector3UVE scale{1.0F, 1.0F, 1.0F};

    [[nodiscard]] bool operator==(const TransformPoseUVE&) const noexcept = default;
};

inline constexpr std::size_t kMaximumSkeletonJointsUVE = 256U;
/// Maximum byte length for skeleton/joint identifiers; accepted identifiers are non-empty where
/// required and never contain embedded NUL bytes.
inline constexpr std::size_t kMaximumAnimationIdentifierBytesUVE = 128U;

struct SkeletonJointUVE final {
    std::string jointId;
    std::string parentJointId;

    [[nodiscard]] bool operator==(const SkeletonJointUVE&) const noexcept = default;
};

struct SkeletonDefinitionUVE final {
    std::string skeletonId;
    std::vector<SkeletonJointUVE> joints;

    [[nodiscard]] bool operator==(const SkeletonDefinitionUVE&) const noexcept = default;
};

struct SkeletonRetargetMapEntryUVE final {
    std::string sourceJointId;
    std::string targetJointId;

    [[nodiscard]] bool operator==(const SkeletonRetargetMapEntryUVE&) const noexcept = default;
};

struct PoseBufferUVE final {
    std::string skeletonId;
    std::vector<TransformPoseUVE> localJoints;

    [[nodiscard]] bool operator==(const PoseBufferUVE&) const noexcept = default;
};

struct AnimationEvaluationContextUVE final {
    UnifiedTimeStateUVE time;
    double sampleTimeSeconds = 0.0;

    [[nodiscard]] bool operator==(const AnimationEvaluationContextUVE&) const noexcept = default;
};

enum class AnimationContractValidationCodeUVE : std::uint8_t {
    Valid = 0,
    InvalidIdentifier,
    CapacityExceeded,
    DuplicateJoint,
    UnknownParent,
    InvalidPose,
    SkeletonMismatch,
    InvalidTime,
};

struct AnimationContractValidationResultUVE final {
    AnimationContractValidationCodeUVE code = AnimationContractValidationCodeUVE::Valid;
    std::size_t index = 0U;
    std::string message;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return code == AnimationContractValidationCodeUVE::Valid;
    }
};

struct PoseSampleUVE final {
    double timeSeconds = 0.0;
    TransformPoseUVE pose;

    [[nodiscard]] bool operator==(const PoseSampleUVE&) const noexcept = default;
};

[[nodiscard]] AnimationContractValidationResultUVE ValidateSkeletonDefinitionUVE(
    const SkeletonDefinitionUVE& skeleton) noexcept;
[[nodiscard]] AnimationContractValidationResultUVE ValidatePoseBufferUVE(
    const PoseBufferUVE& pose, const SkeletonDefinitionUVE& skeleton) noexcept;
[[nodiscard]] AnimationContractValidationResultUVE ValidateSkeletonRetargetMapUVE(
    const SkeletonDefinitionUVE& sourceSkeleton, const SkeletonDefinitionUVE& targetSkeleton,
    const std::vector<SkeletonRetargetMapEntryUVE>& map) noexcept;
[[nodiscard]] AnimationContractValidationResultUVE ValidateAnimationEvaluationContextUVE(
    const AnimationEvaluationContextUVE& context) noexcept;

[[nodiscard]] bool IsFiniteTransformPoseUVE(const TransformPoseUVE& pose) noexcept;
[[nodiscard]] bool TryNormalizeTransformPoseUVE(const TransformPoseUVE& pose,
                                                 TransformPoseUVE& outNormalized) noexcept;

} // namespace UVE::Core
