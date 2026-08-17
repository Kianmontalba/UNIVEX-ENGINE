// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_live_debug_session_uve.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace UVE::Plugins::Editor {
namespace {
[[nodiscard]] const char* CodeMessageUVE(const MotionQueryLiveDebugResponseCodeUVE code) noexcept {
    switch (code) {
        case MotionQueryLiveDebugResponseCodeUVE::Applied: return "motion query live debug command applied";
        case MotionQueryLiveDebugResponseCodeUVE::InvalidProtocol: return "motion query live debug protocol is invalid";
        case MotionQueryLiveDebugResponseCodeUVE::StaleGeneration: return "motion query live debug generation is stale";
        case MotionQueryLiveDebugResponseCodeUVE::InvalidCommand: return "motion query live debug command is invalid";
        case MotionQueryLiveDebugResponseCodeUVE::InvalidDatabase: return "motion query live debug database is invalid";
        case MotionQueryLiveDebugResponseCodeUVE::DatabaseNotFound: return "motion query live debug database was not found";
        case MotionQueryLiveDebugResponseCodeUVE::InvalidFilter: return "motion query live debug filter is invalid";
    }
    return "motion query live debug command failed";
}
} // namespace

MotionQueryLiveDebugResponseUVE MotionQueryLiveDebugSessionUVE::DispatchUVE(
    const MotionQueryLiveDebugCommandUVE& command,
    const MotionQueryEditorAuthoringSessionUVE& authoring) noexcept {
    if (command.protocolVersion != kMotionQueryEditorProtocolVersionUVE) {
        return MakeResponseUVE(command, false, MotionQueryLiveDebugResponseCodeUVE::InvalidProtocol,
                               "motion query live debug protocol version is unsupported");
    }
    if (command.kind != MotionQueryLiveDebugCommandKindUVE::ReadSnapshot &&
        command.kind != MotionQueryLiveDebugCommandKindUVE::ExportTrace &&
        command.expectedGeneration != generation_) {
        return MakeResponseUVE(command, false, MotionQueryLiveDebugResponseCodeUVE::StaleGeneration,
                               "motion query live debug command was based on an older session generation");
    }

    switch (command.kind) {
        case MotionQueryLiveDebugCommandKindUVE::ReadSnapshot:
            return MakeResponseUVE(command, true, MotionQueryLiveDebugResponseCodeUVE::Applied,
                                   "motion query live debug snapshot copied");
        case MotionQueryLiveDebugCommandKindUVE::Attach: {
            if (!command.database.has_value() || !IsValidHandleUVE(*command.database)) {
                return MakeResponseUVE(command, false, MotionQueryLiveDebugResponseCodeUVE::InvalidDatabase,
                                       "attach requires a valid Motion Query resource handle");
            }
            UVE::Core::MotionQueryDatabaseContractUVE contract;
            if (!authoring.TryGetDatabaseCopyUVE(*command.database, contract)) {
                return MakeResponseUVE(command, false, MotionQueryLiveDebugResponseCodeUVE::DatabaseNotFound,
                                       "attach database is not present in the native authoring session");
            }
            const UVE::Core::MotionQueryDatabaseContractResultUVE validation =
                UVE::Core::ValidateMotionQueryDatabaseContractUVE(contract);
            if (!validation.IsValidUVE()) {
                return MakeResponseUVE(command, false, MotionQueryLiveDebugResponseCodeUVE::InvalidDatabase,
                                       validation.message);
            }
            debugger_.AttachUVE(*command.database, contract.database);
            trace_.ClearUVE();
            database_ = *command.database;
            filter_.clear();
            active_ = true;
            ++generation_;
            return MakeResponseUVE(command, true, MotionQueryLiveDebugResponseCodeUVE::Applied,
                                   "motion query live debug session attached");
        }
        case MotionQueryLiveDebugCommandKindUVE::Detach:
            debugger_.DetachUVE();
            trace_.ClearUVE();
            database_.reset();
            filter_.clear();
            active_ = false;
            ++generation_;
            return MakeResponseUVE(command, true, MotionQueryLiveDebugResponseCodeUVE::Applied,
                                   "motion query live debug session detached");
        case MotionQueryLiveDebugCommandKindUVE::ClearTrace:
            trace_.ClearUVE();
            ++generation_;
            return MakeResponseUVE(command, true, MotionQueryLiveDebugResponseCodeUVE::Applied,
                                   "motion query live debug trace cleared");
        case MotionQueryLiveDebugCommandKindUVE::ClearSession:
            ClearUVE();
            return MakeResponseUVE(command, true, MotionQueryLiveDebugResponseCodeUVE::Applied,
                                   "motion query live debug session cleared");
        case MotionQueryLiveDebugCommandKindUVE::SetFilter:
            if (command.filter.size() > kMotionQueryMaximumDebugMessageBytesUVE) {
                return MakeResponseUVE(command, false, MotionQueryLiveDebugResponseCodeUVE::InvalidFilter,
                                       "motion query live debug filter exceeds the bounded message size");
            }
            filter_ = command.filter;
            ++generation_;
            return MakeResponseUVE(command, true, MotionQueryLiveDebugResponseCodeUVE::Applied,
                                   "motion query live debug trace filter updated");
        case MotionQueryLiveDebugCommandKindUVE::SelectEvent: {
            if (!command.eventSequence.has_value()) {
                return MakeResponseUVE(command, false, MotionQueryLiveDebugResponseCodeUVE::InvalidCommand,
                                       "live debug SelectEvent requires an event sequence");
            }
            const auto traceSnapshot = trace_.GetSnapshotUVE();
            const auto found = std::find_if(traceSnapshot.events.begin(), traceSnapshot.events.end(),
                                            [&](const auto& event) {
                                                return event.sequence == *command.eventSequence;
                                            });
            if (found == traceSnapshot.events.end()) {
                return MakeResponseUVE(command, false, MotionQueryLiveDebugResponseCodeUVE::InvalidCommand,
                                       "live debug event sequence not found in trace");
            }
            debugger_.InspectEventUVE(*found);
            ++generation_;
            return MakeResponseUVE(command, true, MotionQueryLiveDebugResponseCodeUVE::Applied,
                                   "live debug event selected for inspection");
        }
        case MotionQueryLiveDebugCommandKindUVE::RemoveEvent: {
            if (!command.eventSequence.has_value()) {
                return MakeResponseUVE(command, false, MotionQueryLiveDebugResponseCodeUVE::InvalidCommand,
                                       "live debug RemoveEvent requires an event sequence");
            }
            const auto result = trace_.RemoveEventUVE(*command.eventSequence);
            if (!result.IsAcceptedUVE()) {
                return MakeResponseUVE(command, false, MotionQueryLiveDebugResponseCodeUVE::InvalidCommand,
                                       result.message);
            }
            ++generation_;
            return MakeResponseUVE(command, true, MotionQueryLiveDebugResponseCodeUVE::Applied,
                                   "live debug event removed from trace");
        }
        case MotionQueryLiveDebugCommandKindUVE::ToggleTraceEventPin: {
            if (!command.eventSequence.has_value()) {
                return MakeResponseUVE(command, false, MotionQueryLiveDebugResponseCodeUVE::InvalidCommand,
                                       "live debug ToggleTraceEventPin requires an event sequence");
            }
            const auto result = trace_.TogglePinUVE(*command.eventSequence);
            if (!result.IsAcceptedUVE()) {
                return MakeResponseUVE(command, false, MotionQueryLiveDebugResponseCodeUVE::InvalidCommand,
                                       result.message);
            }
            ++generation_;
            return MakeResponseUVE(command, true, MotionQueryLiveDebugResponseCodeUVE::Applied,
                                   result.message);
        }
        case MotionQueryLiveDebugCommandKindUVE::SetTraceEventComment: {
            if (!command.eventSequence.has_value()) {
                return MakeResponseUVE(command, false, MotionQueryLiveDebugResponseCodeUVE::InvalidCommand,
                                       "live debug SetTraceEventComment requires an event sequence");
            }
            const auto result = trace_.SetCommentUVE(*command.eventSequence, command.filter); // Reusing filter field for comment
            if (!result.IsAcceptedUVE()) {
                return MakeResponseUVE(command, false, MotionQueryLiveDebugResponseCodeUVE::InvalidCommand,
                                       result.message);
            }
            ++generation_;
            return MakeResponseUVE(command, true, MotionQueryLiveDebugResponseCodeUVE::Applied,
                                   "live debug event comment updated");
        }
        case MotionQueryLiveDebugCommandKindUVE::SetTraceEventCategory: {
            if (!command.eventSequence.has_value()) {
                return MakeResponseUVE(command, false, MotionQueryLiveDebugResponseCodeUVE::InvalidCommand,
                                       "live debug SetTraceEventCategory requires an event sequence");
            }
            const auto result = trace_.SetCategoryUVE(*command.eventSequence, command.filter); // Reusing filter field for category
            if (!result.IsAcceptedUVE()) {
                return MakeResponseUVE(command, false, MotionQueryLiveDebugResponseCodeUVE::InvalidCommand,
                                       result.message);
            }
            ++generation_;
            return MakeResponseUVE(command, true, MotionQueryLiveDebugResponseCodeUVE::Applied,
                                   "live debug event category updated");
        }
        case MotionQueryLiveDebugCommandKindUVE::ExportTrace: {
            const auto result = SerializeMotionQueryLiveDebugTraceUVE(trace_.GetSnapshotUVE(), filter_);
            if (!result.IsAcceptedUVE()) {
                return MakeResponseUVE(command, false, MotionQueryLiveDebugResponseCodeUVE::InvalidCommand,
                                       result.message);
            }
            MotionQueryLiveDebugResponseUVE response = MakeResponseUVE(
                command, true, MotionQueryLiveDebugResponseCodeUVE::Applied,
                "motion query live debug trace exported");
            response.payload = result.payload;
            return response;
        }
        case MotionQueryLiveDebugCommandKindUVE::ImportTrace: {
            const auto result = DeserializeMotionQueryLiveDebugTraceUVE(command.payload);
            if (!result.IsAcceptedUVE()) {
                return MakeResponseUVE(command, false, MotionQueryLiveDebugResponseCodeUVE::InvalidCommand,
                                       result.message);
            }
            MotionQueryTraceSnapshotUVE restoredSnapshot;
            restoredSnapshot.truncated = result.envelope->truncated;
            restoredSnapshot.events = result.envelope->events;
            const auto restoreResult = trace_.RestoreUVE(std::move(restoredSnapshot));
            if (!restoreResult.IsAcceptedUVE()) {
                return MakeResponseUVE(command, false, MotionQueryLiveDebugResponseCodeUVE::InvalidCommand,
                                       restoreResult.message);
            }
            debugger_.DetachUVE();
            database_.reset();
            filter_ = result.envelope->filter;
            active_ = false;
            ++generation_;
            return MakeResponseUVE(command, true, MotionQueryLiveDebugResponseCodeUVE::Applied,
                                   "motion query live debug trace imported for offline inspection");
        }
    }
    return MakeResponseUVE(command, false, MotionQueryLiveDebugResponseCodeUVE::InvalidCommand,
                           CodeMessageUVE(MotionQueryLiveDebugResponseCodeUVE::InvalidCommand));
}

