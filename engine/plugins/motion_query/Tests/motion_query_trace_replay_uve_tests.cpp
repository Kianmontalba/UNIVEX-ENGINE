// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_trace_replay_uve.h"

#include <gtest/gtest.h>

namespace UVE::Plugins::Editor {
namespace {

MotionQueryTraceSnapshotUVE MakeSnapshotUVE() {
    MotionQueryTraceSnapshotUVE snapshot;
    MotionQueryTraceEventUVE event;
    event.sequence = 1U;
    event.timestampNanoseconds = 9000000000U;
    event.frameNumber = 42U;
    event.kind = "accepted";
    event.database = UVE::Asset::ResourceHandleUVE{UVE::Asset::AssetGuidUVE{77U}, 3U};
    event.candidatesConsidered = 8U;
    event.candidatesEvaluated = 4U;
    event.cost = 0.375F;
    event.selectedCandidateIndex = 2U;
    event.qualityTier = 1U;
    event.continuityCode = 1U;
    event.continuityApplied = true;
    event.transitionCode = 2U;
    event.transitionHeldPrevious = false;
    event.telemetryCode = 0U;
    event.telemetryIndexEntryCount = 64U;
    event.telemetryCandidatesConsidered = 4U;
    event.telemetryBudgetSaturated = true;
    event.provenance = "continuity_applied";
    event.message = "machine-specific diagnostic text";
    snapshot.events.push_back(event);
    return snapshot;
}

} // namespace

TEST(MotionQueryTraceReplayUVETest, BuildUVE_CopiesDeterministicFieldsAndRedactsRuntimeIdentity) {
    const MotionQueryTraceReplayFixtureUVE fixture =
        BuildMotionQueryTraceReplayFixtureUVE(MakeSnapshotUVE());

    ASSERT_EQ(fixture.schemaVersion, kMotionQueryTraceReplayFixtureSchemaVersionUVE);
    ASSERT_FALSE(fixture.truncated);
    ASSERT_EQ(fixture.events.size(), 1U);
    const MotionQueryTraceReplayEventUVE& event = fixture.events.front();
    EXPECT_EQ(event.sequence, 1U);
    EXPECT_EQ(event.frameNumber, 42U);
    EXPECT_EQ(event.kind, "accepted");
    EXPECT_EQ(event.candidatesConsidered, 8U);
    EXPECT_EQ(event.candidatesEvaluated, 4U);
    EXPECT_FLOAT_EQ(event.cost, 0.375F);
    EXPECT_EQ(event.selectedCandidateIndex, std::optional<std::size_t>{2U});
    EXPECT_EQ(event.qualityTier, 1U);
    EXPECT_EQ(event.continuityCode, 1U);
    EXPECT_TRUE(event.continuityApplied);
    EXPECT_EQ(event.transitionCode, 2U);
    EXPECT_EQ(event.telemetryCode, 0U);
    EXPECT_EQ(event.telemetryIndexEntryCount, 64U);
    EXPECT_EQ(event.telemetryCandidatesConsidered, 4U);
    EXPECT_TRUE(event.telemetryBudgetSaturated);
    EXPECT_EQ(event.provenance, "continuity_applied");
}

TEST(MotionQueryTraceReplayUVETest, CompareUVE_MatchesIdenticalTraceAndIgnoresRedactedFields) {
    const MotionQueryTraceSnapshotUVE original = MakeSnapshotUVE();
    const MotionQueryTraceReplayFixtureUVE fixture =
        BuildMotionQueryTraceReplayFixtureUVE(original);
    MotionQueryTraceSnapshotUVE replay = original;
    replay.events.front().timestampNanoseconds = 1U;
    replay.events.front().database.reset();
    replay.events.front().message = "different local diagnostic text";

    const MotionQueryTraceReplayComparisonUVE comparison =
        CompareMotionQueryTraceReplayFixtureUVE(fixture, replay);
    EXPECT_TRUE(comparison.IsMatchUVE());
    EXPECT_EQ(comparison.code, MotionQueryTraceReplayComparisonCodeUVE::Match);
    EXPECT_EQ(comparison.comparedEventCount, 1U);
    EXPECT_EQ(comparison.mismatchIndex, kMotionQueryTraceReplayNoMismatchIndexUVE);
    EXPECT_FALSE(comparison.IsTruncatedUVE());
}

TEST(MotionQueryTraceReplayUVETest, CompareUVE_ReportsFirstDeterministicEventMismatch) {
    const MotionQueryTraceReplayFixtureUVE fixture =
        BuildMotionQueryTraceReplayFixtureUVE(MakeSnapshotUVE());
    MotionQueryTraceSnapshotUVE changed = MakeSnapshotUVE();
    changed.events.front().telemetryCandidatesConsidered = 3U;

    const MotionQueryTraceReplayComparisonUVE comparison =
        CompareMotionQueryTraceReplayFixtureUVE(fixture, changed);
    EXPECT_EQ(comparison.code, MotionQueryTraceReplayComparisonCodeUVE::EventMismatch);
    EXPECT_FALSE(comparison.IsMatchUVE());
    EXPECT_EQ(comparison.comparedEventCount, 0U);
    EXPECT_EQ(comparison.mismatchIndex, 0U);
    EXPECT_FALSE(comparison.IsTruncatedUVE());
}

TEST(MotionQueryTraceReplayUVETest, CompareUVE_RetainsBoundedTraceAndReportsTruncationFacts) {
    MotionQueryTraceLoggerUVE logger;
    for (std::size_t index = 0U; index < kMotionQueryMaximumTraceEventsUVE + 2U; ++index) {
        MotionQueryTraceEventUVE event;
        event.timestampNanoseconds = 100U + index;
        event.frameNumber = 10U + index;
        event.kind = "tick";
        ASSERT_TRUE(logger.RecordUVE(event).IsAcceptedUVE());
    }

    const MotionQueryTraceSnapshotUVE snapshot = logger.GetSnapshotUVE();
    const MotionQueryTraceReplayFixtureUVE fixture =
        BuildMotionQueryTraceReplayFixtureUVE(snapshot);
    ASSERT_EQ(fixture.events.size(), kMotionQueryMaximumTraceReplayEventsUVE);
    ASSERT_TRUE(fixture.truncated);
    const MotionQueryTraceReplayComparisonUVE match =
        CompareMotionQueryTraceReplayFixtureUVE(fixture, snapshot);
    EXPECT_TRUE(match.IsMatchUVE());
    EXPECT_TRUE(match.IsTruncatedUVE());
    EXPECT_EQ(match.comparedEventCount, kMotionQueryMaximumTraceReplayEventsUVE);

    MotionQueryTraceSnapshotUVE unmarked = snapshot;
    unmarked.truncated = false;
    const MotionQueryTraceReplayComparisonUVE mismatch =
        CompareMotionQueryTraceReplayFixtureUVE(fixture, unmarked);
    EXPECT_EQ(mismatch.code, MotionQueryTraceReplayComparisonCodeUVE::TruncationMismatch);
    EXPECT_TRUE(mismatch.IsTruncatedUVE());
}

TEST(MotionQueryTraceReplayUVETest, CompareUVE_RejectsUnsupportedSchemaAndInvalidEventPayload) {
    MotionQueryTraceReplayFixtureUVE unsupported =
        BuildMotionQueryTraceReplayFixtureUVE(MakeSnapshotUVE());
    unsupported.schemaVersion = kMotionQueryTraceReplayFixtureSchemaVersionUVE + 1U;
    EXPECT_EQ(CompareMotionQueryTraceReplayFixtureUVE(unsupported, MakeSnapshotUVE()).code,
              MotionQueryTraceReplayComparisonCodeUVE::SchemaMismatch);

    MotionQueryTraceReplayFixtureUVE invalid =
        BuildMotionQueryTraceReplayFixtureUVE(MakeSnapshotUVE());
    invalid.events.front().sequence = 0U;
    EXPECT_EQ(CompareMotionQueryTraceReplayFixtureUVE(invalid, MakeSnapshotUVE()).code,
              MotionQueryTraceReplayComparisonCodeUVE::InvalidFixture);
}

} // namespace UVE::Plugins::Editor
