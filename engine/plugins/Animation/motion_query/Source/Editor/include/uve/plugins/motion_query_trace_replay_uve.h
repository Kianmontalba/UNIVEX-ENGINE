// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/plugins/motion_query_debugging_uve.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace UVE::Plugins::Editor {

inline constexpr std::uint32_t kMotionQueryTraceReplayFixtureSchemaVersionUVE = 1U;
inline constexpr std::size_t kMotionQueryMaximumTraceReplayEventsUVE =
    kMotionQueryMaximumTraceEventsUVE;
inline constexpr std::size_t kMotionQueryMaximumTraceReplayPayloadBytesUVE = 1024U * 1024U;
inline constexpr std::size_t kMotionQueryTraceReplayNoMismatchIndexUVE =
    static_cast<std::size_t>(-1);
inline constexpr std::size_t kMotionQueryMaximumReplayDiagnosticSummaryBytesUVE = 256U;

struct MotionQueryTraceReplayEventUVE final {
    std::uint64_t sequence = 0U;
    std::uint64_t frameNumber = 0U;
    std::string kind;
    std::size_t candidatesConsidered = 0U;
    std::size_t candidatesEvaluated = 0U;
    float cost = 0.0F;
    std::optional<std::size_t> selectedCandidateIndex;
    std::uint8_t qualityTier = 0U;
    std::uint8_t continuityCode = 0U;
    bool continuityApplied = false;
    std::uint8_t transitionCode = 0U;
    bool transitionHeldPrevious = false;
    std::uint8_t telemetryCode = 0U;
    std::size_t telemetryIndexEntryCount = 0U;
    std::size_t telemetryCandidatesConsidered = 0U;
    bool telemetryBudgetSaturated = false;
    std::string provenance;

    [[nodiscard]] bool operator==(const MotionQueryTraceReplayEventUVE&) const = default;
};

struct MotionQueryTraceReplayCompatibilityUVE final {
    std::uint32_t schemaVersion = 1U;
    std::uint32_t samplerVersion = 1U;
    std::uint32_t normalizationVersion = 1U;
    std::uint64_t sourceGeneration = 0U;

    [[nodiscard]] bool operator==(const MotionQueryTraceReplayCompatibilityUVE&) const = default;
};

struct MotionQueryTraceReplayFixtureUVE final {
    std::uint32_t schemaVersion = kMotionQueryTraceReplayFixtureSchemaVersionUVE;
    bool truncated = false;
    std::optional<MotionQueryTraceReplayCompatibilityUVE> compatibility;
    std::vector<MotionQueryTraceReplayEventUVE> events;

    [[nodiscard]] bool operator==(const MotionQueryTraceReplayFixtureUVE&) const = default;
};

enum class MotionQueryTraceReplayComparisonCodeUVE : std::uint8_t {
    Match = 0,
    SchemaMismatch,
    InvalidFixture,
    EventCountMismatch,
    EventMismatch,
    CompatibilityMismatch,
    TruncationMismatch,
};

enum class MotionQueryTraceReplayMismatchFieldUVE : std::uint32_t {
    None = 0U,
    Sequence = 1U << 0U,
    FrameNumber = 1U << 1U,
    Kind = 1U << 2U,
    CandidatesConsidered = 1U << 3U,
    CandidatesEvaluated = 1U << 4U,
    Cost = 1U << 5U,
    SelectedCandidateIndex = 1U << 6U,
    QualityTier = 1U << 7U,
    ContinuityCode = 1U << 8U,
    ContinuityApplied = 1U << 9U,
    TransitionCode = 1U << 10U,
    TransitionHeldPrevious = 1U << 11U,
    TelemetryCode = 1U << 12U,
    TelemetryIndexEntryCount = 1U << 13U,
    TelemetryCandidatesConsidered = 1U << 14U,
    TelemetryBudgetSaturated = 1U << 15U,
    Provenance = 1U << 16U,
};

