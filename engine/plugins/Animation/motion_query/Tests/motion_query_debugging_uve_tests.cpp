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
    provenanceResult.telemetryCode = UVE::Plugins::MotionQueryRuntimeTelemetryCodeUVE::Accepted;
    provenanceResult.telemetryIndexEntryCount = 64U;
    provenanceResult.telemetryCandidatesConsidered = 1U;
    provenanceResult.telemetryBudgetSaturated = true;
    provenanceResult.message = "held previous candidate";
    debugger.PublishMatchUVE(provenanceResult);
    const MotionQueryDebuggerSnapshotUVE provenance = debugger.GetSnapshotUVE();
    EXPECT_EQ(provenance.qualityTier, 1U);
    EXPECT_EQ(provenance.continuityCode, 1U);
    EXPECT_TRUE(provenance.continuityApplied);
    EXPECT_EQ(provenance.transitionCode, 3U);
    EXPECT_TRUE(provenance.transitionHeldPrevious);
    EXPECT_EQ(provenance.telemetryCode, 0U);
    EXPECT_EQ(provenance.telemetryIndexEntryCount, 64U);
    EXPECT_EQ(provenance.telemetryCandidatesConsidered, 1U);
    EXPECT_TRUE(provenance.telemetryBudgetSaturated);
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
    result.telemetryCode = UVE::Plugins::MotionQueryRuntimeTelemetryCodeUVE::Accepted;
    result.telemetryIndexEntryCount = 2U;
    result.telemetryCandidatesConsidered = 1U;
    result.telemetryBudgetSaturated = false;
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
    EXPECT_EQ(published.traceEvents.front().telemetryCode, 0U);
    EXPECT_EQ(published.traceEvents.front().telemetryIndexEntryCount, 2U);
    EXPECT_EQ(published.traceEvents.front().telemetryCandidatesConsidered, 1U);
    EXPECT_FALSE(published.traceEvents.front().telemetryBudgetSaturated);
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

TEST(MotionQueryDebuggingUVETest, LiveDebugTracePersistenceUVE_RoundTripsAnnotationsWithoutRuntimeIdentity) {
    MotionQueryTraceSnapshotUVE snapshot;
    snapshot.generation = 9U;
    snapshot.truncated = true;
    MotionQueryTraceEventUVE event;
    event.sequence = 4U;
    event.timestampNanoseconds = 987654321U;
    event.frameNumber = 12U;
    event.kind = "accepted";
    event.database = MakeHandleUVE(77U);
    event.candidatesConsidered = 2U;
    event.candidatesEvaluated = 1U;
    event.cost = 0.25F;
    event.selectedCandidateIndex = 0U;
    event.provenance = "runtime_search";
    event.message = "accepted live match";
    event.comment = "investigate foot slide";
    event.category = "locomotion";
    event.pinned = true;
    snapshot.events.push_back(event);

    const auto serialized = SerializeMotionQueryLiveDebugTraceUVE(snapshot, "locomotion");
    ASSERT_TRUE(serialized.IsAcceptedUVE()) << serialized.message;
    EXPECT_EQ(serialized.payload.find("timestampNanoseconds"), std::string::npos);
    EXPECT_EQ(serialized.payload.find("database"), std::string::npos);

    const auto restored = DeserializeMotionQueryLiveDebugTraceUVE(serialized.payload);
    ASSERT_TRUE(restored.IsAcceptedUVE()) << restored.message;
    ASSERT_EQ(restored.envelope->events.size(), 1U);
    EXPECT_EQ(restored.envelope->filter, "locomotion");
    EXPECT_TRUE(restored.envelope->truncated);
    EXPECT_EQ(restored.envelope->events.front().sequence, 4U);
    EXPECT_EQ(restored.envelope->events.front().comment, "investigate foot slide");
    EXPECT_EQ(restored.envelope->events.front().category, "locomotion");
    EXPECT_TRUE(restored.envelope->events.front().pinned);
    EXPECT_FALSE(restored.envelope->events.front().database.has_value());
    EXPECT_EQ(restored.envelope->events.front().timestampNanoseconds, 0U);
}