void MotionQueryLiveDebugSessionUVE::PublishUVE(
    const UVE::Plugins::MotionQueryAnimationNodeResultUVE& result,
    const std::uint64_t timestampNanoseconds,
    const std::uint64_t frameNumber) noexcept {
    if (!active_) {
        return;
    }
    if (result.IsAcceptedUVE() || result.code == UVE::Plugins::MotionQueryAnimationNodeCodeUVE::MissingClip ||
        result.code == UVE::Plugins::MotionQueryAnimationNodeCodeUVE::PoseSamplingFailed) {
        debugger_.PublishMatchUVE(result);
    }
    MotionQueryTraceEventUVE event;
    event.timestampNanoseconds = timestampNanoseconds;
    event.frameNumber = frameNumber;
    event.kind = ResultKindUVE(result.code);
    event.database = database_;
    event.candidatesConsidered = result.candidatesEvaluated;
    event.candidatesEvaluated = result.candidatesEvaluated;
    event.cost = std::isfinite(result.cost) && result.cost >= 0.0F ? result.cost : 0.0F;
    if (result.IsAcceptedUVE() || result.code == UVE::Plugins::MotionQueryAnimationNodeCodeUVE::MissingClip ||
        result.code == UVE::Plugins::MotionQueryAnimationNodeCodeUVE::PoseSamplingFailed) {
        event.selectedCandidateIndex = result.candidateIndex;
    }
    event.qualityTier = static_cast<std::uint8_t>(result.qualityTier);
    event.continuityCode = static_cast<std::uint8_t>(result.continuityCode);
    event.continuityApplied = result.continuityApplied;
    event.transitionCode = static_cast<std::uint8_t>(result.transitionCode);
    event.transitionHeldPrevious = result.transitionHeldPrevious;
    event.telemetryCode = static_cast<std::uint8_t>(result.telemetryCode);
    event.telemetryIndexEntryCount = result.telemetryIndexEntryCount;
    event.telemetryCandidatesConsidered = result.telemetryCandidatesConsidered;
    event.telemetryBudgetSaturated = result.telemetryBudgetSaturated;
    event.provenance = result.transitionHeldPrevious ? "history_hold" :
                       result.continuityApplied ? "continuity_applied" : "runtime_search";
    event.message = result.message;
    static_cast<void>(trace_.RecordUVE(std::move(event)));
}

