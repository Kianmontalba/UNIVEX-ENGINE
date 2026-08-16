// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_trace_replay_uve.h"

#include <array>
#include <cmath>
#include <string_view>
#include <utility>

#include <nlohmann/json.hpp>

namespace UVE::Plugins::Editor {
namespace {

using JsonUVE = nlohmann::json;

constexpr std::array<std::string_view, 3U> kForbiddenReplayFieldNamesUVE = {
    "timestampNanoseconds",
    "database",
    "message",
};

[[nodiscard]] bool HasAnyKeyUVE(const JsonUVE& document,
                                const std::array<std::string_view, 3U>& keys) {
    for (const std::string_view key : keys) {
        if (document.contains(key)) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool HasOnlyKeysUVE(const JsonUVE& document,
                                  const std::vector<std::string_view>& allowedKeys) {
    if (!document.is_object()) {
        return false;
    }
    for (const auto& item : document.items()) {
        bool allowed = false;
        for (const std::string_view key : allowedKeys) {
            if (item.key() == key) {
                allowed = true;
                break;
            }
        }
        if (!allowed) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] MotionQueryTraceReplayEventUVE BuildReplayEventUVE(
    const MotionQueryTraceEventUVE& event) {
    MotionQueryTraceReplayEventUVE replayEvent;
    replayEvent.sequence = event.sequence;
    replayEvent.frameNumber = event.frameNumber;
    replayEvent.kind = event.kind;
    replayEvent.candidatesConsidered = event.candidatesConsidered;
    replayEvent.candidatesEvaluated = event.candidatesEvaluated;
    replayEvent.cost = event.cost;
    replayEvent.selectedCandidateIndex = event.selectedCandidateIndex;
    replayEvent.qualityTier = event.qualityTier;
    replayEvent.continuityCode = event.continuityCode;
    replayEvent.continuityApplied = event.continuityApplied;
    replayEvent.transitionCode = event.transitionCode;
    replayEvent.transitionHeldPrevious = event.transitionHeldPrevious;
    replayEvent.telemetryCode = event.telemetryCode;
    replayEvent.telemetryIndexEntryCount = event.telemetryIndexEntryCount;
    replayEvent.telemetryCandidatesConsidered = event.telemetryCandidatesConsidered;
    replayEvent.telemetryBudgetSaturated = event.telemetryBudgetSaturated;
    replayEvent.provenance = event.provenance;
    return replayEvent;
}

[[nodiscard]] bool IsValidReplayEventUVE(const MotionQueryTraceReplayEventUVE& event) noexcept {
    return event.sequence != 0U && !event.kind.empty() &&
           event.kind.size() <= kMotionQueryMaximumDebugMessageBytesUVE &&
           event.provenance.size() <= kMotionQueryMaximumDebugMessageBytesUVE &&
           std::isfinite(event.cost) && event.cost >= 0.0F &&
           event.candidatesEvaluated <= event.candidatesConsidered;
}

[[nodiscard]] bool HasStrictlyIncreasingSequencesUVE(
    const std::vector<MotionQueryTraceReplayEventUVE>& events) noexcept {
    for (std::size_t index = 1U; index < events.size(); ++index) {
        if (events[index].sequence <= events[index - 1U].sequence) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool HasNonDecreasingFramesUVE(
    const std::vector<MotionQueryTraceReplayEventUVE>& events) noexcept {
    for (std::size_t index = 1U; index < events.size(); ++index) {
        if (events[index].frameNumber < events[index - 1U].frameNumber) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool IsValidReplayCompatibilityUVE(
    const MotionQueryTraceReplayCompatibilityUVE& compatibility) noexcept {
    return compatibility.schemaVersion != 0U && compatibility.samplerVersion != 0U &&
           compatibility.normalizationVersion != 0U && compatibility.sourceGeneration != 0U;
}

[[nodiscard]] bool IsValidReplayFixtureUVE(
    const MotionQueryTraceReplayFixtureUVE& fixture) noexcept {
    if (fixture.compatibility.has_value() &&
        !IsValidReplayCompatibilityUVE(*fixture.compatibility)) {
        return false;
    }
    if (fixture.events.size() > kMotionQueryMaximumTraceReplayEventsUVE ||
        !HasStrictlyIncreasingSequencesUVE(fixture.events) ||
        !HasNonDecreasingFramesUVE(fixture.events)) {
        return false;
    }
    for (const MotionQueryTraceReplayEventUVE& event : fixture.events) {
        if (!IsValidReplayEventUVE(event)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] JsonUVE ReplayEventToJsonUVE(const MotionQueryTraceReplayEventUVE& event) {
    return JsonUVE{
        {"sequence", event.sequence},
        {"frameNumber", event.frameNumber},
        {"kind", event.kind},
        {"candidatesConsidered", event.candidatesConsidered},
        {"candidatesEvaluated", event.candidatesEvaluated},
        {"cost", event.cost},
        {"selectedCandidateIndex", event.selectedCandidateIndex.has_value()
                                       ? JsonUVE(*event.selectedCandidateIndex)
                                       : JsonUVE(nullptr)},
        {"qualityTier", event.qualityTier},
        {"continuityCode", event.continuityCode},
        {"continuityApplied", event.continuityApplied},
        {"transitionCode", event.transitionCode},
        {"transitionHeldPrevious", event.transitionHeldPrevious},
        {"telemetryCode", event.telemetryCode},
        {"telemetryIndexEntryCount", event.telemetryIndexEntryCount},
        {"telemetryCandidatesConsidered", event.telemetryCandidatesConsidered},
        {"telemetryBudgetSaturated", event.telemetryBudgetSaturated},
        {"provenance", event.provenance},
    };
}

[[nodiscard]] JsonUVE ReplayCompatibilityToJsonUVE(
    const MotionQueryTraceReplayCompatibilityUVE& compatibility) {
    return JsonUVE{{"schemaVersion", compatibility.schemaVersion},
                   {"samplerVersion", compatibility.samplerVersion},
                   {"normalizationVersion", compatibility.normalizationVersion},
                   {"sourceGeneration", compatibility.sourceGeneration}};
}

[[nodiscard]] MotionQueryTraceReplayCompatibilityUVE ReplayCompatibilityFromJsonUVE(
    const JsonUVE& document) {
    return MotionQueryTraceReplayCompatibilityUVE{
        document.at("schemaVersion").get<std::uint32_t>(),
        document.at("samplerVersion").get<std::uint32_t>(),
        document.at("normalizationVersion").get<std::uint32_t>(),
        document.at("sourceGeneration").get<std::uint64_t>(),
    };
}

[[nodiscard]] MotionQueryTraceReplayEventUVE ReplayEventFromJsonUVE(const JsonUVE& document) {
    MotionQueryTraceReplayEventUVE event;
    event.sequence = document.at("sequence").get<std::uint64_t>();
    event.frameNumber = document.at("frameNumber").get<std::uint64_t>();
    event.kind = document.at("kind").get<std::string>();
    event.candidatesConsidered = document.at("candidatesConsidered").get<std::size_t>();
    event.candidatesEvaluated = document.at("candidatesEvaluated").get<std::size_t>();
    event.cost = document.at("cost").get<float>();
    if (!document.at("selectedCandidateIndex").is_null()) {
        event.selectedCandidateIndex =
            document.at("selectedCandidateIndex").get<std::size_t>();
    }
    event.qualityTier = document.at("qualityTier").get<std::uint8_t>();
    event.continuityCode = document.at("continuityCode").get<std::uint8_t>();
    event.continuityApplied = document.at("continuityApplied").get<bool>();
    event.transitionCode = document.at("transitionCode").get<std::uint8_t>();
    event.transitionHeldPrevious = document.at("transitionHeldPrevious").get<bool>();
    event.telemetryCode = document.at("telemetryCode").get<std::uint8_t>();
    event.telemetryIndexEntryCount = document.at("telemetryIndexEntryCount").get<std::size_t>();
    event.telemetryCandidatesConsidered =
        document.at("telemetryCandidatesConsidered").get<std::size_t>();
    event.telemetryBudgetSaturated = document.at("telemetryBudgetSaturated").get<bool>();
    event.provenance = document.at("provenance").get<std::string>();
    return event;
}

[[nodiscard]] std::uint32_t ClassifyReplayEventMismatchFieldsUVE(
    const MotionQueryTraceReplayEventUVE& expected,
    const MotionQueryTraceReplayEventUVE& actual) noexcept {
    std::uint32_t mask = static_cast<std::uint32_t>(MotionQueryTraceReplayMismatchFieldUVE::None);
    const auto addIfDifferent = [&mask](const bool different,
                                        const MotionQueryTraceReplayMismatchFieldUVE field) {
        if (different) {
            mask |= static_cast<std::uint32_t>(field);
        }
    };
    addIfDifferent(expected.sequence != actual.sequence, MotionQueryTraceReplayMismatchFieldUVE::Sequence);
    addIfDifferent(expected.frameNumber != actual.frameNumber,
                   MotionQueryTraceReplayMismatchFieldUVE::FrameNumber);
    addIfDifferent(expected.kind != actual.kind, MotionQueryTraceReplayMismatchFieldUVE::Kind);
    addIfDifferent(expected.candidatesConsidered != actual.candidatesConsidered,
                   MotionQueryTraceReplayMismatchFieldUVE::CandidatesConsidered);
    addIfDifferent(expected.candidatesEvaluated != actual.candidatesEvaluated,
                   MotionQueryTraceReplayMismatchFieldUVE::CandidatesEvaluated);
    addIfDifferent(expected.cost != actual.cost, MotionQueryTraceReplayMismatchFieldUVE::Cost);
    addIfDifferent(expected.selectedCandidateIndex != actual.selectedCandidateIndex,
                   MotionQueryTraceReplayMismatchFieldUVE::SelectedCandidateIndex);
    addIfDifferent(expected.qualityTier != actual.qualityTier,
                   MotionQueryTraceReplayMismatchFieldUVE::QualityTier);
    addIfDifferent(expected.continuityCode != actual.continuityCode,
                   MotionQueryTraceReplayMismatchFieldUVE::ContinuityCode);
    addIfDifferent(expected.continuityApplied != actual.continuityApplied,
                   MotionQueryTraceReplayMismatchFieldUVE::ContinuityApplied);
    addIfDifferent(expected.transitionCode != actual.transitionCode,
                   MotionQueryTraceReplayMismatchFieldUVE::TransitionCode);
    addIfDifferent(expected.transitionHeldPrevious != actual.transitionHeldPrevious,
                   MotionQueryTraceReplayMismatchFieldUVE::TransitionHeldPrevious);
    addIfDifferent(expected.telemetryCode != actual.telemetryCode,
                   MotionQueryTraceReplayMismatchFieldUVE::TelemetryCode);
    addIfDifferent(expected.telemetryIndexEntryCount != actual.telemetryIndexEntryCount,
                   MotionQueryTraceReplayMismatchFieldUVE::TelemetryIndexEntryCount);
    addIfDifferent(expected.telemetryCandidatesConsidered != actual.telemetryCandidatesConsidered,
                   MotionQueryTraceReplayMismatchFieldUVE::TelemetryCandidatesConsidered);
    addIfDifferent(expected.telemetryBudgetSaturated != actual.telemetryBudgetSaturated,
                   MotionQueryTraceReplayMismatchFieldUVE::TelemetryBudgetSaturated);
    addIfDifferent(expected.provenance != actual.provenance,
                   MotionQueryTraceReplayMismatchFieldUVE::Provenance);
    return mask;
}

[[nodiscard]] std::string SummarizeReplayEventMismatchFieldsUVE(const std::uint32_t mask) {
    constexpr std::array<std::pair<MotionQueryTraceReplayMismatchFieldUVE, std::string_view>, 17U> fields = {{
        {MotionQueryTraceReplayMismatchFieldUVE::Sequence, "sequence"},
        {MotionQueryTraceReplayMismatchFieldUVE::FrameNumber, "frameNumber"},
        {MotionQueryTraceReplayMismatchFieldUVE::Kind, "kind"},
        {MotionQueryTraceReplayMismatchFieldUVE::CandidatesConsidered, "candidatesConsidered"},
        {MotionQueryTraceReplayMismatchFieldUVE::CandidatesEvaluated, "candidatesEvaluated"},
        {MotionQueryTraceReplayMismatchFieldUVE::Cost, "cost"},
        {MotionQueryTraceReplayMismatchFieldUVE::SelectedCandidateIndex, "selectedCandidateIndex"},
        {MotionQueryTraceReplayMismatchFieldUVE::QualityTier, "qualityTier"},
        {MotionQueryTraceReplayMismatchFieldUVE::ContinuityCode, "continuityCode"},
        {MotionQueryTraceReplayMismatchFieldUVE::ContinuityApplied, "continuityApplied"},
        {MotionQueryTraceReplayMismatchFieldUVE::TransitionCode, "transitionCode"},
        {MotionQueryTraceReplayMismatchFieldUVE::TransitionHeldPrevious, "transitionHeldPrevious"},
        {MotionQueryTraceReplayMismatchFieldUVE::TelemetryCode, "telemetryCode"},
        {MotionQueryTraceReplayMismatchFieldUVE::TelemetryIndexEntryCount, "telemetryIndexEntryCount"},
        {MotionQueryTraceReplayMismatchFieldUVE::TelemetryCandidatesConsidered, "telemetryCandidatesConsidered"},
        {MotionQueryTraceReplayMismatchFieldUVE::TelemetryBudgetSaturated, "telemetryBudgetSaturated"},
        {MotionQueryTraceReplayMismatchFieldUVE::Provenance, "provenance"},
    }};
    std::string summary;
    for (const auto& [field, label] : fields) {
        if ((mask & static_cast<std::uint32_t>(field)) == 0U) {
            continue;
        }
        const std::size_t separatorBytes = summary.empty() ? 0U : 1U;
        if (summary.size() + separatorBytes + label.size() >
            kMotionQueryMaximumReplayDiagnosticSummaryBytesUVE) {
            if (summary.size() + separatorBytes + 3U <=
                kMotionQueryMaximumReplayDiagnosticSummaryBytesUVE) {
                summary.append(separatorBytes, ',');
                summary += "...";
            }
            break;
        }
        if (separatorBytes != 0U) {
            summary.push_back(',');
        }
        summary += label;
    }
    return summary;
}

[[nodiscard]] std::uint32_t ClassifyReplayCompatibilityMismatchUVE(
    const MotionQueryTraceReplayFixtureUVE& fixture,
    const MotionQueryTraceReplayCompatibilityUVE& expected) noexcept {
    if (!fixture.compatibility.has_value()) {
        return static_cast<std::uint32_t>(MotionQueryTraceReplayCompatibilityMismatchFieldUVE::
                                              FixtureCompatibilityMissing);
    }
    const MotionQueryTraceReplayCompatibilityUVE& actual = *fixture.compatibility;
    std::uint32_t mask = static_cast<std::uint32_t>(MotionQueryTraceReplayCompatibilityMismatchFieldUVE::None);
    const auto addIfDifferent = [&mask](const bool differs,
                                        const MotionQueryTraceReplayCompatibilityMismatchFieldUVE field) {
        if (differs) {
            mask |= static_cast<std::uint32_t>(field);
        }
    };
    addIfDifferent(actual.schemaVersion != expected.schemaVersion,
                   MotionQueryTraceReplayCompatibilityMismatchFieldUVE::SchemaVersion);
    addIfDifferent(actual.samplerVersion != expected.samplerVersion,
                   MotionQueryTraceReplayCompatibilityMismatchFieldUVE::SamplerVersion);
    addIfDifferent(actual.normalizationVersion != expected.normalizationVersion,
                   MotionQueryTraceReplayCompatibilityMismatchFieldUVE::NormalizationVersion);
    addIfDifferent(actual.sourceGeneration != expected.sourceGeneration,
                   MotionQueryTraceReplayCompatibilityMismatchFieldUVE::SourceGeneration);
    return mask;
}

[[nodiscard]] std::string SummarizeReplayCompatibilityMismatchUVE(const std::uint32_t mask) {
    constexpr std::array<std::pair<MotionQueryTraceReplayCompatibilityMismatchFieldUVE, std::string_view>, 5U>
        fields = {{
            {MotionQueryTraceReplayCompatibilityMismatchFieldUVE::FixtureCompatibilityMissing, "compatibilityMissing"},
            {MotionQueryTraceReplayCompatibilityMismatchFieldUVE::SchemaVersion, "schemaVersion"},
            {MotionQueryTraceReplayCompatibilityMismatchFieldUVE::SamplerVersion, "samplerVersion"},
            {MotionQueryTraceReplayCompatibilityMismatchFieldUVE::NormalizationVersion, "normalizationVersion"},
            {MotionQueryTraceReplayCompatibilityMismatchFieldUVE::SourceGeneration, "sourceGeneration"},
        }};
    std::string summary;
    for (const auto& [field, label] : fields) {
        if ((mask & static_cast<std::uint32_t>(field)) == 0U) {
            continue;
        }
        if (!summary.empty()) {
            summary.push_back(',');
        }
        summary += label;
        if (summary.size() >= kMotionQueryMaximumReplayDiagnosticSummaryBytesUVE) {
            summary.resize(kMotionQueryMaximumReplayDiagnosticSummaryBytesUVE);
            break;
        }
    }
    return summary;
}

[[nodiscard]] MotionQueryTraceReplayComparisonUVE MakeComparisonUVE(
    const MotionQueryTraceReplayComparisonCodeUVE code,
    const std::size_t comparedEventCount,
    const std::size_t mismatchIndex,
    const bool fixtureTruncated,
    const bool snapshotTruncated,
    const std::string_view message,
    const std::uint32_t mismatchFieldMask =
        static_cast<std::uint32_t>(MotionQueryTraceReplayMismatchFieldUVE::None),
    const std::string_view diagnosticSummary = {},
    const std::uint32_t compatibilityMismatchMask =
        static_cast<std::uint32_t>(MotionQueryTraceReplayCompatibilityMismatchFieldUVE::None),
    const std::string_view compatibilityDiagnosticSummary = {}) {
    return MotionQueryTraceReplayComparisonUVE{
        code,
        comparedEventCount,
        mismatchIndex,
        fixtureTruncated,
        snapshotTruncated,
        mismatchFieldMask,
        std::string(message),
        std::string(diagnosticSummary),
        compatibilityMismatchMask,
        std::string(compatibilityDiagnosticSummary),
    };
}

[[nodiscard]] MotionQueryTraceReplaySerializationResultUVE MakeSerializationResultUVE(
    const MotionQueryTraceReplaySerializationCodeUVE code,
    std::string payload,
    const std::string_view message) {
    return MotionQueryTraceReplaySerializationResultUVE{code, std::move(payload), std::string(message)};
}

} // namespace

MotionQueryTraceReplayFixtureUVE BuildMotionQueryTraceReplayFixtureUVE(
    const MotionQueryTraceSnapshotUVE& snapshot) {
    MotionQueryTraceReplayFixtureUVE fixture;
    fixture.truncated = snapshot.truncated ||
                        snapshot.events.size() > kMotionQueryMaximumTraceReplayEventsUVE;

    const std::size_t firstEvent = snapshot.events.size() > kMotionQueryMaximumTraceReplayEventsUVE
                                       ? snapshot.events.size() -
                                             kMotionQueryMaximumTraceReplayEventsUVE
                                       : 0U;
    fixture.events.reserve(snapshot.events.size() - firstEvent);
    for (std::size_t index = firstEvent; index < snapshot.events.size(); ++index) {
        fixture.events.push_back(BuildReplayEventUVE(snapshot.events[index]));
    }
    return fixture;
}

[[nodiscard]] MotionQueryTraceReplayComparisonUVE CompareReplayFixtureInternalUVE(
    const MotionQueryTraceReplayFixtureUVE& fixture,
    const MotionQueryTraceSnapshotUVE& snapshot,
    const std::optional<MotionQueryTraceReplayCompatibilityUVE>& expectedCompatibility) {
    if (fixture.schemaVersion != kMotionQueryTraceReplayFixtureSchemaVersionUVE) {
        return MakeComparisonUVE(MotionQueryTraceReplayComparisonCodeUVE::SchemaMismatch, 0U,
                                 kMotionQueryTraceReplayNoMismatchIndexUVE, fixture.truncated,
                                 snapshot.truncated, "motion query replay fixture schema is unsupported");
    }
    if (!IsValidReplayFixtureUVE(fixture)) {
        return MakeComparisonUVE(MotionQueryTraceReplayComparisonCodeUVE::InvalidFixture, 0U,
                                 kMotionQueryTraceReplayNoMismatchIndexUVE, fixture.truncated,
                                 snapshot.truncated, "motion query replay fixture is invalid");
    }
    if (expectedCompatibility.has_value() &&
        (!fixture.compatibility.has_value() ||
         *fixture.compatibility != *expectedCompatibility)) {
        const std::uint32_t compatibilityMismatchMask =
            ClassifyReplayCompatibilityMismatchUVE(fixture, *expectedCompatibility);
        return MakeComparisonUVE(MotionQueryTraceReplayComparisonCodeUVE::CompatibilityMismatch, 0U,
                                 kMotionQueryTraceReplayNoMismatchIndexUVE, fixture.truncated,
                                 snapshot.truncated, "motion query replay compatibility differs", 0U, {},
                                 compatibilityMismatchMask,
                                 SummarizeReplayCompatibilityMismatchUVE(compatibilityMismatchMask));
    }

    const MotionQueryTraceReplayFixtureUVE actual = expectedCompatibility.has_value()
                                                        ? BuildMotionQueryTraceReplayFixtureUVE(
                                                              snapshot, *expectedCompatibility)
                                                        : BuildMotionQueryTraceReplayFixtureUVE(snapshot);
    if (fixture.truncated != actual.truncated) {
        return MakeComparisonUVE(MotionQueryTraceReplayComparisonCodeUVE::TruncationMismatch, 0U,
                                 kMotionQueryTraceReplayNoMismatchIndexUVE, fixture.truncated,
                                 actual.truncated, "motion query replay truncation state differs");
    }
    if (fixture.events.size() != actual.events.size()) {
        return MakeComparisonUVE(MotionQueryTraceReplayComparisonCodeUVE::EventCountMismatch, 0U,
                                 kMotionQueryTraceReplayNoMismatchIndexUVE, fixture.truncated,
                                 actual.truncated, "motion query replay event count differs");
    }
    for (std::size_t index = 0U; index < fixture.events.size(); ++index) {
        if (fixture.events[index] != actual.events[index]) {
            const std::uint32_t mismatchFieldMask = ClassifyReplayEventMismatchFieldsUVE(
                fixture.events[index], actual.events[index]);
            return MakeComparisonUVE(MotionQueryTraceReplayComparisonCodeUVE::EventMismatch, index,
                                     index, fixture.truncated, actual.truncated,
                                     "motion query replay event differs", mismatchFieldMask,
                                     SummarizeReplayEventMismatchFieldsUVE(mismatchFieldMask));
        }
    }
    return MakeComparisonUVE(MotionQueryTraceReplayComparisonCodeUVE::Match, fixture.events.size(),
                             kMotionQueryTraceReplayNoMismatchIndexUVE, fixture.truncated,
                             actual.truncated, "motion query replay fixture matches trace");
}

MotionQueryTraceReplayFixtureUVE BuildMotionQueryTraceReplayFixtureUVE(
    const MotionQueryTraceSnapshotUVE& snapshot,
    const MotionQueryTraceReplayCompatibilityUVE& compatibility) {
    MotionQueryTraceReplayFixtureUVE fixture = BuildMotionQueryTraceReplayFixtureUVE(snapshot);
    fixture.compatibility = compatibility;
    return fixture;
}

MotionQueryTraceReplayComparisonUVE CompareMotionQueryTraceReplayFixtureUVE(
    const MotionQueryTraceReplayFixtureUVE& fixture,
    const MotionQueryTraceSnapshotUVE& snapshot) {
    return CompareReplayFixtureInternalUVE(fixture, snapshot, std::nullopt);
}

MotionQueryTraceReplayComparisonUVE CompareMotionQueryTraceReplayFixtureUVE(
    const MotionQueryTraceReplayFixtureUVE& fixture,
    const MotionQueryTraceSnapshotUVE& snapshot,
    const MotionQueryTraceReplayCompatibilityUVE& compatibility) {
    return CompareReplayFixtureInternalUVE(fixture, snapshot, compatibility);
}

MotionQueryTraceReplaySerializationResultUVE SerializeMotionQueryTraceReplayFixtureUVE(
    const MotionQueryTraceReplayFixtureUVE& fixture) {
    if (fixture.schemaVersion != kMotionQueryTraceReplayFixtureSchemaVersionUVE) {
        return MakeSerializationResultUVE(
            MotionQueryTraceReplaySerializationCodeUVE::SchemaMismatch, {},
            "motion query replay fixture schema is unsupported");
    }
    if (!IsValidReplayFixtureUVE(fixture)) {
        return MakeSerializationResultUVE(
            MotionQueryTraceReplaySerializationCodeUVE::InvalidFixture, {},
            "motion query replay fixture is invalid");
    }

    const JsonUVE document{
        {"schemaVersion", fixture.schemaVersion},
        {"truncated", fixture.truncated},
        {"compatibility", fixture.compatibility.has_value()
                              ? ReplayCompatibilityToJsonUVE(*fixture.compatibility)
                              : JsonUVE(nullptr)},
        {"events", [&fixture] {
             JsonUVE events = JsonUVE::array();
             for (const MotionQueryTraceReplayEventUVE& event : fixture.events) {
                 events.push_back(ReplayEventToJsonUVE(event));
             }
             return events;
         }()},
    };
    std::string payload = document.dump();
    if (payload.size() > kMotionQueryMaximumTraceReplayPayloadBytesUVE) {
        return MakeSerializationResultUVE(
            MotionQueryTraceReplaySerializationCodeUVE::PayloadTooLarge, {},
            "motion query replay fixture payload exceeds the bounded serialization limit");
    }
    return MakeSerializationResultUVE(MotionQueryTraceReplaySerializationCodeUVE::Accepted,
                                      std::move(payload), "motion query replay fixture serialized");
}

MotionQueryTraceReplayDeserializationResultUVE DeserializeMotionQueryTraceReplayFixtureUVE(
    const std::string_view payload) {
    if (payload.empty()) {
        return {MotionQueryTraceReplaySerializationCodeUVE::EmptyPayload, std::nullopt,
                "motion query replay fixture payload is empty"};
    }
    if (payload.size() > kMotionQueryMaximumTraceReplayPayloadBytesUVE) {
        return {MotionQueryTraceReplaySerializationCodeUVE::PayloadTooLarge, std::nullopt,
                "motion query replay fixture payload exceeds the bounded serialization limit"};
    }

    try {
        const JsonUVE document = JsonUVE::parse(payload);
        MotionQueryTraceReplayFixtureUVE fixture;
        fixture.schemaVersion = document.at("schemaVersion").get<std::uint32_t>();
        if (fixture.schemaVersion != kMotionQueryTraceReplayFixtureSchemaVersionUVE) {
            return {MotionQueryTraceReplaySerializationCodeUVE::SchemaMismatch, std::nullopt,
                    "motion query replay fixture schema is unsupported"};
        }
        if (!HasOnlyKeysUVE(document, {"schemaVersion", "truncated", "compatibility", "events"})) {
            return {MotionQueryTraceReplaySerializationCodeUVE::UnexpectedField, std::nullopt,
                    "motion query replay fixture contains an unexpected field"};
        }
        fixture.truncated = document.at("truncated").get<bool>();
        if (document.contains("compatibility") && !document.at("compatibility").is_null()) {
            if (!document.at("compatibility").is_object() ||
                !HasOnlyKeysUVE(document.at("compatibility"), {"schemaVersion", "samplerVersion",
                                                                 "normalizationVersion",
                                                                 "sourceGeneration"})) {
                return {MotionQueryTraceReplaySerializationCodeUVE::UnexpectedField, std::nullopt,
                        "motion query replay compatibility contains an unexpected field"};
            }
            fixture.compatibility = ReplayCompatibilityFromJsonUVE(document.at("compatibility"));
        }
        const JsonUVE& events = document.at("events");
        if (!events.is_array() || events.size() > kMotionQueryMaximumTraceReplayEventsUVE) {
            return {MotionQueryTraceReplaySerializationCodeUVE::InvalidFixture, std::nullopt,
                    "motion query replay fixture event retention is invalid"};
        }
        fixture.events.reserve(events.size());
        for (const JsonUVE& event : events) {
            if (!event.is_object()) {
                return {MotionQueryTraceReplaySerializationCodeUVE::InvalidFixture, std::nullopt,
                        "motion query replay event is not an object"};
            }
            if (HasAnyKeyUVE(event, kForbiddenReplayFieldNamesUVE)) {
                return {MotionQueryTraceReplaySerializationCodeUVE::ForbiddenField, std::nullopt,
                        "motion query replay event contains a forbidden runtime field"};
            }
            if (!HasOnlyKeysUVE(event, {"sequence", "frameNumber", "kind", "candidatesConsidered",
                                        "candidatesEvaluated", "cost", "selectedCandidateIndex",
                                        "qualityTier", "continuityCode", "continuityApplied",
                                        "transitionCode", "transitionHeldPrevious", "telemetryCode",
                                        "telemetryIndexEntryCount", "telemetryCandidatesConsidered",
                                        "telemetryBudgetSaturated", "provenance"})) {
                return {MotionQueryTraceReplaySerializationCodeUVE::UnexpectedField, std::nullopt,
                        "motion query replay event contains an unexpected field"};
            }
            fixture.events.push_back(ReplayEventFromJsonUVE(event));
        }
        if (!IsValidReplayFixtureUVE(fixture)) {
            return {MotionQueryTraceReplaySerializationCodeUVE::InvalidFixture, std::nullopt,
                    "motion query replay fixture event payload is invalid"};
        }
        return {MotionQueryTraceReplaySerializationCodeUVE::Accepted, std::move(fixture),
                "motion query replay fixture deserialized"};
    } catch (const nlohmann::json::exception&) {
        return {MotionQueryTraceReplaySerializationCodeUVE::ParseError, std::nullopt,
                "motion query replay fixture payload could not be parsed"};
    }
}

} // namespace UVE::Plugins::Editor
