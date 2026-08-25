// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/plugins/motion_query_uve.h"

#include "uve/math/quaternion_uve.h"
#include "uve/math/vector3_uve.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace UVE::Core {
namespace {

constexpr float kMinimumVectorLengthSquaredUVE = 1.0e-8F;
constexpr float kTieToleranceUVE = 1.0e-6F;

[[nodiscard]] bool IsFiniteVectorUVE(const Math::Vector3UVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool IsFiniteNonNegativeUVE(double value) noexcept {
    return std::isfinite(value) && value >= 0.0;
}

[[nodiscard]] MotionQueryValidationResultUVE MakeQueryErrorUVE(
    MotionQueryValidationCodeUVE code, std::size_t index, const char* message) noexcept {
    return MotionQueryValidationResultUVE{code, index, message};
}

[[nodiscard]] MotionMatchingDatabaseValidationResultUVE MakeDatabaseErrorUVE(
    MotionMatchingDatabaseValidationCodeUVE code, std::size_t index,
    const char* message) noexcept {
    return MotionMatchingDatabaseValidationResultUVE{code, index, message};
}

[[nodiscard]] double DistanceSquaredUVE(const Math::Vector3UVE& lhs,
                                        const Math::Vector3UVE& rhs) noexcept {
    if (!IsFiniteVectorUVE(lhs) || !IsFiniteVectorUVE(rhs)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const double x = static_cast<double>(lhs.x) - static_cast<double>(rhs.x);
    const double y = static_cast<double>(lhs.y) - static_cast<double>(rhs.y);
    const double z = static_cast<double>(lhs.z) - static_cast<double>(rhs.z);
    const double scale = std::max(std::fabs(x), std::max(std::fabs(y), std::fabs(z)));
    if (!std::isfinite(scale)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (scale == 0.0) {
        return 0.0;
    }
    const double scaledLengthSquared = (x / scale) * (x / scale) +
                                       (y / scale) * (y / scale) +
                                       (z / scale) * (z / scale);
    if (!std::isfinite(scaledLengthSquared)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const double distanceSquared = (scale * scale) * scaledLengthSquared;
    return std::isfinite(distanceSquared) ? distanceSquared : std::numeric_limits<double>::quiet_NaN();
}

[[nodiscard]] bool TryBuildFiniteVelocityUVE(const TransformPoseUVE& previousPose,
                                             const TransformPoseUVE& currentPose,
                                             const double deltaSeconds,
                                             Math::Vector3UVE& outVelocity) noexcept {
    const double inverseDeltaSeconds = 1.0 / deltaSeconds;
    const double x = (static_cast<double>(currentPose.position.x) -
                      static_cast<double>(previousPose.position.x)) * inverseDeltaSeconds;
    const double y = (static_cast<double>(currentPose.position.y) -
                      static_cast<double>(previousPose.position.y)) * inverseDeltaSeconds;
    const double z = (static_cast<double>(currentPose.position.z) -
                      static_cast<double>(previousPose.position.z)) * inverseDeltaSeconds;
    const double maximumFloat = static_cast<double>(std::numeric_limits<float>::max());
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) ||
        x < -maximumFloat || x > maximumFloat || y < -maximumFloat || y > maximumFloat ||
        z < -maximumFloat || z > maximumFloat) {
        return false;
    }
    outVelocity = Math::Vector3UVE{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)};
    return IsFiniteVectorUVE(outVelocity);
}

[[nodiscard]] bool TryNormalizeDirectionUVE(const Math::Vector3UVE& value,
                                            Math::Vector3UVE& outDirection) noexcept {
    if (!IsFiniteVectorUVE(value)) {
        return false;
    }
    const double x = static_cast<double>(value.x);
    const double y = static_cast<double>(value.y);
    const double z = static_cast<double>(value.z);
    const double scale = std::max(std::fabs(x), std::max(std::fabs(y), std::fabs(z)));
    if (!std::isfinite(scale) || scale == 0.0) {
        return false;
    }
    const double scaledLengthSquared = (x / scale) * (x / scale) +
                                       (y / scale) * (y / scale) +
                                       (z / scale) * (z / scale);
    if (!std::isfinite(scaledLengthSquared) ||
        scaledLengthSquared < static_cast<double>(kMinimumVectorLengthSquaredUVE)) {
        return false;
    }
    outDirection = Math::NormalizeUVE(value);
    return IsFiniteVectorUVE(outDirection);
}

[[nodiscard]] bool HasCandidateIdentifierUVE(const MotionMatchingDatabaseUVE& database,
                                             const std::string& identifier,
                                             std::size_t beforeIndex) noexcept {
    for (std::size_t index = 0U; index < beforeIndex; ++index) {
        if (database.candidates[index].candidateId == identifier) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool HasSameTrajectorySchemaUVE(
    const TimeSampledTrajectoryUVE& lhs,
    const TimeSampledTrajectoryUVE& rhs) noexcept {
    if (lhs.samples.size() != rhs.samples.size()) {
        return false;
    }
    for (std::size_t index = 0U; index < lhs.samples.size(); ++index) {
        if (lhs.samples[index].offsetSeconds != rhs.samples[index].offsetSeconds) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool IsBetterMatchUVE(float candidateCost, const std::string& candidateId,
                                    double candidateTime, std::size_t candidateIndex,
                                    float bestCost, const std::string& bestId,
                                    double bestTime, std::size_t bestIndex) noexcept {
    if (candidateCost + kTieToleranceUVE < bestCost) {
        return true;
    }
    if (std::fabs(candidateCost - bestCost) > kTieToleranceUVE) {
        return false;
    }
    if (candidateId != bestId) {
        return candidateId < bestId;
    }
    if (candidateTime != bestTime) {
        return candidateTime < bestTime;
    }
    return candidateIndex < bestIndex;
}

} // namespace

MotionQueryValidationResultUVE ValidateMotionQueryUVE(const MotionQueryUVE& query) noexcept {
    if (query.trajectory.size() > MotionQueryUVE::kMaximumTrajectorySamplesUVE) {
        return MakeQueryErrorUVE(MotionQueryValidationCodeUVE::CapacityExceeded, 0U,
                                 "motion query trajectory exceeds its bounded capacity");
    }
    if (!IsFiniteVectorUVE(query.rootVelocity) || !IsFiniteVectorUVE(query.facingDirection)) {
        return MakeQueryErrorUVE(MotionQueryValidationCodeUVE::InvalidVector, 0U,
                                 "motion query contains a non-finite vector");
    }
    if (Math::LengthSquaredUVE(query.facingDirection) < kMinimumVectorLengthSquaredUVE) {
        return MakeQueryErrorUVE(MotionQueryValidationCodeUVE::InvalidVector, 0U,
                                 "motion query facing direction must be non-zero");
    }
    const AnimationContractValidationResultUVE skeletonValidation =
        ValidateSkeletonDefinitionUVE(query.skeleton);
    if (!skeletonValidation.IsValidUVE()) {
        return MakeQueryErrorUVE(MotionQueryValidationCodeUVE::InvalidPose, skeletonValidation.index,
                                 "motion query skeleton contract is invalid");
    }
    const AnimationContractValidationResultUVE poseValidation =
        ValidatePoseBufferUVE(query.pose, query.skeleton);
    if (!poseValidation.IsValidUVE()) {
        return MakeQueryErrorUVE(MotionQueryValidationCodeUVE::InvalidPose, poseValidation.index,
                                 "motion query pose contract is invalid");
    }
    const AnimationContractValidationResultUVE timeValidation =
        ValidateAnimationEvaluationContextUVE(query.evaluationContext);
    if (!timeValidation.IsValidUVE()) {
        return MakeQueryErrorUVE(MotionQueryValidationCodeUVE::InvalidEvaluationTime, 0U,
                                 "motion query evaluation context is invalid");
    }

    const TimeSampledTrajectoryValidationResultUVE trajectoryValidation =
        ValidateTimeSampledTrajectoryUVE(query.trajectory);
    if (!trajectoryValidation.IsValidUVE()) {
        const MotionQueryValidationCodeUVE code =
            trajectoryValidation.code == TimeSampledTrajectoryValidationCodeUVE::CapacityExceeded
                ? MotionQueryValidationCodeUVE::CapacityExceeded
                : trajectoryValidation.code == TimeSampledTrajectoryValidationCodeUVE::UnsortedSamples
                    ? MotionQueryValidationCodeUVE::UnsortedTrajectory
                    : MotionQueryValidationCodeUVE::InvalidTrajectoryTime;
        return MakeQueryErrorUVE(code, trajectoryValidation.index, trajectoryValidation.message.c_str());
    }
    double previousOffsetSeconds = -std::numeric_limits<double>::infinity();
    for (std::size_t index = 0U; index < query.trajectory.size(); ++index) {
        const MotionTrajectorySampleUVE& sample = query.trajectory[index];
        if (!IsFiniteNonNegativeUVE(sample.offsetSeconds) ||
            !IsFiniteVectorUVE(sample.relativePosition)) {
            return MakeQueryErrorUVE(MotionQueryValidationCodeUVE::InvalidTrajectoryTime, index,
                                     "motion query trajectory contains invalid time or position");
        }
        if (sample.offsetSeconds < previousOffsetSeconds) {
            return MakeQueryErrorUVE(MotionQueryValidationCodeUVE::UnsortedTrajectory, index,
                                     "motion query trajectory offsets must be sorted");
        }
        previousOffsetSeconds = sample.offsetSeconds;
    }

    return MotionQueryValidationResultUVE{MotionQueryValidationCodeUVE::Valid, 0U, "valid"};
}

MotionMatchingDatabaseValidationResultUVE ValidateMotionMatchingDatabaseUVE(
    const MotionMatchingDatabaseUVE& database) noexcept {
    if (database.candidates.empty()) {
        return MakeDatabaseErrorUVE(MotionMatchingDatabaseValidationCodeUVE::EmptyDatabase, 0U,
                                    "motion matching database must contain a candidate");
    }
    if (database.candidates.size() > MotionMatchingDatabaseUVE::kMaximumCandidatesUVE) {
        return MakeDatabaseErrorUVE(MotionMatchingDatabaseValidationCodeUVE::CapacityExceeded, 0U,
                                    "motion matching database exceeds its bounded capacity");
    }

    const TimeSampledTrajectoryUVE& schema = database.candidates.front().feature.trajectory;
    for (std::size_t index = 0U; index < database.candidates.size(); ++index) {
        const MotionMatchingCandidateUVE& candidate = database.candidates[index];
        if (candidate.candidateId.empty() ||
            candidate.candidateId.size() > MotionMatchingCandidateUVE::kMaximumIdentifierBytesUVE) {
            return MakeDatabaseErrorUVE(
                MotionMatchingDatabaseValidationCodeUVE::InvalidCandidateIdentifier, index,
                "motion matching candidate identifier is empty or too long");
        }
        if (HasCandidateIdentifierUVE(database, candidate.candidateId, index)) {
            return MakeDatabaseErrorUVE(
                MotionMatchingDatabaseValidationCodeUVE::DuplicateCandidateIdentifier, index,
                "motion matching candidate identifiers must be unique");
        }
        if (!IsFiniteNonNegativeUVE(candidate.sampleTimeSeconds)) {
            return MakeDatabaseErrorUVE(MotionMatchingDatabaseValidationCodeUVE::InvalidSampleTime,
                                        index, "motion matching candidate sample time is invalid");
        }
        if (!ValidateMotionQueryUVE(candidate.feature).IsValidUVE()) {
            return MakeDatabaseErrorUVE(MotionMatchingDatabaseValidationCodeUVE::InvalidFeature,
                                        index, "motion matching candidate feature is invalid");
        }
        if (!HasSameTrajectorySchemaUVE(schema, candidate.feature.trajectory)) {
            return MakeDatabaseErrorUVE(
                MotionMatchingDatabaseValidationCodeUVE::InconsistentTrajectorySchema, index,
                "motion matching candidates must share one trajectory schema");
        }
    }

    return MotionMatchingDatabaseValidationResultUVE{
        MotionMatchingDatabaseValidationCodeUVE::Valid, 0U, "valid"};
}

bool TryBuildMotionQueryUVE(const TransformPoseUVE& previousPose,
                           const TransformPoseUVE& currentPose, double deltaSeconds,
                           const TimeSampledTrajectoryUVE& futureTrajectory,
                           MotionQueryUVE& outQuery) noexcept {
    TransformPoseUVE normalizedPreviousPose;
    TransformPoseUVE normalizedCurrentPose;
    if (!TryNormalizeTransformPoseUVE(previousPose, normalizedPreviousPose) ||
        !TryNormalizeTransformPoseUVE(currentPose, normalizedCurrentPose) ||
        !std::isfinite(deltaSeconds) || deltaSeconds <= 0.0 ||
        !ValidateTimeSampledTrajectoryUVE(futureTrajectory).IsValidUVE()) {
        return false;
    }

    MotionQueryUVE candidate;
    if (!TryBuildFiniteVelocityUVE(normalizedPreviousPose, normalizedCurrentPose, deltaSeconds,
                                   candidate.rootVelocity)) {
        return false;
    }
    candidate.facingDirection = Math::RotateVectorUVE(
        normalizedCurrentPose.rotation, Math::Vector3UVE{0.0F, 0.0F, 1.0F});
    candidate.trajectory = futureTrajectory;
    if (!ValidateMotionQueryUVE(candidate).IsValidUVE()) {
        return false;
    }

    outQuery = std::move(candidate);
    return true;
}

MotionMatchingResultUVE FindBestMotionMatchUVE(const MotionQueryUVE& query,
                                               const MotionMatchingDatabaseUVE& database,
                                               MotionMatchingWeightsUVE weights) noexcept {
    const MotionQueryValidationResultUVE queryValidation = ValidateMotionQueryUVE(query);
    if (!queryValidation.IsValidUVE()) {
        return MotionMatchingResultUVE{MotionMatchingResultCodeUVE::InvalidQuery, 0U, 0U, 0.0F,
                                      queryValidation.message};
    }
    const MotionMatchingDatabaseValidationResultUVE databaseValidation =
        ValidateMotionMatchingDatabaseUVE(database);
    if (!databaseValidation.IsValidUVE()) {
        return MotionMatchingResultUVE{MotionMatchingResultCodeUVE::InvalidDatabase, 0U, 0U, 0.0F,
                                      databaseValidation.message};
    }
    const float totalWeight = weights.velocityWeight + weights.facingWeight +
                               weights.trajectoryWeight;
    if (!std::isfinite(weights.velocityWeight) || !std::isfinite(weights.facingWeight) ||
        !std::isfinite(weights.trajectoryWeight) || !std::isfinite(totalWeight) ||
        weights.velocityWeight < 0.0F || weights.facingWeight < 0.0F ||
        weights.trajectoryWeight < 0.0F || totalWeight <= 0.0F) {
        return MotionMatchingResultUVE{MotionMatchingResultCodeUVE::InvalidWeights, 0U, 0U, 0.0F,
                                      "motion matching weights must be finite, non-negative, and non-zero"};
    }

    Math::Vector3UVE normalizedQueryFacing;
    if (!TryNormalizeDirectionUVE(query.facingDirection, normalizedQueryFacing)) {
        return MotionMatchingResultUVE{MotionMatchingResultCodeUVE::InvalidQuery, 0U, 0U, 0.0F,
                                      "motion query facing direction cannot be normalized"};
    }

    const TimeSampledTrajectoryUVE& schema = database.candidates.front().feature.trajectory;
    bool hasBest = false;
    std::size_t bestIndex = 0U;
    float bestCost = std::numeric_limits<float>::infinity();
    std::size_t evaluated = 0U;
    std::string bestId;
    double bestTime = 0.0;

    for (std::size_t index = 0U; index < database.candidates.size(); ++index) {
        const MotionMatchingCandidateUVE& candidate = database.candidates[index];
        if (!HasSameTrajectorySchemaUVE(query.trajectory, schema)) {
            break;
        }

        Math::Vector3UVE normalizedCandidateFacing;
        if (!TryNormalizeDirectionUVE(candidate.feature.facingDirection, normalizedCandidateFacing)) {
            continue;
        }

        double cost = static_cast<double>(weights.velocityWeight) *
                      DistanceSquaredUVE(query.rootVelocity, candidate.feature.rootVelocity);
        const float facingDot = std::clamp(Math::DotUVE(normalizedQueryFacing,
                                                         normalizedCandidateFacing), -1.0F, 1.0F);
        cost += static_cast<double>(weights.facingWeight) * (1.0 - static_cast<double>(facingDot));
        if (!query.trajectory.empty()) {
            double trajectoryCost = 0.0;
            for (std::size_t sampleIndex = 0U; sampleIndex < query.trajectory.size(); ++sampleIndex) {
                trajectoryCost += static_cast<double>(DistanceSquaredUVE(
                    query.trajectory[sampleIndex].relativePosition,
                    candidate.feature.trajectory[sampleIndex].relativePosition));
            }
            const double trajectoryContribution =
                static_cast<double>(weights.trajectoryWeight) * trajectoryCost /
                static_cast<double>(query.trajectory.size());
            if (!std::isfinite(trajectoryContribution)) {
                cost = std::numeric_limits<double>::infinity();
            } else {
                cost += trajectoryContribution;
            }
        }
        if (!std::isfinite(cost) || cost > static_cast<double>(std::numeric_limits<float>::max())) {
            continue;
        }
        const float narrowedCost = static_cast<float>(cost);
        if (!std::isfinite(narrowedCost)) {
            continue;
        }

        ++evaluated;
        if (!hasBest || IsBetterMatchUVE(narrowedCost, candidate.candidateId, candidate.sampleTimeSeconds,
                                         index, bestCost, bestId, bestTime, bestIndex)) {
            hasBest = true;
            bestIndex = index;
            bestCost = narrowedCost;
            bestId = candidate.candidateId;
            bestTime = candidate.sampleTimeSeconds;
        }
    }

    if (!hasBest) {
        return MotionMatchingResultUVE{MotionMatchingResultCodeUVE::NoComparableCandidate, 0U,
                                      evaluated, 0.0F, "no comparable motion matching candidate"};
    }
    return MotionMatchingResultUVE{MotionMatchingResultCodeUVE::Matched, bestIndex, evaluated,
                                   bestCost, "matched"};
}

} // namespace UVE::Core
