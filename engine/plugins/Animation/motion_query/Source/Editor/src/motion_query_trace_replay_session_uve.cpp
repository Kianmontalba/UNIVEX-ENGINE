// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_trace_replay_session_uve.h"

#include <utility>

namespace UVE::Plugins::Editor {

MotionQueryTraceReplaySessionResultUVE MotionQueryTraceReplaySessionUVE::LoadFixtureUVE(
    const MotionQueryTraceReplayFixtureUVE& fixture) {
    const MotionQueryTraceReplaySerializationResultUVE validation =
        SerializeMotionQueryTraceReplayFixtureUVE(fixture);
    if (validation.code == MotionQueryTraceReplaySerializationCodeUVE::SchemaMismatch) {
        return MakeResultUVE(MotionQueryTraceReplaySessionCodeUVE::SchemaMismatch, false,
                             validation.message);
    }
    if (!validation.IsAcceptedUVE()) {
        return MakeResultUVE(MotionQueryTraceReplaySessionCodeUVE::InvalidFixture, false,
                             validation.message);
    }

    fixture_ = fixture;
    ++generation_;
    return MakeResultUVE(MotionQueryTraceReplaySessionCodeUVE::Applied, true,
                         "motion query replay fixture loaded");
}

MotionQueryTraceReplaySessionResultUVE MotionQueryTraceReplaySessionUVE::LoadSerializedUVE(
    const std::string_view payload) {
    const MotionQueryTraceReplayDeserializationResultUVE decoded =
        DeserializeMotionQueryTraceReplayFixtureUVE(payload);
    if (decoded.code == MotionQueryTraceReplaySerializationCodeUVE::SchemaMismatch) {
        return MakeResultUVE(MotionQueryTraceReplaySessionCodeUVE::SchemaMismatch, false,
                             decoded.message);
    }
    if (!decoded.IsAcceptedUVE()) {
        return MakeResultUVE(MotionQueryTraceReplaySessionCodeUVE::SerializationFailure, false,
                             decoded.message);
    }
    return LoadFixtureUVE(*decoded.fixture);
}

MotionQueryTraceReplaySessionResultUVE MotionQueryTraceReplaySessionUVE::CompareUVE(
    const MotionQueryTraceSnapshotUVE& snapshot) const {
    if (!fixture_.has_value()) {
        return MakeResultUVE(MotionQueryTraceReplaySessionCodeUVE::EmptySession, false,
                             "motion query replay session has no fixture");
    }

    const MotionQueryTraceReplayComparisonUVE comparison =
        CompareMotionQueryTraceReplayFixtureUVE(*fixture_, snapshot);
    MotionQueryTraceReplaySessionResultUVE result;
    result.code = comparison.IsMatchUVE() ? MotionQueryTraceReplaySessionCodeUVE::Match
                                          : MotionQueryTraceReplaySessionCodeUVE::Mismatch;
    result.generation = generation_;
    result.comparison = comparison;
    result.message = comparison.message;
    return result;
}

MotionQueryTraceReplaySessionResultUVE MotionQueryTraceReplaySessionUVE::CompareUVE(
    const MotionQueryTraceSnapshotUVE& snapshot,
    const MotionQueryTraceReplayCompatibilityUVE& compatibility) const {
    if (!fixture_.has_value()) {
        return MakeResultUVE(MotionQueryTraceReplaySessionCodeUVE::EmptySession, false,
                             "motion query replay session has no fixture");
    }

    const MotionQueryTraceReplayComparisonUVE comparison =
        CompareMotionQueryTraceReplayFixtureUVE(*fixture_, snapshot, compatibility);
    MotionQueryTraceReplaySessionResultUVE result;
    result.code = comparison.IsMatchUVE() ? MotionQueryTraceReplaySessionCodeUVE::Match
                                          : MotionQueryTraceReplaySessionCodeUVE::Mismatch;
    result.generation = generation_;
    result.comparison = comparison;
    result.message = comparison.message;
    return result;
}

void MotionQueryTraceReplaySessionUVE::ClearUVE() noexcept {
    fixture_.reset();
    ++generation_;
}

MotionQueryTraceReplaySessionSnapshotUVE
MotionQueryTraceReplaySessionUVE::GetSnapshotUVE() const noexcept {
    MotionQueryTraceReplaySessionSnapshotUVE snapshot;
    snapshot.generation = generation_;
    if (fixture_.has_value()) {
        snapshot.hasFixture = true;
        snapshot.schemaVersion = fixture_->schemaVersion;
        snapshot.eventCount = fixture_->events.size();
        snapshot.truncated = fixture_->truncated;
    }
    return snapshot;
}

MotionQueryTraceReplaySessionResultUVE MotionQueryTraceReplaySessionUVE::MakeResultUVE(
    const MotionQueryTraceReplaySessionCodeUVE code, const bool applied,
    std::string message) const {
    return MotionQueryTraceReplaySessionResultUVE{code, generation_, applied, std::nullopt,
                                                  std::move(message)};
}

} // namespace UVE::Plugins::Editor
