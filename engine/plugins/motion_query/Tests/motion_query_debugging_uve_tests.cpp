// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_debugging_uve.h"
#include "uve/plugins/motion_query_live_debug_session_uve.h"

#include <gtest/gtest.h>

namespace UVE::Plugins::Editor {
namespace {
UVE::Asset::ResourceHandleUVE MakeHandleUVE(std::uint64_t guid) {
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
} // namespace

TEST(MotionQueryDebuggingUVETest, TraceLoggerUVE_EnforcesBoundsAndMonotonicity) {
    MotionQueryTraceLoggerUVE logger;
    MotionQueryTraceEventUVE event;
    event.timestampNanoseconds = 10U;
    event.frameNumber = 1U;
    event.kind = "match";
    event.candidatesConsidered = 2U;
    event.candidatesEvaluated = 1U;
    event.cost = 0.25F;
    ASSERT_TRUE(logger.RecordUVE(event).IsAcceptedUVE());

    MotionQueryTraceEventUVE invalid = event;
    invalid.timestampNanoseconds = 9U;
    EXPECT_EQ(logger.RecordUVE(invalid).code, MotionQueryTraceCodeUVE::NonMonotonicTimestamp);

    invalid = event;
    invalid.timestampNanoseconds = 11U;
    invalid.candidatesEvaluated = 3U;
    EXPECT_EQ(logger.RecordUVE(invalid).code, MotionQueryTraceCodeUVE::InvalidEvent);

    for (std::size_t index = 0U; index < kMotionQueryMaximumTraceEventsUVE + 1U; ++index) {
        MotionQueryTraceEventUVE bounded;
        bounded.timestampNanoseconds = 100U + index;
        bounded.frameNumber = 2U + index;
        bounded.kind = "tick";
        ASSERT_TRUE(logger.RecordUVE(bounded).IsAcceptedUVE());
    }
    const MotionQueryTraceSnapshotUVE snapshot = logger.GetSnapshotUVE();
    EXPECT_EQ(snapshot.events.size(), kMotionQueryMaximumTraceEventsUVE);
    EXPECT_TRUE(snapshot.truncated);
}

TEST(MotionQueryDebuggingUVETest, DebuggerUVE_PublishesCopiedSelectedCandidateFacts) {
    MotionQueryDebuggerUVE debugger;
    const UVE::Asset::ResourceHandleUVE databaseHandle = MakeHandleUVE(7U);
    debugger.AttachUVE(databaseHandle, MakeDatabaseUVE());
    debugger.PublishMatchUVE(0U, 1U, 0.5F, "matched");
    const MotionQueryDebuggerSnapshotUVE snapshot = debugger.GetSnapshotUVE();
    ASSERT_TRUE(snapshot.attached);
    EXPECT_EQ(snapshot.database, databaseHandle);
    EXPECT_EQ(snapshot.selectedCandidateIndex, std::optional<std::size_t>{0U});
    EXPECT_EQ(snapshot.selectedCandidateId, "candidate-0");
    EXPECT_EQ(snapshot.selectedSourceClipId, "walk");
    EXPECT_EQ(snapshot.candidatesEvaluated, 1U);

    UVE::Plugins::MotionQueryAnimationNodeResultUVE provenanceResult;
    provenanceResult.code = UVE::Plugins::MotionQueryAnimationNodeCodeUVE::Accepted;
    provenanceResult.candidateIndex = 0U;
    provenanceResult.candidatesEvaluated = 1U;
    provenanceResult.cost = 0.25F;
    provenanceResult.qualityTier = UVE::Plugins::MotionQueryQualityTierUVE::Reduced;
    provenanceResult.continuityCode = UVE::Plugins::MotionQueryContinuityCodeUVE::Applied;
    provenanceResult.continuityApplied = true;
    provenanceResult.transitionCode = UVE::Plugins::MotionQueryTransitionCodeUVE::HeldPreviousCandidate;
    provenanceResult.transitionHeldPrevious = true;
    provenanceResult.message = "held previous candidate";
    debugger.PublishMatchUVE(provenanceResult);
    const MotionQueryDebuggerSnapshotUVE provenance = debugger.GetSnapshotUVE();
    EXPECT_EQ(provenance.qualityTier, 1U);
    EXPECT_EQ(provenance.continuityCode, 1U);
    EXPECT_TRUE(provenance.continuityApplied);
    EXPECT_EQ(provenance.transitionCode, 3U);
    EXPECT_TRUE(provenance.transitionHeldPrevious);
    EXPECT_EQ(provenance.provenance, "history_hold");

    debugger.PublishMatchUVE(4U, 1U, 0.5F, "invalid");
    EXPECT_EQ(debugger.GetSnapshotUVE().selectedCandidateId, "candidate-0");
}

TEST(MotionQueryDebuggingUVETest, LiveDebugSessionUVE_AttachesFiltersPublishesAndRejectsStaleCommands) {
    MotionQueryEditorAuthoringSessionUVE authoring;
    MotionQueryEditorDatabaseEntryUVE entry;
    entry.resource = MakeHandleUVE(31U);
    entry.displayName = "Locomotion";
    entry.contract = MakeContractUVE();
    MotionQueryEditorCommandUVE registerCommand;
    registerCommand.requestId = 1U;
    registerCommand.kind = MotionQueryEditorCommandKindUVE::RegisterDatabase;
    registerCommand.database = entry;
    ASSERT_TRUE(authoring.DispatchUVE(registerCommand).applied);

    MotionQueryLiveDebugSessionUVE session;
    MotionQueryLiveDebugCommandUVE attach;
    attach.requestId = 2U;
    attach.kind = MotionQueryLiveDebugCommandKindUVE::Attach;
    attach.database = entry.resource;
    const MotionQueryLiveDebugResponseUVE attached = session.DispatchUVE(attach, authoring);
    ASSERT_TRUE(attached.applied) << attached.message;
    ASSERT_TRUE(attached.snapshot.active);
    EXPECT_EQ(attached.snapshot.database, entry.resource);

    UVE::Plugins::MotionQueryAnimationNodeResultUVE result;
    result.code = UVE::Plugins::MotionQueryAnimationNodeCodeUVE::Accepted;
    result.candidateIndex = 0U;
    result.candidatesEvaluated = 1U;
    result.cost = 0.25F;
    result.qualityTier = UVE::Plugins::MotionQueryQualityTierUVE::Minimal;
    result.continuityCode = UVE::Plugins::MotionQueryContinuityCodeUVE::Applied;
    result.continuityApplied = true;
    result.transitionCode = UVE::Plugins::MotionQueryTransitionCodeUVE::SwitchedCandidate;
    result.sourceClipId = "walk";
    result.message = "accepted live match";
    session.PublishUVE(result, 100U, 4U);
    const MotionQueryLiveDebugSnapshotUVE published = session.GetSnapshotUVE();
    ASSERT_EQ(published.traceEvents.size(), 1U);
    EXPECT_EQ(published.traceEvents.front().kind, "accepted");
    EXPECT_EQ(published.traceEvents.front().selectedCandidateIndex, std::optional<std::size_t>{0U});
    EXPECT_EQ(published.traceEvents.front().qualityTier, 2U);
    EXPECT_EQ(published.traceEvents.front().continuityCode, 1U);
    EXPECT_TRUE(published.traceEvents.front().continuityApplied);
    EXPECT_EQ(published.traceEvents.front().transitionCode, 2U);
    EXPECT_EQ(published.traceEvents.front().provenance, "continuity_applied");
    EXPECT_EQ(published.debugger.selectedCandidateId, "candidate-0");
    EXPECT_EQ(published.debugger.provenance, "continuity_applied");

    MotionQueryLiveDebugCommandUVE filter = attach;
    filter.requestId = 3U;
    filter.kind = MotionQueryLiveDebugCommandKindUVE::SetFilter;
    filter.expectedGeneration = attached.snapshot.generation;
    filter.filter = "accepted";
    ASSERT_TRUE(session.DispatchUVE(filter, authoring).applied);
    EXPECT_EQ(session.GetSnapshotUVE().visibleTraceEventCount, 1U);

    MotionQueryLiveDebugCommandUVE stale = filter;
    stale.requestId = 4U;
    stale.expectedGeneration = attached.snapshot.generation;
    stale.filter = "other";
    EXPECT_EQ(session.DispatchUVE(stale, authoring).code,
              MotionQueryLiveDebugResponseCodeUVE::StaleGeneration);

    MotionQueryLiveDebugCommandUVE clear = filter;
    clear.requestId = 5U;
    clear.kind = MotionQueryLiveDebugCommandKindUVE::ClearTrace;
    clear.expectedGeneration = session.GetSnapshotUVE().generation;
    ASSERT_TRUE(session.DispatchUVE(clear, authoring).applied);
    EXPECT_TRUE(session.GetSnapshotUVE().traceEvents.empty());
}

TEST(MotionQueryDebuggingUVETest, ProfilerAdapterUVE_DelegatesToExistingCaptureAuthority) {
    UVE::Core::ProfilerCaptureUVE profiler;
    ASSERT_TRUE(profiler.BeginUVE("motion-query", 1U).IsAcceptedUVE());
    MotionQueryTraceProfilerAdapterUVE adapter;
    MotionQueryTraceEventUVE event;
    event.kind = "match";
    event.timestampNanoseconds = 2U;
    event.frameNumber = 3U;
    ASSERT_TRUE(adapter.RecordMatchUVE(profiler, event).IsAcceptedUVE());
    EXPECT_TRUE(profiler.EndUVE(3U).IsAcceptedUVE());
}

TEST(MotionQueryDebuggingUVETest, AutomatedValidationUVE_ReportsDatabaseIndexAndInventoryDisposition) {
    std::vector<UVE::Core::MotionQueryDatabaseContractUVE> databases = {MakeContractUVE()};
    std::vector<MotionQuerySearchIndexUVE> indices(1U);
    const std::vector<MotionQueryInventoryReviewUVE> validReviews = {
        MotionQueryInventoryReviewUVE{"MotionQueryDebugger.cpp",
                                     MotionQueryInventoryDispositionUVE::RewrittenNative,
                                     "rewritten against native copied diagnostics"},
        MotionQueryInventoryReviewUVE{"Trace_MotionQueryTrace.h",
                                     MotionQueryInventoryDispositionUVE::RejectedUnrealSpecific,
                                     "Unreal Trace macros have no native equivalent"},
    };
    MotionQueryAutomatedValidationReportUVE invalid =
        ValidateMotionQueryRuntimeUVE(databases, indices, validReviews);
    EXPECT_FALSE(invalid.valid);
    ASSERT_FALSE(invalid.diagnostics.empty());

    ASSERT_TRUE(indices[0].BuildUVE(databases[0].database,
                                    UVE::Core::MotionQueryFeatureSchemaUVE{
                                        1U,
                                        {UVE::Core::MotionQueryFeatureChannelUVE{
                                            "velocity", UVE::Core::MotionQueryFeatureChannelKindUVE::RootVelocity,
                                            0U, 1.0F}}})
                    .IsAcceptedUVE());
    const std::vector<MotionQueryInventoryReviewUVE> invalidReviews = {
        MotionQueryInventoryReviewUVE{"", MotionQueryInventoryDispositionUVE::Deferred, ""},
    };
    const MotionQueryAutomatedValidationReportUVE invalidInventory =
        ValidateMotionQueryRuntimeUVE(databases, indices, invalidReviews);
    EXPECT_FALSE(invalidInventory.valid);
    EXPECT_FALSE(invalidInventory.diagnostics.empty());
}
} // namespace UVE::Plugins::Editor
