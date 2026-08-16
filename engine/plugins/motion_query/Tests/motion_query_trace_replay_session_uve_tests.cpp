// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_trace_replay_session_uve.h"

#include <gtest/gtest.h>

namespace UVE::Plugins::Editor {
namespace {

MotionQueryTraceSnapshotUVE MakeSnapshotUVE() {
    MotionQueryTraceSnapshotUVE snapshot;
    MotionQueryTraceEventUVE event;
    event.sequence = 1U;
    event.timestampNanoseconds = 123456U;
    event.frameNumber = 7U;
    event.kind = "accepted";
    event.candidatesConsidered = 4U;
    event.candidatesEvaluated = 2U;
    event.cost = 0.25F;
    event.selectedCandidateIndex = 1U;
    event.qualityTier = 2U;
    event.telemetryCode = 0U;
    event.telemetryIndexEntryCount = 16U;
    event.telemetryCandidatesConsidered = 2U;
    event.provenance = "runtime";
    snapshot.events.push_back(event);
    return snapshot;
}

MotionQueryTraceReplayFixtureUVE MakeFixtureUVE() {
    return BuildMotionQueryTraceReplayFixtureUVE(MakeSnapshotUVE());
}

MotionQueryTraceReplayCompatibilityUVE MakeCompatibilityUVE() {
    return MotionQueryTraceReplayCompatibilityUVE{3U, 4U, 5U, 9U};
}

} // namespace

TEST(MotionQueryTraceReplaySessionUVETest, CompareUVE_RejectsEmptySessionWithoutChangingGeneration) {
    MotionQueryTraceReplaySessionUVE session;
    const MotionQueryTraceReplaySessionSnapshotUVE initial = session.GetSnapshotUVE();
    const MotionQueryTraceReplaySessionResultUVE result = session.CompareUVE(MakeSnapshotUVE());

    EXPECT_EQ(result.code, MotionQueryTraceReplaySessionCodeUVE::EmptySession);
    EXPECT_FALSE(result.applied);
    EXPECT_EQ(result.generation, initial.generation);
    EXPECT_EQ(session.GetSnapshotUVE(), initial);
}

TEST(MotionQueryTraceReplaySessionUVETest, LoadUVE_CopiesValidatedFixtureAndAdvancesGeneration) {
    MotionQueryTraceReplaySessionUVE session;
    const MotionQueryTraceReplayFixtureUVE fixture = MakeFixtureUVE();
    const MotionQueryTraceReplaySessionResultUVE loaded = session.LoadFixtureUVE(fixture);

    ASSERT_TRUE(loaded.IsAppliedUVE());
    EXPECT_EQ(loaded.code, MotionQueryTraceReplaySessionCodeUVE::Applied);
    EXPECT_EQ(loaded.generation, 1U);
    const MotionQueryTraceReplaySessionSnapshotUVE snapshot = session.GetSnapshotUVE();
    EXPECT_TRUE(snapshot.hasFixture);
    EXPECT_EQ(snapshot.schemaVersion, kMotionQueryTraceReplayFixtureSchemaVersionUVE);
    EXPECT_EQ(snapshot.eventCount, 1U);
    EXPECT_FALSE(snapshot.truncated);

    MotionQueryTraceReplayFixtureUVE changed = fixture;
    changed.events.front().kind = "changed-after-load";
    EXPECT_EQ(session.GetSnapshotUVE().eventCount, 1U);
    EXPECT_EQ(session.CompareUVE(MakeSnapshotUVE()).code,
              MotionQueryTraceReplaySessionCodeUVE::Match);
}

TEST(MotionQueryTraceReplaySessionUVETest, LoadSerializedUVE_UsesCodecAndPreservesGenerationOnReject) {
    MotionQueryTraceReplaySessionUVE session;
    const MotionQueryTraceReplaySerializationResultUVE encoded =
        SerializeMotionQueryTraceReplayFixtureUVE(MakeFixtureUVE());
    ASSERT_TRUE(encoded.IsAcceptedUVE());

    const MotionQueryTraceReplaySessionResultUVE loaded = session.LoadSerializedUVE(encoded.payload);
    ASSERT_TRUE(loaded.IsAppliedUVE());
    ASSERT_EQ(loaded.generation, 1U);

    const MotionQueryTraceReplaySessionResultUVE rejected = session.LoadSerializedUVE("not-json");
    EXPECT_EQ(rejected.code, MotionQueryTraceReplaySessionCodeUVE::SerializationFailure);
    EXPECT_EQ(rejected.generation, 1U);
    EXPECT_EQ(session.GetSnapshotUVE().generation, 1U);
}

TEST(MotionQueryTraceReplaySessionUVETest, CompareUVE_ReturnsMatchAndMismatchWithSessionGeneration) {
    MotionQueryTraceReplaySessionUVE session;
    ASSERT_TRUE(session.LoadFixtureUVE(MakeFixtureUVE()).IsAppliedUVE());

    const MotionQueryTraceReplaySessionResultUVE match = session.CompareUVE(MakeSnapshotUVE());
    EXPECT_TRUE(match.IsMatchUVE());
    ASSERT_TRUE(match.comparison.has_value());
    EXPECT_EQ(match.comparison->comparedEventCount, 1U);
    EXPECT_EQ(match.generation, 1U);

    MotionQueryTraceSnapshotUVE changed = MakeSnapshotUVE();
    changed.events.front().telemetryCandidatesConsidered = 1U;
    const MotionQueryTraceReplaySessionResultUVE mismatch = session.CompareUVE(changed);
    EXPECT_EQ(mismatch.code, MotionQueryTraceReplaySessionCodeUVE::Mismatch);
    EXPECT_FALSE(mismatch.IsMatchUVE());
    ASSERT_TRUE(mismatch.comparison.has_value());
    EXPECT_EQ(mismatch.comparison->mismatchIndex, 0U);
    EXPECT_EQ(mismatch.generation, 1U);
}

TEST(MotionQueryTraceReplaySessionUVETest, CompareUVE_EnforcesCompatibilityAtSessionBoundary) {
    MotionQueryTraceReplaySessionUVE session;
    ASSERT_TRUE(session.LoadFixtureUVE(
                             BuildMotionQueryTraceReplayFixtureUVE(MakeSnapshotUVE(), MakeCompatibilityUVE()))
                    .IsAppliedUVE());

    EXPECT_EQ(session.CompareUVE(MakeSnapshotUVE(), MakeCompatibilityUVE()).code,
              MotionQueryTraceReplaySessionCodeUVE::Match);
    const MotionQueryTraceReplayCompatibilityUVE changed{3U, 4U, 6U, 9U};
    const MotionQueryTraceReplaySessionResultUVE mismatch =
        session.CompareUVE(MakeSnapshotUVE(), changed);
    EXPECT_EQ(mismatch.code, MotionQueryTraceReplaySessionCodeUVE::Mismatch);
    ASSERT_TRUE(mismatch.comparison.has_value());
    EXPECT_EQ(mismatch.comparison->code,
              MotionQueryTraceReplayComparisonCodeUVE::CompatibilityMismatch);
}

TEST(MotionQueryTraceReplaySessionUVETest, ClearUVE_RemovesFixtureAndAdvancesGeneration) {
    MotionQueryTraceReplaySessionUVE session;
    ASSERT_TRUE(session.LoadFixtureUVE(MakeFixtureUVE()).IsAppliedUVE());
    session.ClearUVE();

    const MotionQueryTraceReplaySessionSnapshotUVE snapshot = session.GetSnapshotUVE();
    EXPECT_EQ(snapshot.generation, 2U);
    EXPECT_FALSE(snapshot.hasFixture);
    EXPECT_EQ(snapshot.eventCount, 0U);
    EXPECT_EQ(session.CompareUVE(MakeSnapshotUVE()).code,
              MotionQueryTraceReplaySessionCodeUVE::EmptySession);
}

TEST(MotionQueryTraceReplaySessionUVETest, LoadUVE_RejectsSchemaAndInvalidFixtureWithoutMutation) {
    MotionQueryTraceReplaySessionUVE session;
    ASSERT_TRUE(session.LoadFixtureUVE(MakeFixtureUVE()).IsAppliedUVE());

    MotionQueryTraceReplayFixtureUVE unsupported = MakeFixtureUVE();
    unsupported.schemaVersion = kMotionQueryTraceReplayFixtureSchemaVersionUVE + 1U;
    const MotionQueryTraceReplaySessionResultUVE schemaRejected = session.LoadFixtureUVE(unsupported);
    EXPECT_EQ(schemaRejected.code, MotionQueryTraceReplaySessionCodeUVE::SchemaMismatch);
    EXPECT_EQ(schemaRejected.generation, 1U);

    MotionQueryTraceReplayFixtureUVE invalid = MakeFixtureUVE();
    invalid.events.front().sequence = 0U;
    const MotionQueryTraceReplaySessionResultUVE invalidRejected = session.LoadFixtureUVE(invalid);
    EXPECT_EQ(invalidRejected.code, MotionQueryTraceReplaySessionCodeUVE::InvalidFixture);
    EXPECT_EQ(invalidRejected.generation, 1U);
    EXPECT_TRUE(session.GetSnapshotUVE().hasFixture);
    EXPECT_EQ(session.GetSnapshotUVE().eventCount, 1U);
}

} // namespace UVE::Plugins::Editor