TEST(MotionQueryDebuggingUVETest, LiveDebugTracePersistenceUVE_RejectsUnexpectedAndNonMonotonicPayloads) {
    MotionQueryTraceSnapshotUVE snapshot;
    MotionQueryTraceEventUVE first;
    first.sequence = 1U;
    first.frameNumber = 2U;
    first.kind = "tick";
    snapshot.events.push_back(first);
    const auto serialized = SerializeMotionQueryLiveDebugTraceUVE(snapshot, "");
    ASSERT_TRUE(serialized.IsAcceptedUVE());

    const auto unexpected = DeserializeMotionQueryLiveDebugTraceUVE(
        serialized.payload.substr(0, serialized.payload.size() - 1U) + ",\"unexpected\":true}");
    EXPECT_EQ(unexpected.code, MotionQueryLiveDebugTracePersistenceCodeUVE::UnexpectedField);

    const std::string nonMonotonic =
        "{\"schemaVersion\":1,\"truncated\":false,\"filter\":\"\",\"events\":["
        "{\"sequence\":2,\"frameNumber\":3,\"kind\":\"a\",\"candidatesConsidered\":0,"
        "\"candidatesEvaluated\":0,\"cost\":0.0,\"selectedCandidateIndex\":null,"
        "\"qualityTier\":0,\"continuityCode\":0,\"continuityApplied\":false,\"transitionCode\":0,"
        "\"transitionHeldPrevious\":false,\"telemetryCode\":0,\"telemetryIndexEntryCount\":0,"
        "\"telemetryCandidatesConsidered\":0,\"telemetryBudgetSaturated\":false,"
        "\"provenance\":\"\",\"message\":\"\",\"comment\":\"\",\"category\":\"\",\"pinned\":false},"
        "{\"sequence\":1,\"frameNumber\":4,\"kind\":\"b\",\"candidatesConsidered\":0,"
        "\"candidatesEvaluated\":0,\"cost\":0.0,\"selectedCandidateIndex\":null,"
        "\"qualityTier\":0,\"continuityCode\":0,\"continuityApplied\":false,\"transitionCode\":0,"
        "\"transitionHeldPrevious\":false,\"telemetryCode\":0,\"telemetryIndexEntryCount\":0,"
        "\"telemetryCandidatesConsidered\":0,\"telemetryBudgetSaturated\":false,"
        "\"provenance\":\"\",\"message\":\"\",\"comment\":\"\",\"category\":\"\",\"pinned\":false}]}";
    EXPECT_EQ(DeserializeMotionQueryLiveDebugTraceUVE(nonMonotonic).code,
              MotionQueryLiveDebugTracePersistenceCodeUVE::InvalidTrace);
}

TEST(MotionQueryDebuggingUVETest, LiveDebugTracePersistenceUVE_EnforcesSchemaAndEventBounds) {
    const auto schemaMismatch = DeserializeMotionQueryLiveDebugTraceUVE(
        R"({"schemaVersion":2,"truncated":false,"filter":"","events":[]})");
    EXPECT_EQ(schemaMismatch.code, MotionQueryLiveDebugTracePersistenceCodeUVE::SchemaMismatch);

    MotionQueryTraceSnapshotUVE oversized;
    oversized.events.reserve(kMotionQueryMaximumTraceEventsUVE + 1U);
    for (std::size_t index = 0U; index <= kMotionQueryMaximumTraceEventsUVE; ++index) {
        MotionQueryTraceEventUVE event;
        event.sequence = index + 1U;
        event.frameNumber = index + 1U;
        event.kind = "tick";
        oversized.events.push_back(std::move(event));
    }
    const auto oversizedResult = SerializeMotionQueryLiveDebugTraceUVE(oversized, "");
    EXPECT_EQ(oversizedResult.code, MotionQueryLiveDebugTracePersistenceCodeUVE::InvalidTrace);
    EXPECT_TRUE(oversizedResult.payload.empty());
}

