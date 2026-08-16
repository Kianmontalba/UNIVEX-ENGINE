// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/plugins/motion_query_live_debug_session_uve.h"
#include "uve/plugins/motion_query_trace_replay_uve.h"

#include <cstddef>
#include <optional>
#include <string>

namespace UVE::Plugins::Editor {

enum class MotionQueryTraceReplayRegressionCodeUVE : std::uint8_t {
    Match = 0,
    Mismatch,
    EmptyTrace,
    FilteredSnapshot,
};

struct MotionQueryTraceReplayRegressionResultUVE final {
    MotionQueryTraceReplayRegressionCodeUVE code =
        MotionQueryTraceReplayRegressionCodeUVE::Mismatch;
    std::optional<MotionQueryTraceReplayComparisonUVE> comparison;
    std::string message;

    [[nodiscard]] bool IsMatchUVE() const noexcept {
        return code == MotionQueryTraceReplayRegressionCodeUVE::Match;
    }
};

[[nodiscard]] MotionQueryTraceReplayRegressionResultUVE
CompareMotionQueryLiveDebugSnapshotAgainstFixtureUVE(
    const MotionQueryTraceReplayFixtureUVE& fixture,
    const MotionQueryLiveDebugSnapshotUVE& snapshot);

[[nodiscard]] MotionQueryTraceReplayRegressionResultUVE
CompareMotionQueryLiveDebugSnapshotAgainstFixtureUVE(
    const MotionQueryTraceReplayFixtureUVE& fixture,
    const MotionQueryLiveDebugSnapshotUVE& snapshot,
    const MotionQueryTraceReplayCompatibilityUVE& compatibility);

} // namespace UVE::Plugins::Editor
