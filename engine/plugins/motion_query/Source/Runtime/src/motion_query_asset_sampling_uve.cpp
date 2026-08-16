// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/plugins/motion_query_asset_sampling_uve.h"

#include <algorithm>
#include <cmath>
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

} // namespace

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
            const float normalizedValue = range.maximum == range.minimum
                                              ? 0.0F
                                              : (value - range.minimum) / (range.maximum - range.minimum);
            normalized.values.push_back(std::clamp(normalizedValue, 0.0F, 1.0F));
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
