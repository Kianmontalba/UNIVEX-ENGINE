// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_trace_replay_regression_uve.h"

#include <algorithm>

namespace UVE::Plugins::Editor {

MotionQueryTraceReplayCaptureResultUVE CaptureMotionQueryTraceReplayFixtureUVE(
    const MotionQueryLiveDebugSnapshotUVE& snapshot) {
    if (!snapshot.filter.empty() ||
        snapshot.totalTraceEventCount != snapshot.visibleTraceEventCount) {
        return {MotionQueryTraceReplayCaptureCodeUVE::FilteredSnapshot, std::nullopt,
                "motion query live debug snapshot is filtered and cannot be captured as a full fixture"};
    }
    if (snapshot.visibleTraceEventCount == 0U) {
        return {MotionQueryTraceReplayCaptureCodeUVE::EmptyTrace, std::nullopt,
                "motion query live debug snapshot contains no trace events"};
    }

    MotionQueryTraceSnapshotUVE trace;
    trace.truncated = snapshot.traceTruncated;
    trace.events = snapshot.traceEvents;
    return {MotionQueryTraceReplayCaptureCodeUVE::Accepted,
            BuildMotionQueryTraceReplayFixtureUVE(trace),
            "motion query replay fixture captured from copied live debug trace"};
}

namespace {

[[nodiscard]] MotionQueryTraceReplayRegressionResultUVE CompareSnapshotInternalUVE(
    const MotionQueryTraceReplayFixtureUVE& fixture,
    const MotionQueryLiveDebugSnapshotUVE& snapshot,
    const std::optional<MotionQueryTraceReplayCompatibilityUVE>& compatibility) {
    if (!snapshot.filter.empty() ||
        snapshot.totalTraceEventCount != snapshot.visibleTraceEventCount) {
        return {MotionQueryTraceReplayRegressionCodeUVE::FilteredSnapshot, std::nullopt,
                "motion query live debug snapshot is filtered and cannot prove full replay equivalence"};
    }
    if (snapshot.visibleTraceEventCount == 0U) {
        return {MotionQueryTraceReplayRegressionCodeUVE::EmptyTrace, std::nullopt,
                "motion query live debug snapshot contains no trace events"};
    }

    MotionQueryTraceSnapshotUVE trace;
    trace.truncated = snapshot.traceTruncated;
    trace.events = snapshot.traceEvents;
    const MotionQueryTraceReplayComparisonUVE comparison = compatibility.has_value()
                                                               ? CompareMotionQueryTraceReplayFixtureUVE(
                                                                     fixture, trace, *compatibility)
                                                               : CompareMotionQueryTraceReplayFixtureUVE(
                                                                     fixture, trace);
    return {comparison.IsMatchUVE() ? MotionQueryTraceReplayRegressionCodeUVE::Match
                                    : MotionQueryTraceReplayRegressionCodeUVE::Mismatch,
            comparison, comparison.message};
}

} // namespace

MotionQueryTraceReplayBaselineBatchResultUVE
CompareMotionQueryLiveDebugSnapshotAgainstAllBaselinesUVE(
    const MotionQueryTraceReplayBaselineRegistryUVE& registry,
    const MotionQueryLiveDebugSnapshotUVE& snapshot,
    const std::optional<std::uint64_t> expectedRegistryGeneration,
    const std::optional<MotionQueryTraceReplayCompatibilityUVE> expectedCompatibility) {
    const MotionQueryTraceReplayBaselineSnapshotUVE registrySnapshot = registry.GetSnapshotUVE();
    MotionQueryTraceReplayBaselineBatchResultUVE batch;
    batch.registryGeneration = registrySnapshot.generation;
    batch.results.reserve(std::min(registrySnapshot.entries.size(),
                                   kMotionQueryMaximumReplayRegressionBatchResultsUVE));
    if (expectedRegistryGeneration.has_value() &&
        expectedRegistryGeneration.value() != registrySnapshot.generation) {
        batch.code = MotionQueryTraceReplayBaselineBatchCodeUVE::StaleGeneration;
        batch.message = "motion query replay baseline batch used a stale registry generation";
        return batch;
    }
    if (registrySnapshot.entries.empty()) {
        batch.code = MotionQueryTraceReplayBaselineBatchCodeUVE::EmptyRegistry;
        batch.message = "motion query replay baseline registry contains no baselines";
        return batch;
    }
    if (!snapshot.filter.empty() || snapshot.totalTraceEventCount != snapshot.visibleTraceEventCount) {
        batch.code = MotionQueryTraceReplayBaselineBatchCodeUVE::FilteredSnapshot;
    } else if (snapshot.visibleTraceEventCount == 0U) {
        batch.code = MotionQueryTraceReplayBaselineBatchCodeUVE::EmptyTrace;
    } else {
        batch.code = MotionQueryTraceReplayBaselineBatchCodeUVE::Accepted;
    }

    for (const MotionQueryTraceReplayBaselineEntryUVE& entry : registrySnapshot.entries) {
        if (batch.results.size() >= kMotionQueryMaximumReplayRegressionBatchResultsUVE) {
            batch.truncated = true;
            break;
        }
        const MotionQueryTraceReplayBaselineRegressionResultUVE result =
            CompareMotionQueryLiveDebugSnapshotAgainstNamedBaselineUVE(
                registry, entry.name, snapshot, registrySnapshot.generation, expectedCompatibility);
        batch.results.push_back(result);
        ++batch.evaluatedBaselineCount;
        if (result.IsMatchUVE()) {
            ++batch.matchCount;
        } else {
            ++batch.mismatchCount;
        }
    }
    if (batch.code == MotionQueryTraceReplayBaselineBatchCodeUVE::Accepted) {
        batch.message = "motion query replay baseline batch comparison completed";
    } else if (batch.code == MotionQueryTraceReplayBaselineBatchCodeUVE::FilteredSnapshot) {
        batch.message = "motion query replay baseline batch received filtered evidence";
    } else if (batch.code == MotionQueryTraceReplayBaselineBatchCodeUVE::EmptyTrace) {
        batch.message = "motion query replay baseline batch received empty evidence";
    }
    return batch;
}

MotionQueryTraceReplayBaselineRegressionResultUVE
CompareMotionQueryLiveDebugSnapshotAgainstNamedBaselineUVE(
    const MotionQueryTraceReplayBaselineRegistryUVE& registry,
    const std::string_view baselineName,
    const MotionQueryLiveDebugSnapshotUVE& snapshot,
    const std::optional<std::uint64_t> expectedRegistryGeneration,
    const std::optional<MotionQueryTraceReplayCompatibilityUVE> expectedCompatibility) {
    const MotionQueryTraceReplayBaselineSelectionUVE selection =
        registry.SelectUVE(baselineName, expectedRegistryGeneration);
    if (selection.code == MotionQueryTraceReplayBaselineCodeUVE::StaleGeneration) {
        return {MotionQueryTraceReplayBaselineRegressionCodeUVE::StaleGeneration,
                selection.registryGeneration, std::string(baselineName), std::nullopt, selection.message};
    }
    if (!selection.IsAcceptedUVE()) {
        return {MotionQueryTraceReplayBaselineRegressionCodeUVE::BaselineNotFound,
                selection.registryGeneration, std::string(baselineName), std::nullopt, selection.message};
    }
    if (!snapshot.filter.empty() ||
        snapshot.totalTraceEventCount != snapshot.visibleTraceEventCount) {
        return {MotionQueryTraceReplayBaselineRegressionCodeUVE::FilteredSnapshot,
                selection.registryGeneration, std::string(baselineName), std::nullopt,
                "motion query live debug snapshot is filtered and cannot prove named baseline equivalence"};
    }
    if (snapshot.visibleTraceEventCount == 0U) {
        return {MotionQueryTraceReplayBaselineRegressionCodeUVE::EmptyTrace,
                selection.registryGeneration, std::string(baselineName), std::nullopt,
                "motion query live debug snapshot contains no trace events"};
    }

    MotionQueryTraceSnapshotUVE trace;
    trace.truncated = snapshot.traceTruncated;
    trace.events = snapshot.traceEvents;
    const MotionQueryTraceReplayComparisonUVE comparison = expectedCompatibility.has_value()
                                                                 ? CompareMotionQueryTraceReplayFixtureUVE(
                                                                       *selection.fixture, trace,
                                                                       *expectedCompatibility)
                                                                 : CompareMotionQueryTraceReplayFixtureUVE(
                                                                       *selection.fixture, trace);
    return {comparison.IsMatchUVE() ? MotionQueryTraceReplayBaselineRegressionCodeUVE::Match
                                    : MotionQueryTraceReplayBaselineRegressionCodeUVE::Mismatch,
            selection.registryGeneration, std::string(baselineName), comparison, comparison.message};
}

MotionQueryTraceReplayRegressionResultUVE
CompareMotionQueryLiveDebugSnapshotAgainstFixtureUVE(
    const MotionQueryTraceReplayFixtureUVE& fixture,
    const MotionQueryLiveDebugSnapshotUVE& snapshot) {
    return CompareSnapshotInternalUVE(fixture, snapshot, std::nullopt);
}

MotionQueryTraceReplayRegressionResultUVE
CompareMotionQueryLiveDebugSnapshotAgainstFixtureUVE(
    const MotionQueryTraceReplayFixtureUVE& fixture,
    const MotionQueryLiveDebugSnapshotUVE& snapshot,
    const MotionQueryTraceReplayCompatibilityUVE& compatibility) {
    return CompareSnapshotInternalUVE(fixture, snapshot, compatibility);
}

} // namespace UVE::Plugins::Editor
