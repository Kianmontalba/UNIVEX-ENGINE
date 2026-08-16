// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_trace_replay_regression_uve.h"

#include <gtest/gtest.h>

namespace UVE::Plugins::Editor {
namespace {

UVE::Asset::ResourceHandleUVE MakeHandleUVE(const std::uint64_t guid) {
    return UVE::Asset::ResourceHandleUVE{UVE::Asset::AssetGuidUVE{guid}, 1U};
}

UVE::Core::MotionMatchingDatabaseUVE MakeDatabaseUVE() {
    UVE::Core::MotionMatchingDatabaseUVE database;
    UVE::Core::MotionMatchingCandidateUVE candidate;
    candidate.candidateId = "candidate-0";
    candidate.sourceClipId = "walk";
    candidate.feature.facingDirection = UVE::Math::Vector3UVE{0.0F, 0.0F, 1.0F};
    database.candidates = {candidate};
    return database;
}

UVE::Core::MotionQueryDatabaseContractUVE MakeContractUVE() {
    UVE::Core::MotionQueryDatabaseContractUVE contract;
    contract.context.databaseId = "locomotion";
    contract.context.generation = 1U;
    contract.schema.schemaId = "locomotion-v1";
    contract.settings.maximumCandidates = 8U;
    contract.database = MakeDatabaseUVE();
    return contract;
}

void MakePublishedSnapshotUVE(MotionQueryLiveDebugSnapshotUVE& outSnapshot) {
    MotionQueryEditorAuthoringSessionUVE authoring;
    MotionQueryEditorDatabaseEntryUVE entry;
    entry.resource = MakeHandleUVE(31U);
    entry.displayName = "Locomotion";
    entry.contract = MakeContractUVE();
    MotionQueryEditorCommandUVE registerCommand;
    registerCommand.requestId = 1U;
    registerCommand.kind = MotionQueryEditorCommandKindUVE::RegisterDatabase;
    registerCommand.database = entry;
    EXPECT_TRUE(authoring.DispatchUVE(registerCommand).applied);

    MotionQueryLiveDebugSessionUVE session;
    MotionQueryLiveDebugCommandUVE attach;
    attach.requestId = 2U;
    attach.kind = MotionQueryLiveDebugCommandKindUVE::Attach;
    attach.database = entry.resource;
    ASSERT_TRUE(session.DispatchUVE(attach, authoring).applied);

    UVE::Plugins::MotionQueryAnimationNodeResultUVE result;
    result.code = UVE::Plugins::MotionQueryAnimationNodeCodeUVE::Accepted;
    result.candidateIndex = 0U;
    result.candidatesEvaluated = 1U;
    result.cost = 0.25F;
    result.qualityTier = UVE::Plugins::MotionQueryQualityTierUVE::Minimal;
    result.continuityCode = UVE::Plugins::MotionQueryContinuityCodeUVE::Applied;
    result.continuityApplied = true;
    result.transitionCode = UVE::Plugins::MotionQueryTransitionCodeUVE::SwitchedCandidate;
    result.telemetryCode = UVE::Plugins::MotionQueryRuntimeTelemetryCodeUVE::Accepted;
    result.telemetryIndexEntryCount = 2U;
    result.telemetryCandidatesConsidered = 1U;
    result.telemetryBudgetSaturated = false;
    result.sourceClipId = "walk";
    result.message = "accepted live match";
    session.PublishUVE(result, 100U, 4U);
    outSnapshot = session.GetSnapshotUVE();
}

MotionQueryTraceReplayFixtureUVE MakeFixtureFromSnapshotUVE(
    const MotionQueryLiveDebugSnapshotUVE& snapshot) {
    MotionQueryTraceSnapshotUVE trace;
    trace.truncated = snapshot.traceTruncated;
    trace.events = snapshot.traceEvents;
    return BuildMotionQueryTraceReplayFixtureUVE(trace);
}

} // namespace

TEST(MotionQueryTraceReplayRegressionUVETest, CaptureUVE_CopiesAcceptedLiveDebugTraceIntoFixture) {
    MotionQueryLiveDebugSnapshotUVE snapshot;
    MakePublishedSnapshotUVE(snapshot);

    const MotionQueryTraceReplayCaptureResultUVE captured =
        CaptureMotionQueryTraceReplayFixtureUVE(snapshot);
    ASSERT_TRUE(captured.IsAcceptedUVE());
    ASSERT_TRUE(captured.fixture.has_value());
    EXPECT_EQ(captured.fixture->events.size(), 1U);
    EXPECT_EQ(captured.fixture->events.front().kind, "accepted");
    EXPECT_EQ(captured.fixture->events.front().provenance, "continuity_applied");
    EXPECT_FALSE(captured.message.empty());
}

TEST(MotionQueryTraceReplayRegressionUVETest, CaptureUVE_RejectsFilteredEvidence) {
    MotionQueryLiveDebugSnapshotUVE snapshot;
    MakePublishedSnapshotUVE(snapshot);
    snapshot.filter = "accepted";

    const MotionQueryTraceReplayCaptureResultUVE captured =
        CaptureMotionQueryTraceReplayFixtureUVE(snapshot);
    EXPECT_EQ(captured.code, MotionQueryTraceReplayCaptureCodeUVE::FilteredSnapshot);
    EXPECT_FALSE(captured.fixture.has_value());
}

TEST(MotionQueryTraceReplayRegressionUVETest, CaptureUVE_RejectsEmptyTrace) {
    const MotionQueryTraceReplayCaptureResultUVE captured =
        CaptureMotionQueryTraceReplayFixtureUVE(MotionQueryLiveDebugSnapshotUVE{});
    EXPECT_EQ(captured.code, MotionQueryTraceReplayCaptureCodeUVE::EmptyTrace);
    EXPECT_FALSE(captured.fixture.has_value());
}

TEST(MotionQueryTraceReplayRegressionUVETest, LiveDebugSnapshotUVE_MatchesAnimationEvaluationTrace) {
    MotionQueryLiveDebugSnapshotUVE snapshot;
    MakePublishedSnapshotUVE(snapshot);
    const MotionQueryTraceReplayFixtureUVE fixture = MakeFixtureFromSnapshotUVE(snapshot);

    const MotionQueryTraceReplayRegressionResultUVE result =
        CompareMotionQueryLiveDebugSnapshotAgainstFixtureUVE(fixture, snapshot);
    EXPECT_TRUE(result.IsMatchUVE());
    ASSERT_TRUE(result.comparison.has_value());
    EXPECT_EQ(result.comparison->comparedEventCount, 1U);
}

TEST(MotionQueryTraceReplayRegressionUVETest, NamedBaselineUVE_MatchesCopiedFixtureAndPublishesGeneration) {
    MotionQueryLiveDebugSnapshotUVE snapshot;
    MakePublishedSnapshotUVE(snapshot);
    MotionQueryTraceSnapshotUVE trace;
    trace.truncated = snapshot.traceTruncated;
    trace.events = snapshot.traceEvents;
    const MotionQueryTraceReplayCompatibilityUVE compatibility{3U, 4U, 5U, 9U};
    const MotionQueryTraceReplayFixtureUVE fixture =
        BuildMotionQueryTraceReplayFixtureUVE(trace, compatibility);
    MotionQueryTraceReplayBaselineRegistryUVE registry;
    const MotionQueryTraceReplayBaselineResultUVE registered = registry.RegisterUVE("ci.fixture", fixture);
    ASSERT_TRUE(registered.IsAcceptedUVE());

    const MotionQueryTraceReplayBaselineRegressionResultUVE result =
        CompareMotionQueryLiveDebugSnapshotAgainstNamedBaselineUVE(
            registry, "ci.fixture", snapshot, registered.registryGeneration, compatibility);
    EXPECT_TRUE(result.IsMatchUVE());
    EXPECT_EQ(result.code, MotionQueryTraceReplayBaselineRegressionCodeUVE::Match);
    EXPECT_EQ(result.baselineName, "ci.fixture");
    EXPECT_EQ(result.registryGeneration, registered.registryGeneration);
    ASSERT_TRUE(result.comparison.has_value());
    EXPECT_EQ(result.comparison->comparedEventCount, 1U);
}

TEST(MotionQueryTraceReplayRegressionUVETest, BatchUVE_EvaluatesSortedBaselinesWithDeterministicCounters) {
    MotionQueryLiveDebugSnapshotUVE snapshot;
    MakePublishedSnapshotUVE(snapshot);
    const MotionQueryTraceReplayFixtureUVE fixture = MakeFixtureFromSnapshotUVE(snapshot);
    MotionQueryTraceReplayFixtureUVE mismatchFixture = fixture;
    mismatchFixture.events.front().cost = 0.5F;
    MotionQueryTraceReplayBaselineRegistryUVE registry;
    ASSERT_TRUE(registry.RegisterUVE("zeta", mismatchFixture).IsAcceptedUVE());
    ASSERT_TRUE(registry.RegisterUVE("alpha", fixture).IsAcceptedUVE());
    const std::uint64_t generation = registry.GetSnapshotUVE().generation;

    const MotionQueryTraceReplayBaselineBatchResultUVE batch =
        CompareMotionQueryLiveDebugSnapshotAgainstAllBaselinesUVE(registry, snapshot, generation);
    EXPECT_EQ(batch.code, MotionQueryTraceReplayBaselineBatchCodeUVE::Accepted);
    EXPECT_TRUE(batch.IsCompleteUVE());
    EXPECT_EQ(batch.registryGeneration, generation);
    EXPECT_EQ(batch.evaluatedBaselineCount, 2U);
    EXPECT_EQ(batch.matchCount, 1U);
    EXPECT_EQ(batch.mismatchCount, 1U);
    ASSERT_EQ(batch.results.size(), 2U);
    EXPECT_EQ(batch.results[0].baselineName, "alpha");
    EXPECT_EQ(batch.results[0].code, MotionQueryTraceReplayBaselineRegressionCodeUVE::Match);
    EXPECT_EQ(batch.results[1].baselineName, "zeta");
    EXPECT_EQ(batch.results[1].code, MotionQueryTraceReplayBaselineRegressionCodeUVE::Mismatch);

    const MotionQueryTraceReplayBaselineBatchResultUVE stale =
        CompareMotionQueryLiveDebugSnapshotAgainstAllBaselinesUVE(registry, snapshot, generation - 1U);
    EXPECT_EQ(stale.code, MotionQueryTraceReplayBaselineBatchCodeUVE::StaleGeneration);
    EXPECT_TRUE(stale.results.empty());
}

TEST(MotionQueryTraceReplayRegressionUVETest, BatchUVE_ReportsFilteredEvidenceForEachBoundedBaseline) {
    MotionQueryLiveDebugSnapshotUVE snapshot;
    MakePublishedSnapshotUVE(snapshot);
    const MotionQueryTraceReplayFixtureUVE fixture = MakeFixtureFromSnapshotUVE(snapshot);
    MotionQueryTraceReplayBaselineRegistryUVE registry;
    ASSERT_TRUE(registry.RegisterUVE("alpha", fixture).IsAcceptedUVE());
    ASSERT_TRUE(registry.RegisterUVE("beta", fixture).IsAcceptedUVE());
    snapshot.filter = "accepted";

    const MotionQueryTraceReplayBaselineBatchResultUVE batch =
        CompareMotionQueryLiveDebugSnapshotAgainstAllBaselinesUVE(registry, snapshot);
    EXPECT_EQ(batch.code, MotionQueryTraceReplayBaselineBatchCodeUVE::FilteredSnapshot);
    EXPECT_FALSE(batch.IsCompleteUVE());
    EXPECT_EQ(batch.evaluatedBaselineCount, 2U);
    EXPECT_EQ(batch.mismatchCount, 2U);
    ASSERT_EQ(batch.results.size(), 2U);
    EXPECT_EQ(batch.results[0].code, MotionQueryTraceReplayBaselineRegressionCodeUVE::FilteredSnapshot);
    EXPECT_EQ(batch.results[1].code, MotionQueryTraceReplayBaselineRegressionCodeUVE::FilteredSnapshot);
}

TEST(MotionQueryTraceReplayRegressionUVETest, NamedBaselineUVE_ReportsFieldMismatchAndRejectsStaleGeneration) {
    MotionQueryLiveDebugSnapshotUVE snapshot;
    MakePublishedSnapshotUVE(snapshot);
    const MotionQueryTraceReplayFixtureUVE fixture = MakeFixtureFromSnapshotUVE(snapshot);
    MotionQueryTraceReplayBaselineRegistryUVE registry;
    const MotionQueryTraceReplayBaselineResultUVE registered = registry.RegisterUVE("ci.fixture", fixture);
    ASSERT_TRUE(registered.IsAcceptedUVE());
    snapshot.traceEvents.front().telemetryCandidatesConsidered = 2U;

    const MotionQueryTraceReplayBaselineRegressionResultUVE mismatch =
        CompareMotionQueryLiveDebugSnapshotAgainstNamedBaselineUVE(
            registry, "ci.fixture", snapshot, registered.registryGeneration);
    EXPECT_EQ(mismatch.code, MotionQueryTraceReplayBaselineRegressionCodeUVE::Mismatch);
    ASSERT_TRUE(mismatch.comparison.has_value());
    EXPECT_EQ(mismatch.comparison->mismatchFieldMask,
              static_cast<std::uint32_t>(MotionQueryTraceReplayMismatchFieldUVE::TelemetryCandidatesConsidered));
    EXPECT_EQ(mismatch.comparison->diagnosticSummary, "telemetryCandidatesConsidered");

    const MotionQueryTraceReplayBaselineRegressionResultUVE stale =
        CompareMotionQueryLiveDebugSnapshotAgainstNamedBaselineUVE(
            registry, "ci.fixture", snapshot, registered.registryGeneration - 1U);
    EXPECT_EQ(stale.code, MotionQueryTraceReplayBaselineRegressionCodeUVE::StaleGeneration);
    EXPECT_FALSE(stale.comparison.has_value());
}

TEST(MotionQueryTraceReplayRegressionUVETest, NamedBaselineUVE_RejectsMissingAndFilteredEvidence) {
    MotionQueryLiveDebugSnapshotUVE snapshot;
    MakePublishedSnapshotUVE(snapshot);
    const MotionQueryTraceReplayFixtureUVE fixture = MakeFixtureFromSnapshotUVE(snapshot);
    MotionQueryTraceReplayBaselineRegistryUVE registry;
    ASSERT_TRUE(registry.RegisterUVE("ci.fixture", fixture).IsAcceptedUVE());

    const MotionQueryTraceReplayBaselineRegressionResultUVE missing =
        CompareMotionQueryLiveDebugSnapshotAgainstNamedBaselineUVE(registry, "missing", snapshot);
    EXPECT_EQ(missing.code, MotionQueryTraceReplayBaselineRegressionCodeUVE::BaselineNotFound);
    EXPECT_FALSE(missing.comparison.has_value());

    snapshot.filter = "accepted";
    const MotionQueryTraceReplayBaselineRegressionResultUVE filtered =
        CompareMotionQueryLiveDebugSnapshotAgainstNamedBaselineUVE(registry, "ci.fixture", snapshot);
    EXPECT_EQ(filtered.code, MotionQueryTraceReplayBaselineRegressionCodeUVE::FilteredSnapshot);
    EXPECT_FALSE(filtered.comparison.has_value());
}

TEST(MotionQueryTraceReplayRegressionUVETest, LiveDebugSnapshotUVE_ReportsDeterministicEventMismatch) {
    MotionQueryLiveDebugSnapshotUVE snapshot;
    MakePublishedSnapshotUVE(snapshot);
    const MotionQueryTraceReplayFixtureUVE fixture = MakeFixtureFromSnapshotUVE(snapshot);
    snapshot.traceEvents.front().telemetryCandidatesConsidered = 2U;

    const MotionQueryTraceReplayRegressionResultUVE result =
        CompareMotionQueryLiveDebugSnapshotAgainstFixtureUVE(fixture, snapshot);
    EXPECT_EQ(result.code, MotionQueryTraceReplayRegressionCodeUVE::Mismatch);
    ASSERT_TRUE(result.comparison.has_value());
    EXPECT_EQ(result.comparison->code, MotionQueryTraceReplayComparisonCodeUVE::EventMismatch);
    EXPECT_EQ(result.comparison->mismatchIndex, 0U);
}

TEST(MotionQueryTraceReplayRegressionUVETest, LiveDebugSnapshotUVE_RejectsFilteredViewAsIncompleteEvidence) {
    MotionQueryLiveDebugSnapshotUVE snapshot;
    MakePublishedSnapshotUVE(snapshot);
    const MotionQueryTraceReplayFixtureUVE fixture = MakeFixtureFromSnapshotUVE(snapshot);
    snapshot.filter = "accepted";

    const MotionQueryTraceReplayRegressionResultUVE result =
        CompareMotionQueryLiveDebugSnapshotAgainstFixtureUVE(fixture, snapshot);
    EXPECT_EQ(result.code, MotionQueryTraceReplayRegressionCodeUVE::FilteredSnapshot);
    EXPECT_FALSE(result.comparison.has_value());
}

TEST(MotionQueryTraceReplayRegressionUVETest, LiveDebugSnapshotUVE_RejectsEmptyTrace) {
    MotionQueryLiveDebugSnapshotUVE snapshot;
    const MotionQueryTraceReplayFixtureUVE fixture = BuildMotionQueryTraceReplayFixtureUVE(
        MotionQueryTraceSnapshotUVE{});

    const MotionQueryTraceReplayRegressionResultUVE result =
        CompareMotionQueryLiveDebugSnapshotAgainstFixtureUVE(fixture, snapshot);
    EXPECT_EQ(result.code, MotionQueryTraceReplayRegressionCodeUVE::EmptyTrace);
    EXPECT_FALSE(result.comparison.has_value());
}

TEST(MotionQueryTraceReplayRegressionUVETest, LiveDebugSnapshotUVE_EnforcesCompatibilityIdentity) {
    MotionQueryLiveDebugSnapshotUVE snapshot;
    MakePublishedSnapshotUVE(snapshot);
    const MotionQueryTraceReplayCompatibilityUVE compatibility{3U, 4U, 5U, 9U};
    MotionQueryTraceSnapshotUVE trace;
    trace.truncated = snapshot.traceTruncated;
    trace.events = snapshot.traceEvents;
    const MotionQueryTraceReplayFixtureUVE fixture =
        BuildMotionQueryTraceReplayFixtureUVE(trace, compatibility);

    EXPECT_TRUE(CompareMotionQueryLiveDebugSnapshotAgainstFixtureUVE(fixture, snapshot, compatibility)
                    .IsMatchUVE());
    const MotionQueryTraceReplayCompatibilityUVE changed{3U, 4U, 6U, 9U};
    const MotionQueryTraceReplayRegressionResultUVE mismatch =
        CompareMotionQueryLiveDebugSnapshotAgainstFixtureUVE(fixture, snapshot, changed);
    EXPECT_EQ(mismatch.code, MotionQueryTraceReplayRegressionCodeUVE::Mismatch);
    ASSERT_TRUE(mismatch.comparison.has_value());
    EXPECT_EQ(mismatch.comparison->code,
              MotionQueryTraceReplayComparisonCodeUVE::CompatibilityMismatch);
}

} // namespace UVE::Plugins::Editor