enum class MotionQueryTraceReplayCompatibilityMismatchFieldUVE : std::uint32_t {
    None = 0U,
    FixtureCompatibilityMissing = 1U << 0U,
    SchemaVersion = 1U << 1U,
    SamplerVersion = 1U << 2U,
    NormalizationVersion = 1U << 3U,
    SourceGeneration = 1U << 4U,
};

struct MotionQueryTraceReplayComparisonUVE final {
    MotionQueryTraceReplayComparisonCodeUVE code =
        MotionQueryTraceReplayComparisonCodeUVE::InvalidFixture;
    std::size_t comparedEventCount = 0U;
    std::size_t mismatchIndex = kMotionQueryTraceReplayNoMismatchIndexUVE;
    bool fixtureTruncated = false;
    bool snapshotTruncated = false;
    std::uint32_t mismatchFieldMask =
        static_cast<std::uint32_t>(MotionQueryTraceReplayMismatchFieldUVE::None);
    std::string message;
    std::string diagnosticSummary;
    std::uint32_t compatibilityMismatchMask =
        static_cast<std::uint32_t>(MotionQueryTraceReplayCompatibilityMismatchFieldUVE::None);
    std::string compatibilityDiagnosticSummary;

    [[nodiscard]] bool IsMatchUVE() const noexcept {
        return code == MotionQueryTraceReplayComparisonCodeUVE::Match;
    }

    [[nodiscard]] bool IsTruncatedUVE() const noexcept {
        return fixtureTruncated || snapshotTruncated;
    }
};

enum class MotionQueryTraceReplaySerializationCodeUVE : std::uint8_t {
    Accepted = 0,
    EmptyPayload,
    PayloadTooLarge,
    ParseError,
    SchemaMismatch,
    ForbiddenField,
    UnexpectedField,
    InvalidFixture,
};

struct MotionQueryTraceReplaySerializationResultUVE final {
    MotionQueryTraceReplaySerializationCodeUVE code =
        MotionQueryTraceReplaySerializationCodeUVE::InvalidFixture;
    std::string payload;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == MotionQueryTraceReplaySerializationCodeUVE::Accepted;
    }
};

struct MotionQueryTraceReplayDeserializationResultUVE final {
    MotionQueryTraceReplaySerializationCodeUVE code =
        MotionQueryTraceReplaySerializationCodeUVE::InvalidFixture;
    std::optional<MotionQueryTraceReplayFixtureUVE> fixture;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == MotionQueryTraceReplaySerializationCodeUVE::Accepted &&
               fixture.has_value();
    }
};

[[nodiscard]] MotionQueryTraceReplayFixtureUVE BuildMotionQueryTraceReplayFixtureUVE(
    const MotionQueryTraceSnapshotUVE& snapshot);

[[nodiscard]] MotionQueryTraceReplayFixtureUVE BuildMotionQueryTraceReplayFixtureUVE(
    const MotionQueryTraceSnapshotUVE& snapshot,
    const MotionQueryTraceReplayCompatibilityUVE& compatibility);

[[nodiscard]] MotionQueryTraceReplayComparisonUVE CompareMotionQueryTraceReplayFixtureUVE(
    const MotionQueryTraceReplayFixtureUVE& fixture,
    const MotionQueryTraceSnapshotUVE& snapshot);

[[nodiscard]] MotionQueryTraceReplayComparisonUVE CompareMotionQueryTraceReplayFixtureUVE(
    const MotionQueryTraceReplayFixtureUVE& fixture,
    const MotionQueryTraceSnapshotUVE& snapshot,
    const MotionQueryTraceReplayCompatibilityUVE& compatibility);

[[nodiscard]] MotionQueryTraceReplaySerializationResultUVE
SerializeMotionQueryTraceReplayFixtureUVE(
    const MotionQueryTraceReplayFixtureUVE& fixture);

[[nodiscard]] MotionQueryTraceReplayDeserializationResultUVE
DeserializeMotionQueryTraceReplayFixtureUVE(std::string_view payload);

} // namespace UVE::Plugins::Editor
