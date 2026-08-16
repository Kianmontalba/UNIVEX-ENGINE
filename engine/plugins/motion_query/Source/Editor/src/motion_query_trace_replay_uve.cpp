// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_trace_replay_uve.h"

#include <cmath>
#include <string_view>

namespace UVE::Plugins::Editor {
namespace {

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

[[nodiscard]] MotionQueryTraceReplayComparisonUVE MakeComparisonUVE(
    const MotionQueryTraceReplayComparisonCodeUVE code,
    const std::size_t comparedEventCount,
    const std::size_t mismatchIndex,
    const bool fixtureTruncated,
    const bool snapshotTruncated,
    const std::string_view message) {
    return MotionQueryTraceReplayComparisonUVE{
        code,
        comparedEventCount,
        mismatchIndex,
        fixtureTruncated,
        snapshotTruncated,
        std::string(message),
    };
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

MotionQueryTraceReplayComparisonUVE CompareMotionQueryTraceReplayFixtureUVE(
    const MotionQueryTraceReplayFixtureUVE& fixture,
    const MotionQueryTraceSnapshotUVE& snapshot) {
    if (fixture.schemaVersion != kMotionQueryTraceReplayFixtureSchemaVersionUVE) {
        return MakeComparisonUVE(MotionQueryTraceReplayComparisonCodeUVE::SchemaMismatch, 0U,
                                 kMotionQueryTraceReplayNoMismatchIndexUVE, fixture.truncated,
                                 snapshot.truncated, "motion query replay fixture schema is unsupported");
    }
    if (fixture.events.size() > kMotionQueryMaximumTraceReplayEventsUVE ||
        !HasStrictlyIncreasingSequencesUVE(fixture.events) ||
        !HasNonDecreasingFramesUVE(fixture.events)) {
        return MakeComparisonUVE(MotionQueryTraceReplayComparisonCodeUVE::InvalidFixture, 0U,
                                 kMotionQueryTraceReplayNoMismatchIndexUVE, fixture.truncated,
                                 snapshot.truncated, "motion query replay fixture event retention is invalid");
    }
    for (const MotionQueryTraceReplayEventUVE& event : fixture.events) {
        if (!IsValidReplayEventUVE(event)) {
            return MakeComparisonUVE(MotionQueryTraceReplayComparisonCodeUVE::InvalidFixture, 0U,
                                     kMotionQueryTraceReplayNoMismatchIndexUVE, fixture.truncated,
                                     snapshot.truncated, "motion query replay fixture event is invalid");
        }
    }

    const MotionQueryTraceReplayFixtureUVE actual =
        BuildMotionQueryTraceReplayFixtureUVE(snapshot);
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
            return MakeComparisonUVE(MotionQueryTraceReplayComparisonCodeUVE::EventMismatch, index,
                                     index, fixture.truncated, actual.truncated,
                                     "motion query replay event differs");
        }
    }
    return MakeComparisonUVE(MotionQueryTraceReplayComparisonCodeUVE::Match, fixture.events.size(),
                             kMotionQueryTraceReplayNoMismatchIndexUVE, fixture.truncated,
                             actual.truncated, "motion query replay fixture matches trace");
}

} // namespace UVE::Plugins::Editor
