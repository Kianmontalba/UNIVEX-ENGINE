// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/plugins/motion_query_live_debug_session_uve.h"
#include "uve/plugins/motion_query_trace_replay_baseline_registry_uve.h"
#include "uve/plugins/motion_query_trace_replay_uve.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace UVE::Plugins::Editor {

enum class MotionQueryTraceReplayCaptureCodeUVE : std::uint8_t {
    Accepted = 0,
    EmptyTrace,
    FilteredSnapshot,
};

struct MotionQueryTraceReplayCaptureResultUVE final {
    MotionQueryTraceReplayCaptureCodeUVE code = MotionQueryTraceReplayCaptureCodeUVE::EmptyTrace;
    std::optional<MotionQueryTraceReplayFixtureUVE> fixture;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == MotionQueryTraceReplayCaptureCodeUVE::Accepted && fixture.has_value();
    }
};

[[nodiscard]] MotionQueryTraceReplayCaptureResultUVE CaptureMotionQueryTraceReplayFixtureUVE(
    const MotionQueryLiveDebugSnapshotUVE& snapshot);

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

enum class MotionQueryTraceReplayBaselineRegressionCodeUVE : std::uint8_t {
    Match = 0,
    Mismatch,
    BaselineNotFound,
    StaleGeneration,
    EmptyTrace,
    FilteredSnapshot,
};

struct MotionQueryTraceReplayBaselineRegressionResultUVE final {
    MotionQueryTraceReplayBaselineRegressionCodeUVE code =
        MotionQueryTraceReplayBaselineRegressionCodeUVE::BaselineNotFound;
    std::uint64_t registryGeneration = 0U;
    std::string baselineName;
    std::optional<MotionQueryTraceReplayComparisonUVE> comparison;
    std::string message;

    [[nodiscard]] bool IsMatchUVE() const noexcept {
        return code == MotionQueryTraceReplayBaselineRegressionCodeUVE::Match;
    }
};

[[nodiscard]] MotionQueryTraceReplayBaselineRegressionResultUVE
CompareMotionQueryLiveDebugSnapshotAgainstNamedBaselineUVE(
    const MotionQueryTraceReplayBaselineRegistryUVE& registry,
    std::string_view baselineName,
    const MotionQueryLiveDebugSnapshotUVE& snapshot,
    std::optional<std::uint64_t> expectedRegistryGeneration = std::nullopt,
    std::optional<MotionQueryTraceReplayCompatibilityUVE> expectedCompatibility = std::nullopt);

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
