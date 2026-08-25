// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/asset/resource_dependency_graph_uve.h"
#include "uve/core/animation_clip_uve.h"
#include "uve/plugins/motion_query_feature_channels_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace UVE::Plugins {

struct MotionQueryNormalizationRangeUVE final {
    float minimum = 0.0F;
    float maximum = 1.0F;

    [[nodiscard]] bool operator==(const MotionQueryNormalizationRangeUVE&) const = default;
};

struct MotionQueryDerivedDataKeyUVE final {
    Asset::ResourceHandleUVE source;
    std::uint32_t schemaVersion = 1U;
    std::uint32_t samplerVersion = 1U;
    std::uint32_t normalizationVersion = 1U;

    [[nodiscard]] bool operator==(const MotionQueryDerivedDataKeyUVE&) const noexcept = default;
};

struct MotionQueryAssetSamplingRequestUVE final {
    static constexpr std::size_t kMaximumSamplesUVE = 4096U;

    MotionQueryDerivedDataKeyUVE key;
    std::vector<UVE::Core::MotionQueryFeatureVectorUVE> samples;
    std::vector<MotionQueryNormalizationRangeUVE> normalizationRanges;
};

struct MotionQueryDerivedDataUVE final {
    MotionQueryDerivedDataKeyUVE key;
    std::vector<UVE::Core::MotionQueryFeatureVectorUVE> normalizedSamples;

    [[nodiscard]] bool operator==(const MotionQueryDerivedDataUVE&) const = default;
};

struct MotionQueryClipSamplingRequestUVE final {
    static constexpr std::size_t kMaximumClipsUVE = 256U;

    MotionQueryDerivedDataKeyUVE key;
    UVE::Core::MotionQueryFeatureSchemaUVE featureSchema;
    std::vector<double> trajectoryOffsets;
    std::vector<UVE::Core::AnimationClipUVE> clips;
    double samplePeriodSeconds = 1.0 / 30.0;
    bool looping = false;
};

struct MotionQueryClipSamplingResultUVE final {
    enum class Code : std::uint8_t {
        Accepted = 0,
        InvalidSourceHandle,
        InvalidSamplingPeriod,
        EmptyClips,
        CapacityExceeded,
        InvalidClip,
        InvalidFeatureSchema,
        InvalidTrajectoryOffsets,
        PoseSampleFailed,
        FeatureBuildFailed,
    };

    Code code = Code::InvalidSourceHandle;
    std::size_t index = 0U;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == Code::Accepted;
    }
};

[[nodiscard]] MotionQueryClipSamplingResultUVE BuildMotionQueryDatabaseFromClipsUVE(
    const MotionQueryClipSamplingRequestUVE& request,
    UVE::Core::MotionMatchingDatabaseUVE& outDatabase) noexcept;

enum class MotionQueryAssetSamplingCodeUVE : std::uint8_t {
    Accepted = 0,
    InvalidSourceHandle,
    CapacityExceeded,
    EmptySamples,
    InconsistentFeatureDimensions,
    InvalidNormalizationRange,
    NonFiniteFeature,
    StaleDependency,
};

struct MotionQueryAssetSamplingResultUVE final {
    MotionQueryAssetSamplingCodeUVE code = MotionQueryAssetSamplingCodeUVE::InvalidSourceHandle;
    std::size_t index = 0U;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == MotionQueryAssetSamplingCodeUVE::Accepted;
    }
};

[[nodiscard]] MotionQueryAssetSamplingResultUVE BuildMotionQueryDerivedDataUVE(
    const MotionQueryAssetSamplingRequestUVE& request,
    MotionQueryDerivedDataUVE& outDerivedData) noexcept;

[[nodiscard]] bool IsMotionQueryDerivedDataCurrentUVE(
    const MotionQueryDerivedDataUVE& derivedData,
    const Asset::ResourceDependencySnapshotUVE& dependencies) noexcept;

} // namespace UVE::Plugins
