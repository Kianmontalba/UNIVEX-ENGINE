// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/core/future_trajectory_contract_uve.h"
#include "uve/core/time_pose_contract_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace UVE::Core {

// A copied feature vector shared by motion-query consumers; runtime playback remains elsewhere.
struct MotionQueryUVE final {
    static constexpr std::size_t kMaximumTrajectorySamplesUVE = 32U;

    Math::Vector3UVE rootVelocity;
    Math::Vector3UVE facingDirection{0.0F, 0.0F, 1.0F};
    TimeSampledTrajectoryUVE trajectory;
    SkeletonDefinitionUVE skeleton;
    PoseBufferUVE pose;
    AnimationEvaluationContextUVE evaluationContext;
};

enum class MotionQueryValidationCodeUVE : std::uint8_t {
    Valid = 0,
    CapacityExceeded,
    InvalidVector,
    InvalidTrajectoryTime,
    UnsortedTrajectory,
    InvalidPose,
    InvalidEvaluationTime,
};

struct MotionQueryValidationResultUVE final {
    MotionQueryValidationCodeUVE code = MotionQueryValidationCodeUVE::InvalidVector;
    std::size_t index = 0U;
    std::string message;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return code == MotionQueryValidationCodeUVE::Valid;
    }
};

struct MotionMatchingCandidateUVE final {
    static constexpr std::size_t kMaximumIdentifierBytesUVE = 128U;

    std::string candidateId;
    std::string sourceClipId;
    double sampleTimeSeconds = 0.0;
    MotionQueryUVE feature;
};

struct MotionMatchingDatabaseUVE final {
    static constexpr std::size_t kMaximumCandidatesUVE = 4096U;

    std::vector<MotionMatchingCandidateUVE> candidates;
};

enum class MotionMatchingDatabaseValidationCodeUVE : std::uint8_t {
    Valid = 0,
    EmptyDatabase,
    CapacityExceeded,
    InvalidCandidateIdentifier,
    DuplicateCandidateIdentifier,
    InvalidSampleTime,
    InvalidFeature,
    InconsistentTrajectorySchema,
};

struct MotionMatchingDatabaseValidationResultUVE final {
    MotionMatchingDatabaseValidationCodeUVE code =
        MotionMatchingDatabaseValidationCodeUVE::EmptyDatabase;
    std::size_t index = 0U;
    std::string message;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return code == MotionMatchingDatabaseValidationCodeUVE::Valid;
    }
};

struct MotionMatchingWeightsUVE final {
    float velocityWeight = 1.0F;
    float facingWeight = 1.0F;
    float trajectoryWeight = 1.0F;
};

enum class MotionMatchingResultCodeUVE : std::uint8_t {
    Matched = 0,
    InvalidQuery,
    InvalidDatabase,
    InvalidWeights,
    NoComparableCandidate,
};

struct MotionMatchingResultUVE final {
    MotionMatchingResultCodeUVE code = MotionMatchingResultCodeUVE::NoComparableCandidate;
    std::size_t candidateIndex = 0U;
    std::size_t candidatesEvaluated = 0U;
    float cost = 0.0F;
    std::string message;

    [[nodiscard]] bool IsMatchUVE() const noexcept {
        return code == MotionMatchingResultCodeUVE::Matched;
    }
};

[[nodiscard]] MotionQueryValidationResultUVE ValidateMotionQueryUVE(
    const MotionQueryUVE& query) noexcept;

[[nodiscard]] MotionMatchingDatabaseValidationResultUVE ValidateMotionMatchingDatabaseUVE(
    const MotionMatchingDatabaseUVE& database) noexcept;

[[nodiscard]] bool TryBuildMotionQueryUVE(
    const TransformPoseUVE& previousPose, const TransformPoseUVE& currentPose,
    double deltaSeconds, const TimeSampledTrajectoryUVE& futureTrajectory,
    MotionQueryUVE& outQuery) noexcept;

[[nodiscard]] MotionMatchingResultUVE FindBestMotionMatchUVE(
    const MotionQueryUVE& query, const MotionMatchingDatabaseUVE& database,
    MotionMatchingWeightsUVE weights) noexcept;

} // namespace UVE::Core
