// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/plugins/motion_query_feature_channels_uve.h"

#include <cmath>

namespace UVE::Core {
namespace {

[[nodiscard]] MotionQueryFeatureValidationResultUVE MakeFeatureErrorUVE(
    MotionQueryFeatureValidationCodeUVE code, std::size_t index, const char* message) noexcept {
    return MotionQueryFeatureValidationResultUVE{code, index, message};
}

[[nodiscard]] bool HasDuplicateChannelIdUVE(const MotionQueryFeatureSchemaUVE& schema,
                                            std::size_t beforeIndex) noexcept {
    for (std::size_t index = 0U; index < beforeIndex; ++index) {
        if (schema.channels[index].id == schema.channels[beforeIndex].id) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool IsFiniteNonNegativeUVE(float value) noexcept {
    return std::isfinite(value) && value >= 0.0F;
}

} // namespace

MotionQueryFeatureValidationResultUVE ValidateMotionQueryFeatureSchemaUVE(
    const MotionQueryFeatureSchemaUVE& schema) noexcept {
    if (schema.channels.empty()) {
        return MakeFeatureErrorUVE(MotionQueryFeatureValidationCodeUVE::EmptySchema, 0U,
                                   "motion query feature schema must contain a channel");
    }
    if (schema.channels.size() > MotionQueryFeatureSchemaUVE::kMaximumChannelsUVE) {
        return MakeFeatureErrorUVE(MotionQueryFeatureValidationCodeUVE::CapacityExceeded, 0U,
                                   "motion query feature schema exceeds its bounded channel capacity");
    }
    for (std::size_t index = 0U; index < schema.channels.size(); ++index) {
        const MotionQueryFeatureChannelUVE& channel = schema.channels[index];
        if (channel.id.empty() || channel.id.size() > MotionQueryFeatureChannelUVE::kMaximumIdBytesUVE) {
            return MakeFeatureErrorUVE(MotionQueryFeatureValidationCodeUVE::InvalidChannelId, index,
                                       "motion query feature channel identifier is invalid");
        }
        if (HasDuplicateChannelIdUVE(schema, index)) {
            return MakeFeatureErrorUVE(MotionQueryFeatureValidationCodeUVE::DuplicateChannelId, index,
                                       "motion query feature channel identifiers must be unique");
        }
        if (!IsFiniteNonNegativeUVE(channel.weight)) {
            return MakeFeatureErrorUVE(MotionQueryFeatureValidationCodeUVE::InvalidWeight, index,
                                       "motion query feature channel weight must be finite and non-negative");
        }
        if ((channel.kind == MotionQueryFeatureChannelKindUVE::TrajectoryPosition ||
             channel.kind == MotionQueryFeatureChannelKindUVE::TrajectoryDistance ||
             channel.kind == MotionQueryFeatureChannelKindUVE::TrajectoryTime) &&
            channel.trajectorySampleIndex >= MotionQueryUVE::kMaximumTrajectorySamplesUVE) {
            return MakeFeatureErrorUVE(MotionQueryFeatureValidationCodeUVE::InvalidSampleIndex, index,
                                       "motion query feature channel trajectory index exceeds bounds");
        }
    }
    return MotionQueryFeatureValidationResultUVE{MotionQueryFeatureValidationCodeUVE::Valid, 0U,
                                                 "valid"};
}

MotionQueryFeatureValidationResultUVE TryBuildMotionQueryFeatureVectorUVE(
    const MotionQueryUVE& query, const MotionQueryFeatureSchemaUVE& schema,
    MotionQueryFeatureVectorUVE& outVector) noexcept {
    const MotionQueryFeatureValidationResultUVE schemaValidation =
        ValidateMotionQueryFeatureSchemaUVE(schema);
    if (!schemaValidation.IsValidUVE()) {
        return schemaValidation;
    }
    if (!ValidateMotionQueryUVE(query).IsValidUVE()) {
        return MakeFeatureErrorUVE(MotionQueryFeatureValidationCodeUVE::InvalidQuery, 0U,
                                   "motion query is invalid for feature extraction");
    }

    MotionQueryFeatureVectorUVE vector;
    vector.values.reserve(schema.channels.size());
    for (const MotionQueryFeatureChannelUVE& channel : schema.channels) {
        const bool needsTrajectorySample =
            channel.kind == MotionQueryFeatureChannelKindUVE::TrajectoryPosition ||
            channel.kind == MotionQueryFeatureChannelKindUVE::TrajectoryDistance ||
            channel.kind == MotionQueryFeatureChannelKindUVE::TrajectoryTime;
        if (needsTrajectorySample && channel.trajectorySampleIndex >= query.trajectory.size()) {
            return MakeFeatureErrorUVE(MotionQueryFeatureValidationCodeUVE::InvalidSampleIndex,
                                       vector.values.size(), "feature channel trajectory index is unavailable");
        }
        float value = 0.0F;
        switch (channel.kind) {
        case MotionQueryFeatureChannelKindUVE::RootVelocity:
            value = Math::LengthSquaredUVE(query.rootVelocity);
            break;
        case MotionQueryFeatureChannelKindUVE::FacingDirection:
            value = query.facingDirection.z;
            break;
        case MotionQueryFeatureChannelKindUVE::TrajectoryPosition:
            value = Math::LengthSquaredUVE(
                query.trajectory[channel.trajectorySampleIndex].relativePosition);
            break;
        case MotionQueryFeatureChannelKindUVE::TrajectoryDistance:
            if (channel.trajectorySampleIndex == 0U) {
                value = Math::LengthSquaredUVE(query.trajectory.front().relativePosition);
            } else {
                const Math::Vector3UVE delta =
                    query.trajectory[channel.trajectorySampleIndex].relativePosition -
                    query.trajectory[channel.trajectorySampleIndex - 1U].relativePosition;
                value = Math::LengthSquaredUVE(delta);
            }
            break;
        case MotionQueryFeatureChannelKindUVE::TrajectoryTime:
            value = static_cast<float>(query.trajectory[channel.trajectorySampleIndex].offsetSeconds);
            break;
        }
        if (!std::isfinite(value)) {
            return MakeFeatureErrorUVE(MotionQueryFeatureValidationCodeUVE::InvalidQuery,
                                       vector.values.size(), "feature extraction produced a non-finite value");
        }
        vector.values.push_back(value * channel.weight);
        vector.totalWeight += channel.weight;
    }
    outVector = vector;
    return MotionQueryFeatureValidationResultUVE{MotionQueryFeatureValidationCodeUVE::Valid, 0U,
                                                 "valid"};
}

MotionQueryCandidateFilterCodeUVE EvaluateMotionQueryCandidateFilterUVE(
    const MotionMatchingCandidateUVE& candidate, const MotionMatchingResultUVE& result,
    const MotionQueryCandidateFilterUVE& filter) noexcept {
    if (filter.sourceClipId.has_value() && candidate.sourceClipId != *filter.sourceClipId) {
        return MotionQueryCandidateFilterCodeUVE::SourceClipMismatch;
    }
    if (filter.minimumSampleTimeSeconds.has_value() &&
        (!std::isfinite(*filter.minimumSampleTimeSeconds) ||
         candidate.sampleTimeSeconds < *filter.minimumSampleTimeSeconds)) {
        return MotionQueryCandidateFilterCodeUVE::SampleTimeBelowMinimum;
    }
    if (filter.maximumSampleTimeSeconds.has_value() &&
        (!std::isfinite(*filter.maximumSampleTimeSeconds) ||
         candidate.sampleTimeSeconds > *filter.maximumSampleTimeSeconds)) {
        return MotionQueryCandidateFilterCodeUVE::SampleTimeAboveMaximum;
    }
    if (filter.maximumCost.has_value() &&
        (!std::isfinite(*filter.maximumCost) || result.cost > *filter.maximumCost)) {
        return MotionQueryCandidateFilterCodeUVE::CostAboveMaximum;
    }
    return MotionQueryCandidateFilterCodeUVE::Accepted;
}

MotionQueryChooserRowUVE BuildMotionQueryChooserRowUVE(
    const MotionMatchingCandidateUVE& candidate, const MotionMatchingResultUVE& result) noexcept {
    return MotionQueryChooserRowUVE{candidate.candidateId, candidate.sourceClipId,
                                   candidate.sampleTimeSeconds, result.cost,
                                   result.candidatesEvaluated};
}

} // namespace UVE::Core
