// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/plugins/motion_query_feature_channels_uve.h"

#include <algorithm>
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

[[nodiscard]] bool TrySquaredLengthUVE(const Math::Vector3UVE& value,
                                       double& outValue) noexcept {
    if (!std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z)) {
        return false;
    }
    const double x = static_cast<double>(value.x);
    const double y = static_cast<double>(value.y);
    const double z = static_cast<double>(value.z);
    const double scale = std::max(std::fabs(x), std::max(std::fabs(y), std::fabs(z)));
    if (!std::isfinite(scale)) {
        return false;
    }
    if (scale == 0.0) {
        outValue = 0.0;
        return true;
    }
    const double scaledLengthSquared = (x / scale) * (x / scale) +
                                       (y / scale) * (y / scale) +
                                       (z / scale) * (z / scale);
    if (!std::isfinite(scaledLengthSquared)) {
        return false;
    }
    outValue = (scale * scale) * scaledLengthSquared;
    return std::isfinite(outValue);
}

[[nodiscard]] bool TrySquaredDifferenceUVE(const Math::Vector3UVE& lhs,
                                           const Math::Vector3UVE& rhs,
                                           double& outValue) noexcept {
    const double x = static_cast<double>(lhs.x) - static_cast<double>(rhs.x);
    const double y = static_cast<double>(lhs.y) - static_cast<double>(rhs.y);
    const double z = static_cast<double>(lhs.z) - static_cast<double>(rhs.z);
    const double scale = std::max(std::fabs(x), std::max(std::fabs(y), std::fabs(z)));
    if (!std::isfinite(scale)) {
        return false;
    }
    if (scale == 0.0) {
        outValue = 0.0;
        return true;
    }
    const double scaledLengthSquared = (x / scale) * (x / scale) +
                                       (y / scale) * (y / scale) +
                                       (z / scale) * (z / scale);
    if (!std::isfinite(scaledLengthSquared)) {
        return false;
    }
    outValue = (scale * scale) * scaledLengthSquared;
    return std::isfinite(outValue);
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
        double value = 0.0;
        bool valueIsValid = true;
        switch (channel.kind) {
        case MotionQueryFeatureChannelKindUVE::RootVelocity:
            valueIsValid = TrySquaredLengthUVE(query.rootVelocity, value);
            break;
        case MotionQueryFeatureChannelKindUVE::FacingDirection:
            value = static_cast<double>(query.facingDirection.z);
            break;
        case MotionQueryFeatureChannelKindUVE::TrajectoryPosition:
            valueIsValid = TrySquaredLengthUVE(
                query.trajectory[channel.trajectorySampleIndex].relativePosition, value);
            break;
        case MotionQueryFeatureChannelKindUVE::TrajectoryDistance:
            if (channel.trajectorySampleIndex == 0U) {
                valueIsValid = TrySquaredLengthUVE(
                    query.trajectory.front().relativePosition, value);
            } else {
                valueIsValid = TrySquaredDifferenceUVE(
                    query.trajectory[channel.trajectorySampleIndex].relativePosition,
                    query.trajectory[channel.trajectorySampleIndex - 1U].relativePosition, value);
            }
            break;
        case MotionQueryFeatureChannelKindUVE::TrajectoryTime:
            value = static_cast<double>(static_cast<float>(
                query.trajectory[channel.trajectorySampleIndex].offsetSeconds));
            break;
        }
        const double weightedValue = value * static_cast<double>(channel.weight);
        const float narrowedWeightedValue = static_cast<float>(weightedValue);
        const float totalWeight = vector.totalWeight + channel.weight;
        if (!valueIsValid || !std::isfinite(value) || !std::isfinite(weightedValue) ||
            !std::isfinite(narrowedWeightedValue) || !std::isfinite(totalWeight)) {
            return MakeFeatureErrorUVE(MotionQueryFeatureValidationCodeUVE::InvalidQuery,
                                       vector.values.size(), "feature extraction produced a non-finite weighted value");
        }
        vector.values.push_back(narrowedWeightedValue);
        vector.totalWeight = totalWeight;
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
