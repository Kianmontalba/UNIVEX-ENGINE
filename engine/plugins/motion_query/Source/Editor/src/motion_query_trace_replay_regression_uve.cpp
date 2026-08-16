// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_trace_replay_regression_uve.h"

namespace UVE::Plugins::Editor {
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
