// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/plugins/motion_query_uve.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace UVE::Core {

enum class MotionQueryFeatureChannelKindUVE : std::uint8_t {
    RootVelocity = 0,
    FacingDirection,
    TrajectoryPosition,
    TrajectoryDistance,
    TrajectoryTime,
};

struct MotionQueryFeatureChannelUVE final {
    static constexpr std::size_t kMaximumIdBytesUVE = 128U;

    std::string id;
    MotionQueryFeatureChannelKindUVE kind = MotionQueryFeatureChannelKindUVE::RootVelocity;
    std::size_t trajectorySampleIndex = 0U;
    float weight = 1.0F;

    [[nodiscard]] bool operator==(const MotionQueryFeatureChannelUVE&) const = default;
};

struct MotionQueryFeatureSchemaUVE final {
    static constexpr std::size_t kMaximumChannelsUVE = 64U;

    std::uint32_t version = 1U;
    std::vector<MotionQueryFeatureChannelUVE> channels;

    [[nodiscard]] bool operator==(const MotionQueryFeatureSchemaUVE&) const = default;
};

enum class MotionQueryFeatureValidationCodeUVE : std::uint8_t {
    Valid = 0,
    EmptySchema,
    CapacityExceeded,
    InvalidChannelId,
    DuplicateChannelId,
    InvalidWeight,
    InvalidSampleIndex,
    InvalidQuery,
};

struct MotionQueryFeatureValidationResultUVE final {
    MotionQueryFeatureValidationCodeUVE code = MotionQueryFeatureValidationCodeUVE::EmptySchema;
    std::size_t index = 0U;
    std::string message;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return code == MotionQueryFeatureValidationCodeUVE::Valid;
    }
};

struct MotionQueryFeatureVectorUVE final {
    std::vector<float> values;
    float totalWeight = 0.0F;

    [[nodiscard]] bool operator==(const MotionQueryFeatureVectorUVE&) const = default;
};

enum class MotionQueryCandidateFilterCodeUVE : std::uint8_t {
    Accepted = 0,
    SourceClipMismatch,
    SampleTimeBelowMinimum,
    SampleTimeAboveMaximum,
    CostAboveMaximum,
};

struct MotionQueryCandidateFilterUVE final {
    std::optional<std::string> sourceClipId;
    std::optional<double> minimumSampleTimeSeconds;
    std::optional<double> maximumSampleTimeSeconds;
    std::optional<float> maximumCost;
};

struct MotionQueryChooserRowUVE final {
    std::string candidateId;
    std::string sourceClipId;
    double sampleTimeSeconds = 0.0;
    float cost = 0.0F;
    std::size_t candidatesEvaluated = 0U;

    [[nodiscard]] bool operator==(const MotionQueryChooserRowUVE&) const = default;
};

[[nodiscard]] MotionQueryFeatureValidationResultUVE ValidateMotionQueryFeatureSchemaUVE(
    const MotionQueryFeatureSchemaUVE& schema) noexcept;

[[nodiscard]] MotionQueryFeatureValidationResultUVE TryBuildMotionQueryFeatureVectorUVE(
    const MotionQueryUVE& query, const MotionQueryFeatureSchemaUVE& schema,
    MotionQueryFeatureVectorUVE& outVector) noexcept;

[[nodiscard]] MotionQueryCandidateFilterCodeUVE EvaluateMotionQueryCandidateFilterUVE(
    const MotionMatchingCandidateUVE& candidate, const MotionMatchingResultUVE& result,
    const MotionQueryCandidateFilterUVE& filter) noexcept;

[[nodiscard]] MotionQueryChooserRowUVE BuildMotionQueryChooserRowUVE(
    const MotionMatchingCandidateUVE& candidate, const MotionMatchingResultUVE& result) noexcept;

} // namespace UVE::Core