void MotionQueryLiveDebugSessionUVE::ClearUVE() noexcept {
    debugger_.DetachUVE();
    trace_.ClearUVE();
    database_.reset();
    filter_.clear();
    active_ = false;
    ++generation_;
}

MotionQueryLiveDebugSnapshotUVE MotionQueryLiveDebugSessionUVE::GetSnapshotUVE() const noexcept {
    MotionQueryLiveDebugSnapshotUVE snapshot;
    snapshot.generation = generation_;
    snapshot.active = active_;
    snapshot.database = database_;
    snapshot.filter = filter_;
    snapshot.debugger = debugger_.GetSnapshotUVE();
    const MotionQueryTraceSnapshotUVE trace = trace_.GetSnapshotUVE();
    snapshot.totalTraceEventCount = trace.events.size();
    snapshot.traceTruncated = trace.truncated;
    snapshot.traceEvents.reserve(trace.events.size());
    for (const MotionQueryTraceEventUVE& event : trace.events) {
        if (!ContainsFilterUVE(event, filter_)) {
            continue;
        }
        snapshot.traceEvents.push_back(event);
    }
    snapshot.visibleTraceEventCount = snapshot.traceEvents.size();
    snapshot.diagnostic = active_ ? "native Motion Query live debug session is active"
                                  : "native Motion Query live debug session is detached";
    return snapshot;
}

