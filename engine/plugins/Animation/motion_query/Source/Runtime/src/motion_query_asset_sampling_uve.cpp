// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/plugins/motion_query_asset_sampling_uve.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <utility>

namespace UVE::Plugins {
namespace {

[[nodiscard]] MotionQueryAssetSamplingResultUVE MakeSamplingErrorUVE(
    MotionQueryAssetSamplingCodeUVE code, std::size_t index, const char* message) noexcept {
    return MotionQueryAssetSamplingResultUVE{code, index, message};
}

[[nodiscard]] bool IsFiniteRangeUVE(const MotionQueryNormalizationRangeUVE& range) noexcept {
    return std::isfinite(range.minimum) && std::isfinite(range.maximum) &&
           range.maximum >= range.minimum;
}

[[nodiscard]] MotionQueryClipSamplingResultUVE MakeClipSamplingErrorUVE(
    MotionQueryClipSamplingResultUVE::Code code, std::size_t index,
    const char* message) noexcept {
    return MotionQueryClipSamplingResultUVE{code, index, message};
}

} // namespace

MotionQueryClipSamplingResultUVE BuildMotionQueryDatabaseFromClipsUVE(
    const MotionQueryClipSamplingRequestUVE& request,
    UVE::Core::MotionMatchingDatabaseUVE& outDatabase) noexcept {
    if (request.key.source.guid == Asset::kInvalidAssetGuidUVE ||
        request.key.source.generation == 0U) {
        return MakeClipSamplingErrorUVE(MotionQueryClipSamplingResultUVE::Code::InvalidSourceHandle, 0U,
                                        "motion query clip sampling source handle is invalid");
    }
    if (request.clips.empty()) {
        return MakeClipSamplingErrorUVE(MotionQueryClipSamplingResultUVE::Code::EmptyClips, 0U,
                                        "motion query clip sampling requires at least one clip");
    }
    if (request.clips.size() > MotionQueryClipSamplingRequestUVE::kMaximumClipsUVE) {
        return MakeClipSamplingErrorUVE(MotionQueryClipSamplingResultUVE::Code::CapacityExceeded, 0U,
                                        "motion query clip sampling exceeds its bounded clip capacity");
    }
    if (!std::isfinite(request.samplePeriodSeconds) || request.samplePeriodSeconds <= 0.0) {
        return MakeClipSamplingErrorUVE(MotionQueryClipSamplingResultUVE::Code::InvalidSamplingPeriod, 0U,
                                        "motion query clip sampling period must be finite and positive");
    }
    if (request.key.schemaVersion != request.featureSchema.version) {
        return MakeClipSamplingErrorUVE(MotionQueryClipSamplingResultUVE::Code::InvalidFeatureSchema, 0U,
                                        "motion query clip sampling schema version does not match its key");
    }
    const UVE::Core::MotionQueryFeatureValidationResultUVE schemaValidation =
        UVE::Core::ValidateMotionQueryFeatureSchemaUVE(request.featureSchema);
    if (!schemaValidation.IsValidUVE()) {
        return MakeClipSamplingErrorUVE(MotionQueryClipSamplingResultUVE::Code::InvalidFeatureSchema,
                                        schemaValidation.index, schemaValidation.message.c_str());
    }
    if (request.trajectoryOffsets.size() > UVE::Core::MotionQueryUVE::kMaximumTrajectorySamplesUVE) {
        return MakeClipSamplingErrorUVE(MotionQueryClipSamplingResultUVE::Code::InvalidTrajectoryOffsets, 0U,
                                        "motion query clip trajectory offsets exceed capacity");
    }
    for (std::size_t channelIndex = 0U; channelIndex < request.featureSchema.channels.size();
         ++channelIndex) {
        const UVE::Core::MotionQueryFeatureChannelUVE& channel =
            request.featureSchema.channels[channelIndex];
        const bool usesTrajectory =
            channel.kind == UVE::Core::MotionQueryFeatureChannelKindUVE::TrajectoryPosition ||
            channel.kind == UVE::Core::MotionQueryFeatureChannelKindUVE::TrajectoryDistance ||
            channel.kind == UVE::Core::MotionQueryFeatureChannelKindUVE::TrajectoryTime;
        if (usesTrajectory && channel.trajectorySampleIndex >= request.trajectoryOffsets.size()) {
            return MakeClipSamplingErrorUVE(MotionQueryClipSamplingResultUVE::Code::InvalidFeatureSchema,
                                            channelIndex,
                                            "motion query feature channel references an unavailable trajectory sample");
        }
    }
    double previousOffset = -std::numeric_limits<double>::infinity();
    for (std::size_t index = 0U; index < request.trajectoryOffsets.size(); ++index) {
        const double offset = request.trajectoryOffsets[index];
        if (!std::isfinite(offset) || offset < 0.0 || offset < previousOffset) {
            return MakeClipSamplingErrorUVE(MotionQueryClipSamplingResultUVE::Code::InvalidTrajectoryOffsets,
                                            index, "motion query clip trajectory offsets must be finite and sorted");
        }
        previousOffset = offset;
    }

    std::size_t totalCandidateCount = 0U;
    for (std::size_t clipIndex = 0U; clipIndex < request.clips.size(); ++clipIndex) {
        const UVE::Core::AnimationClipUVE& clip = request.clips[clipIndex];
        if (!UVE::Core::ValidateAnimationClipUVE(clip).IsValidUVE()) {
            return MakeClipSamplingErrorUVE(MotionQueryClipSamplingResultUVE::Code::InvalidClip,
                                            clipIndex, "motion query clip is invalid");
        }
        for (std::size_t previousClipIndex = 0U; previousClipIndex < clipIndex; ++previousClipIndex) {
            if (request.clips[previousClipIndex].clipId == clip.clipId) {
                return MakeClipSamplingErrorUVE(MotionQueryClipSamplingResultUVE::Code::InvalidClip,
                                                clipIndex, "motion query clip identifiers must be unique");
            }
        }
        const double sampleCountReal = std::ceil(clip.durationSeconds / request.samplePeriodSeconds) + 1.0;
        if (!std::isfinite(sampleCountReal) ||
            sampleCountReal > static_cast<double>(UVE::Core::MotionMatchingDatabaseUVE::kMaximumCandidatesUVE)) {
            return MakeClipSamplingErrorUVE(MotionQueryClipSamplingResultUVE::Code::CapacityExceeded,
                                            clipIndex, "motion query clip sample count exceeds database capacity");
        }
        const std::size_t sampleCount = static_cast<std::size_t>(sampleCountReal);
        if (sampleCount > UVE::Core::MotionMatchingDatabaseUVE::kMaximumCandidatesUVE - totalCandidateCount) {
            return MakeClipSamplingErrorUVE(MotionQueryClipSamplingResultUVE::Code::CapacityExceeded,
                                            clipIndex, "motion query clip database candidate capacity is exceeded");
        }
        totalCandidateCount += sampleCount;
    }

    UVE::Core::MotionMatchingDatabaseUVE database;
    database.candidates.reserve(totalCandidateCount);
    for (const UVE::Core::AnimationClipUVE& clip : request.clips) {
        const std::size_t sampleCount = static_cast<std::size_t>(
            std::ceil(clip.durationSeconds / request.samplePeriodSeconds) + 1.0);
        for (std::size_t sampleIndex = 0U; sampleIndex < sampleCount; ++sampleIndex) {
            const double sampleTime = std::min(
                clip.durationSeconds, static_cast<double>(sampleIndex) * request.samplePeriodSeconds);
            const double previousTime = sampleIndex == 0U
                                            ? sampleTime
                                            : std::max(0.0, sampleTime - request.samplePeriodSeconds);
            UVE::Core::TransformPoseUVE previousPose;
            UVE::Core::TransformPoseUVE currentPose;
            if (!UVE::Core::TrySampleAnimationClipUVE(clip, previousTime, request.looping, previousPose) ||
                !UVE::Core::TrySampleAnimationClipUVE(clip, sampleTime, request.looping, currentPose)) {
                return MakeClipSamplingErrorUVE(MotionQueryClipSamplingResultUVE::Code::PoseSampleFailed,
                                                sampleIndex, "motion query clip pose sampling failed");
            }
            std::vector<UVE::Core::MotionTrajectorySampleUVE> trajectory;
            trajectory.reserve(request.trajectoryOffsets.size());
            for (std::size_t trajectoryIndex = 0U;
                 trajectoryIndex < request.trajectoryOffsets.size(); ++trajectoryIndex) {
                const double targetTime = sampleTime + request.trajectoryOffsets[trajectoryIndex];
                if (!std::isfinite(targetTime)) {
                    return MakeClipSamplingErrorUVE(
                        MotionQueryClipSamplingResultUVE::Code::PoseSampleFailed, sampleIndex,
                        "motion query clip trajectory sample time is non-finite");
                }
                UVE::Core::TransformPoseUVE futurePose;
                if (!UVE::Core::TrySampleAnimationClipUVE(clip, targetTime, request.looping, futurePose)) {
                    return MakeClipSamplingErrorUVE(MotionQueryClipSamplingResultUVE::Code::PoseSampleFailed,
                                                    sampleIndex, "motion query future pose sampling failed");
                }
                trajectory.push_back(UVE::Core::MotionTrajectorySampleUVE{
                    request.trajectoryOffsets[trajectoryIndex],
                    UVE::Math::Vector3UVE{futurePose.position.x - currentPose.position.x,
                                          futurePose.position.y - currentPose.position.y,
                                          futurePose.position.z - currentPose.position.z}});
            }
            UVE::Core::MotionQueryUVE feature;
            if (!UVE::Core::TryBuildMotionQueryUVE(previousPose, currentPose,
                                                   request.samplePeriodSeconds, trajectory, feature)) {
                return MakeClipSamplingErrorUVE(MotionQueryClipSamplingResultUVE::Code::FeatureBuildFailed,
                                                sampleIndex, "motion query feature construction failed");
            }
            UVE::Core::MotionMatchingCandidateUVE candidate;
            candidate.candidateId = clip.clipId + "@" + std::to_string(sampleIndex);
            candidate.sourceClipId = clip.clipId;
            candidate.sampleTimeSeconds = sampleTime;
            candidate.feature = std::move(feature);
            database.candidates.push_back(std::move(candidate));
        }
    }
    if (!UVE::Core::ValidateMotionMatchingDatabaseUVE(database).IsValidUVE()) {
        return MakeClipSamplingErrorUVE(MotionQueryClipSamplingResultUVE::Code::FeatureBuildFailed, 0U,
                                        "motion query clip database validation failed");
    }
    outDatabase = std::move(database);
    return MotionQueryClipSamplingResultUVE{MotionQueryClipSamplingResultUVE::Code::Accepted, 0U,
                                            "motion query database built from clips"};
}

MotionQueryAssetSamplingResultUVE BuildMotionQueryDerivedDataUVE(
    const MotionQueryAssetSamplingRequestUVE& request,
    MotionQueryDerivedDataUVE& outDerivedData) noexcept {
    if (request.key.source.guid == Asset::kInvalidAssetGuidUVE ||
        request.key.source.generation == 0U) {
        return MakeSamplingErrorUVE(MotionQueryAssetSamplingCodeUVE::InvalidSourceHandle, 0U,
                                    "motion query sampling source handle is invalid");
    }
    if (request.samples.empty()) {
        return MakeSamplingErrorUVE(MotionQueryAssetSamplingCodeUVE::EmptySamples, 0U,
                                    "motion query sampling requires at least one sample");
    }
    if (request.samples.size() > MotionQueryAssetSamplingRequestUVE::kMaximumSamplesUVE) {
        return MakeSamplingErrorUVE(MotionQueryAssetSamplingCodeUVE::CapacityExceeded, 0U,
                                    "motion query sampling exceeds its bounded sample capacity");
    }
    const std::size_t featureDimension = request.samples.front().values.size();
    if (featureDimension == 0U || request.normalizationRanges.size() != featureDimension) {
        return MakeSamplingErrorUVE(MotionQueryAssetSamplingCodeUVE::InconsistentFeatureDimensions,
                                    0U, "motion query sampling feature dimensions are inconsistent");
    }
    for (std::size_t index = 0U; index < request.normalizationRanges.size(); ++index) {
        if (!IsFiniteRangeUVE(request.normalizationRanges[index])) {
            return MakeSamplingErrorUVE(MotionQueryAssetSamplingCodeUVE::InvalidNormalizationRange,
                                        index, "motion query normalization range is invalid");
        }
    }

    MotionQueryDerivedDataUVE derived;
    derived.key = request.key;
    derived.normalizedSamples.reserve(request.samples.size());
    for (std::size_t sampleIndex = 0U; sampleIndex < request.samples.size(); ++sampleIndex) {
        const UVE::Core::MotionQueryFeatureVectorUVE& source = request.samples[sampleIndex];
        if (source.values.size() != featureDimension || !std::isfinite(source.totalWeight)) {
            return MakeSamplingErrorUVE(MotionQueryAssetSamplingCodeUVE::InconsistentFeatureDimensions,
                                        sampleIndex, "motion query sample dimensions are inconsistent");
        }
        UVE::Core::MotionQueryFeatureVectorUVE normalized;
        normalized.values.reserve(featureDimension);
        normalized.totalWeight = source.totalWeight;
        for (std::size_t featureIndex = 0U; featureIndex < featureDimension; ++featureIndex) {
            const float value = source.values[featureIndex];
            if (!std::isfinite(value)) {
                return MakeSamplingErrorUVE(MotionQueryAssetSamplingCodeUVE::NonFiniteFeature,
                                            featureIndex, "motion query feature value is non-finite");
            }
            const MotionQueryNormalizationRangeUVE& range = request.normalizationRanges[featureIndex];
            const double normalizedValue = range.maximum == range.minimum
                                               ? 0.0
                                               : (static_cast<double>(value) -
                                                  static_cast<double>(range.minimum)) /
                                                     (static_cast<double>(range.maximum) -
                                                      static_cast<double>(range.minimum));
            if (!std::isfinite(normalizedValue)) {
                return MakeSamplingErrorUVE(MotionQueryAssetSamplingCodeUVE::NonFiniteFeature,
                                            featureIndex,
                                            "motion query normalized feature value is non-finite");
            }
            normalized.values.push_back(
                static_cast<float>(std::clamp(normalizedValue, 0.0, 1.0)));
        }
        derived.normalizedSamples.push_back(std::move(normalized));
    }
    outDerivedData = std::move(derived);
    return MotionQueryAssetSamplingResultUVE{MotionQueryAssetSamplingCodeUVE::Accepted, 0U,
                                             "accepted"};
}

bool IsMotionQueryDerivedDataCurrentUVE(
    const MotionQueryDerivedDataUVE& derivedData,
    const Asset::ResourceDependencySnapshotUVE& dependencies) noexcept {
    for (const Asset::ResourceDependencyEntryUVE& entry : dependencies.entries) {
        if (entry.handle == derivedData.key.source) {
            return true;
        }
    }
    return false;
}

} // namespace UVE::Plugins