TEST(MotionQueryDebuggingUVETest, LiveDebugTraceAnalyzerUVE_ProducesDeterministicBoundedSummary) {
    MotionQueryTraceSnapshotUVE snapshot;
    MotionQueryTraceEventUVE first;
    first.sequence = 1U;
    first.frameNumber = 4U;
    first.kind = "zeta";
    first.candidatesConsidered = 5U;
    first.candidatesEvaluated = 3U;
    first.cost = 1.25F;
    snapshot.events.push_back(first);
    MotionQueryTraceEventUVE second = first;
    second.sequence = 2U;
    second.kind = "alpha";
    second.candidatesEvaluated = 4U;
    second.cost = 0.5F;
    snapshot.events.push_back(second);
    MotionQueryTraceEventUVE third = first;
    third.sequence = 3U;
    third.frameNumber = 5U;
    third.candidatesEvaluated = 1U;
    third.cost = 0.25F;
    snapshot.events.push_back(third);

    const auto result = AnalyzeMotionQueryLiveDebugTraceUVE(snapshot);
    ASSERT_TRUE(result.IsAcceptedUVE()) << result.message;
    ASSERT_TRUE(result.analysis.has_value());
    EXPECT_EQ(result.analysis->eventCount, 3U);
    EXPECT_DOUBLE_EQ(result.analysis->totalCost, 2.0);
    EXPECT_EQ(result.analysis->maximumCandidatesEvaluated, 4U);
    ASSERT_EQ(result.analysis->kindCounts.size(), 2U);
    EXPECT_EQ(result.analysis->kindCounts[0], (MotionQueryLiveDebugTraceKindCountUVE{"alpha", 1U}));
    EXPECT_EQ(result.analysis->kindCounts[1], (MotionQueryLiveDebugTraceKindCountUVE{"zeta", 2U}));

    snapshot.events[2].sequence = 2U;
    EXPECT_EQ(AnalyzeMotionQueryLiveDebugTraceUVE(snapshot).code,
              MotionQueryLiveDebugTraceAnalysisCodeUVE::InvalidTrace);

    snapshot.events.resize(kMotionQueryMaximumTraceEventsUVE + 1U);
    EXPECT_EQ(AnalyzeMotionQueryLiveDebugTraceUVE(snapshot).code,
              MotionQueryLiveDebugTraceAnalysisCodeUVE::CapacityExceeded);
}

TEST(MotionQueryDebuggingUVETest, LiveDebugSessionUVE_ExportsAndImportsTraceForOfflineInspection) {
    MotionQueryLiveDebugSessionUVE session;
    MotionQueryLiveDebugCommandUVE exportCommand;
    exportCommand.kind = MotionQueryLiveDebugCommandKindUVE::ExportTrace;
    const MotionQueryLiveDebugResponseUVE emptyExport = session.DispatchUVE(exportCommand, MotionQueryEditorAuthoringSessionUVE{});
    ASSERT_TRUE(emptyExport.applied) << emptyExport.message;
    ASSERT_TRUE(emptyExport.payload.has_value());

    MotionQueryLiveDebugCommandUVE importCommand;
    importCommand.kind = MotionQueryLiveDebugCommandKindUVE::ImportTrace;
    importCommand.expectedGeneration = session.GetSnapshotUVE().generation;
    importCommand.payload = *emptyExport.payload;
    const MotionQueryLiveDebugResponseUVE imported = session.DispatchUVE(importCommand, MotionQueryEditorAuthoringSessionUVE{});
    ASSERT_TRUE(imported.applied) << imported.message;
    EXPECT_FALSE(imported.snapshot.active);
    EXPECT_TRUE(imported.snapshot.traceEvents.empty());
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