MotionQueryLiveDebugResponseUVE MotionQueryLiveDebugSessionUVE::MakeResponseUVE(
    const MotionQueryLiveDebugCommandUVE& command, const bool applied,
    const MotionQueryLiveDebugResponseCodeUVE code, std::string message) const noexcept {
    MotionQueryLiveDebugResponseUVE response;
    response.requestId = command.requestId;
    response.applied = applied;
    response.code = code;
    response.message = std::move(message);
    response.snapshot = GetSnapshotUVE();
    return response;
}

bool MotionQueryLiveDebugSessionUVE::IsValidHandleUVE(
    const UVE::Asset::ResourceHandleUVE resource) noexcept {
    return resource.guid.value != 0U && resource.generation != 0U;
}

bool MotionQueryLiveDebugSessionUVE::ContainsFilterUVE(
    const MotionQueryTraceEventUVE& event, const std::string& filter) noexcept {
    return filter.empty() || event.kind.find(filter) != std::string::npos ||
           event.message.find(filter) != std::string::npos ||
           event.category.find(filter) != std::string::npos ||
           event.comment.find(filter) != std::string::npos;
}

const char* MotionQueryLiveDebugSessionUVE::ResultKindUVE(
    const UVE::Plugins::MotionQueryAnimationNodeCodeUVE code) noexcept {
    switch (code) {
        case UVE::Plugins::MotionQueryAnimationNodeCodeUVE::Accepted: return "accepted";
        case UVE::Plugins::MotionQueryAnimationNodeCodeUVE::InvalidSettings: return "invalid_settings";
        case UVE::Plugins::MotionQueryAnimationNodeCodeUVE::InvalidQuery: return "invalid_query";
        case UVE::Plugins::MotionQueryAnimationNodeCodeUVE::InvalidDatabase: return "invalid_database";
        case UVE::Plugins::MotionQueryAnimationNodeCodeUVE::InvalidSchema: return "invalid_schema";
        case UVE::Plugins::MotionQueryAnimationNodeCodeUVE::SchemaMismatch: return "schema_mismatch";
        case UVE::Plugins::MotionQueryAnimationNodeCodeUVE::ContinuityFailed: return "continuity_failed";
        case UVE::Plugins::MotionQueryAnimationNodeCodeUVE::IndexNotBuilt: return "index_not_built";
        case UVE::Plugins::MotionQueryAnimationNodeCodeUVE::SearchFailed: return "search_failed";
        case UVE::Plugins::MotionQueryAnimationNodeCodeUVE::NoMatch: return "no_match";
        case UVE::Plugins::MotionQueryAnimationNodeCodeUVE::CandidateIndexOutOfRange: return "candidate_index_out_of_range";
        case UVE::Plugins::MotionQueryAnimationNodeCodeUVE::MissingClip: return "missing_clip";
        case UVE::Plugins::MotionQueryAnimationNodeCodeUVE::PoseSamplingFailed: return "pose_sampling_failed";
        case UVE::Plugins::MotionQueryAnimationNodeCodeUVE::InvalidEvaluationTime: return "invalid_evaluation_time";
        case UVE::Plugins::MotionQueryAnimationNodeCodeUVE::NoHistoryFrame: return "no_history_frame";
    }
    return "unknown";
}

} // namespace UVE::Plugins::Editor
