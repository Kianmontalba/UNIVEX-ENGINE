// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/plugins/motion_query_trace_replay_uve.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace UVE::Plugins::Editor {

enum class MotionQueryTraceReplaySessionCodeUVE : std::uint8_t {
    Applied = 0,
    EmptySession,
    InvalidFixture,
    SerializationFailure,
    SchemaMismatch,
    Match,
    Mismatch,
};

struct MotionQueryTraceReplaySessionSnapshotUVE final {
    std::uint64_t generation = 0U;
    bool hasFixture = false;
    std::uint32_t schemaVersion = 0U;
    std::size_t eventCount = 0U;
    bool truncated = false;

    [[nodiscard]] bool operator==(const MotionQueryTraceReplaySessionSnapshotUVE&) const = default;
};

struct MotionQueryTraceReplaySessionResultUVE final {
    MotionQueryTraceReplaySessionCodeUVE code =
        MotionQueryTraceReplaySessionCodeUVE::InvalidFixture;
    std::uint64_t generation = 0U;
    bool applied = false;
    std::optional<MotionQueryTraceReplayComparisonUVE> comparison;
    std::string message;

    [[nodiscard]] bool IsAppliedUVE() const noexcept {
        return applied && code == MotionQueryTraceReplaySessionCodeUVE::Applied;
    }

    [[nodiscard]] bool IsMatchUVE() const noexcept {
        return code == MotionQueryTraceReplaySessionCodeUVE::Match;
    }
};

class MotionQueryTraceReplaySessionUVE final {
public:
    [[nodiscard]] MotionQueryTraceReplaySessionResultUVE LoadFixtureUVE(
        const MotionQueryTraceReplayFixtureUVE& fixture);
    [[nodiscard]] MotionQueryTraceReplaySessionResultUVE LoadSerializedUVE(
        std::string_view payload);
    [[nodiscard]] MotionQueryTraceReplaySessionResultUVE CompareUVE(
        const MotionQueryTraceSnapshotUVE& snapshot) const;
    [[nodiscard]] MotionQueryTraceReplaySessionResultUVE CompareUVE(
        const MotionQueryTraceSnapshotUVE& snapshot,
        const MotionQueryTraceReplayCompatibilityUVE& compatibility) const;
    void ClearUVE() noexcept;

    [[nodiscard]] MotionQueryTraceReplaySessionSnapshotUVE GetSnapshotUVE() const noexcept;

private:
    [[nodiscard]] MotionQueryTraceReplaySessionResultUVE MakeResultUVE(
        MotionQueryTraceReplaySessionCodeUVE code, bool applied, std::string message) const;

    std::optional<MotionQueryTraceReplayFixtureUVE> fixture_;
    std::uint64_t generation_ = 0U;
};

} // namespace UVE::Plugins::Editor
