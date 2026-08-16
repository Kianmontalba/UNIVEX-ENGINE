// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_debugging_uve.h"
#include "uve/plugins/motion_query_animation_node_uve.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace UVE::Plugins::Editor {
namespace {
[[nodiscard]] MotionQueryTraceResultUVE MakeTraceResultUVE(
    MotionQueryTraceCodeUVE code, std::size_t index, const char* message) noexcept {
    return MotionQueryTraceResultUVE{code, index, message};
}

[[nodiscard]] bool IsValidHandleUVE(const UVE::Asset::ResourceHandleUVE handle) noexcept {
    return handle.guid.value != 0U && handle.generation != 0U;
}

[[nodiscard]] bool IsValidReviewUVE(const MotionQueryInventoryReviewUVE& review) noexcept {
    return !review.fileName.empty() && !review.rationale.empty();
}
} // namespace

MotionQueryTraceResultUVE MotionQueryTraceLoggerUVE::RecordUVE(
    MotionQueryTraceEventUVE event) noexcept {
    if (event.kind.empty() || event.kind.size() > kMotionQueryMaximumDebugMessageBytesUVE ||
        event.message.size() > kMotionQueryMaximumDebugMessageBytesUVE ||
        !std::isfinite(event.cost) || event.cost < 0.0F) {
        return MakeTraceResultUVE(MotionQueryTraceCodeUVE::InvalidEvent, snapshot_.events.size(),
                                  "motion query trace event payload is invalid");
    }
    if (!snapshot_.events.empty()) {
        const MotionQueryTraceEventUVE& previous = snapshot_.events.back();
        if (event.timestampNanoseconds < previous.timestampNanoseconds) {
            return MakeTraceResultUVE(MotionQueryTraceCodeUVE::NonMonotonicTimestamp,
                                      snapshot_.events.size(),
                                      "motion query trace timestamps must be monotonic");
        }
        if (event.frameNumber < previous.frameNumber) {
            return MakeTraceResultUVE(MotionQueryTraceCodeUVE::NonMonotonicFrame,
                                      snapshot_.events.size(),
                                      "motion query trace frame numbers must be monotonic");
        }
    }
    if (event.sequence == 0U) {
        event.sequence = nextSequence_++;
    } else if (event.sequence != nextSequence_) {
        return MakeTraceResultUVE(MotionQueryTraceCodeUVE::InvalidEvent,
                                  snapshot_.events.size(),
                                  "motion query trace sequence is not the next sequence");
    } else {
        ++nextSequence_;
    }
    if (event.database.has_value() && !IsValidHandleUVE(*event.database)) {
        return MakeTraceResultUVE(MotionQueryTraceCodeUVE::InvalidEvent,
                                  snapshot_.events.size(),
                                  "motion query trace database handle is invalid");
    }
    if (event.candidatesEvaluated > event.candidatesConsidered) {
        return MakeTraceResultUVE(MotionQueryTraceCodeUVE::InvalidEvent,
                                  snapshot_.events.size(),
                                  "motion query trace evaluated count exceeds considered count");
    }
    if (snapshot_.events.size() >= kMotionQueryMaximumTraceEventsUVE) {
        const auto victim = std::find_if(snapshot_.events.begin(), snapshot_.events.end(),
                                         [](const auto& entry) { return !entry.pinned; });
        if (victim != snapshot_.events.end()) {
            snapshot_.events.erase(victim);
            snapshot_.truncated = true;
        } else {
            // All events are pinned, we must discard the oldest pinned event or reject
            snapshot_.events.erase(snapshot_.events.begin());
            snapshot_.truncated = true;
        }
    }
    snapshot_.events.push_back(std::move(event));
    ++snapshot_.generation;
    return MakeTraceResultUVE(MotionQueryTraceCodeUVE::Accepted, snapshot_.events.size() - 1U,
                              "motion query trace event recorded");
}

MotionQueryTraceResultUVE MotionQueryTraceLoggerUVE::RemoveEventUVE(const std::uint64_t sequence) noexcept {
    const auto found = std::find_if(snapshot_.events.begin(), snapshot_.events.end(),
                                    [&](const auto& event) { return event.sequence == sequence; });
    if (found == snapshot_.events.end()) {
        return MakeTraceResultUVE(MotionQueryTraceCodeUVE::InvalidEvent, 0U,
                                  "motion query trace event sequence not found");
    }
    const std::size_t index = static_cast<std::size_t>(std::distance(snapshot_.events.begin(), found));
    snapshot_.events.erase(found);
    ++snapshot_.generation;
    return MakeTraceResultUVE(MotionQueryTraceCodeUVE::Accepted, index,
                              "motion query trace event removed");
}

MotionQueryTraceResultUVE MotionQueryTraceLoggerUVE::TogglePinUVE(const std::uint64_t sequence) noexcept {
    const auto found = std::find_if(snapshot_.events.begin(), snapshot_.events.end(),
                                    [&](const auto& event) { return event.sequence == sequence; });
    if (found == snapshot_.events.end()) {
        return MakeTraceResultUVE(MotionQueryTraceCodeUVE::InvalidEvent, 0U,
                                  "motion query trace event sequence not found");
    }
    found->pinned = !found->pinned;
    ++snapshot_.generation;
    return MakeTraceResultUVE(MotionQueryTraceCodeUVE::Accepted, 0U,
                              found->pinned ? "motion query trace event pinned" : "motion query trace event unpinned");
}

MotionQueryTraceResultUVE MotionQueryTraceLoggerUVE::SetCommentUVE(
    const std::uint64_t sequence, std::string comment) noexcept {
    const auto found = std::find_if(snapshot_.events.begin(), snapshot_.events.end(),
                                    [&](const auto& event) { return event.sequence == sequence; });
    if (found == snapshot_.events.end()) {
        return MakeTraceResultUVE(MotionQueryTraceCodeUVE::InvalidEvent, 0U,
                                  "motion query trace event sequence not found");
    }
    if (comment.size() > kMotionQueryMaximumDebugMessageBytesUVE) {
        comment.resize(kMotionQueryMaximumDebugMessageBytesUVE);
    }
    found->comment = std::move(comment);
    ++snapshot_.generation;
    return MakeTraceResultUVE(MotionQueryTraceCodeUVE::Accepted, 0U,
                              "motion query trace event comment updated");
}

MotionQueryTraceResultUVE MotionQueryTraceLoggerUVE::SetCategoryUVE(
    const std::uint64_t sequence, std::string category) noexcept {
    const auto found = std::find_if(snapshot_.events.begin(), snapshot_.events.end(),
                                    [&](const auto& event) { return event.sequence == sequence; });
    if (found == snapshot_.events.end()) {
        return MakeTraceResultUVE(MotionQueryTraceCodeUVE::InvalidEvent, 0U,
                                  "motion query trace event sequence not found");
    }
    if (category.size() > 32U) {
        category.resize(32U);
    }
    found->category = std::move(category);
    ++snapshot_.generation;
    return MakeTraceResultUVE(MotionQueryTraceCodeUVE::Accepted, 0U,
                              "motion query trace event category updated");
}

MotionQueryTraceResultUVE MotionQueryTraceLoggerUVE::RestoreUVE(
    MotionQueryTraceSnapshotUVE snapshot) noexcept {
    if (snapshot.events.size() > kMotionQueryMaximumTraceEventsUVE) {
        return MakeTraceResultUVE(MotionQueryTraceCodeUVE::CapacityExceeded, snapshot.events.size(),
                                  "motion query trace restore exceeds the event capacity");
    }
    std::uint64_t nextSequence = 1U;
    std::uint64_t previousSequence = 0U;
    std::uint64_t previousFrame = 0U;
    bool first = true;
    for (const auto& event : snapshot.events) {
        if (event.sequence == 0U || event.sequence <= previousSequence ||
            (!first && event.frameNumber < previousFrame) || event.kind.empty() ||
            event.kind.size() > kMotionQueryMaximumDebugMessageBytesUVE ||
            event.provenance.size() > kMotionQueryMaximumDebugMessageBytesUVE ||
            event.message.size() > kMotionQueryMaximumDebugMessageBytesUVE ||
            event.comment.size() > kMotionQueryMaximumDebugMessageBytesUVE ||
            event.category.size() > 32U || !std::isfinite(event.cost) || event.cost < 0.0F ||
            event.candidatesEvaluated > event.candidatesConsidered) {
            return MakeTraceResultUVE(MotionQueryTraceCodeUVE::InvalidEvent, snapshot.events.size(),
                                      "motion query trace restore contains an invalid event");
        }
        previousSequence = event.sequence;
        previousFrame = event.frameNumber;
        first = false;
        if (event.sequence == std::numeric_limits<std::uint64_t>::max()) {
            nextSequence = event.sequence;
        } else {
            nextSequence = event.sequence + 1U;
        }
    }
    snapshot.generation = snapshot_.generation + 1U;
    snapshot_ = std::move(snapshot);
    nextSequence_ = nextSequence;
    return MakeTraceResultUVE(MotionQueryTraceCodeUVE::Accepted, snapshot_.events.size(),
                              "motion query trace restored");
}

void MotionQueryTraceLoggerUVE::ClearUVE() noexcept {
    snapshot_.events.clear();
    snapshot_.truncated = false;
    ++snapshot_.generation;
    nextSequence_ = 1U;
}

void MotionQueryDebuggerUVE::AttachUVE(
    const UVE::Asset::ResourceHandleUVE database,
    const UVE::Core::MotionMatchingDatabaseUVE& value) noexcept {
    snapshot_ = MotionQueryDebuggerSnapshotUVE{};
    snapshot_.attached = IsValidHandleUVE(database);
    snapshot_.database = snapshot_.attached ? std::optional{database} : std::nullopt;
    snapshot_.candidateCount = value.candidates.size();
    snapshot_.generation++;
    candidateMetadata_.clear();
    candidateMetadata_.reserve(value.candidates.size());
    for (const auto& candidate : value.candidates) {
        candidateMetadata_.emplace_back(candidate.candidateId, candidate.sourceClipId);
    }
    snapshot_.message = snapshot_.attached ? "motion query debugger attached" :
                                               "motion query debugger rejected invalid database handle";
}

void MotionQueryDebuggerUVE::DetachUVE() noexcept {
    snapshot_ = MotionQueryDebuggerSnapshotUVE{};
    candidateMetadata_.clear();
}

void MotionQueryDebuggerUVE::PublishMatchUVE(
    const std::size_t candidateIndex, const std::size_t candidatesEvaluated,
    const float cost, const std::string_view message) noexcept {
    if (!snapshot_.attached || candidateIndex >= candidateMetadata_.size() ||
        candidatesEvaluated > snapshot_.candidateCount || !std::isfinite(cost) || cost < 0.0F) {
        snapshot_.message = "motion query debugger rejected invalid match snapshot";
        return;
    }
    snapshot_.selectedCandidateIndex = candidateIndex;
    snapshot_.candidatesEvaluated = candidatesEvaluated;
    snapshot_.selectedCost = cost;
    snapshot_.selectedCandidateId = candidateMetadata_[candidateIndex].first;
    snapshot_.selectedSourceClipId = candidateMetadata_[candidateIndex].second;
    snapshot_.message.assign(message.data(), message.size());
    if (snapshot_.message.size() > kMotionQueryMaximumDebugMessageBytesUVE) {
        snapshot_.message.resize(kMotionQueryMaximumDebugMessageBytesUVE);
    }
    ++snapshot_.generation;
}

void MotionQueryDebuggerUVE::PublishMatchUVE(
    const UVE::Plugins::MotionQueryAnimationNodeResultUVE& result) noexcept {
    PublishMatchUVE(result.candidateIndex, result.candidatesEvaluated, result.cost, result.message);
    if (!snapshot_.attached || !snapshot_.selectedCandidateIndex.has_value()) {
        return;
    }
    snapshot_.qualityTier = static_cast<std::uint8_t>(result.qualityTier);
    snapshot_.continuityCode = static_cast<std::uint8_t>(result.continuityCode);
    snapshot_.continuityApplied = result.continuityApplied;
    snapshot_.transitionCode = static_cast<std::uint8_t>(result.transitionCode);
    snapshot_.transitionHeldPrevious = result.transitionHeldPrevious;
    snapshot_.telemetryCode = static_cast<std::uint8_t>(result.telemetryCode);
    snapshot_.telemetryIndexEntryCount = result.telemetryIndexEntryCount;
    snapshot_.telemetryCandidatesConsidered = result.telemetryCandidatesConsidered;
    snapshot_.telemetryBudgetSaturated = result.telemetryBudgetSaturated;
    snapshot_.provenance = result.transitionHeldPrevious ? "history_hold" :
                           result.continuityApplied ? "continuity_applied" : "runtime_search";
}

void MotionQueryDebuggerUVE::InspectEventUVE(const MotionQueryTraceEventUVE& event) noexcept {
    snapshot_.selectedCandidateIndex = event.selectedCandidateIndex;
    snapshot_.candidatesEvaluated = event.candidatesEvaluated;
    snapshot_.selectedCost = event.cost;
    snapshot_.selectedCandidateId = ""; // Metadata not available in trace event
    snapshot_.selectedSourceClipId = "";
    snapshot_.message = event.message;
    snapshot_.qualityTier = event.qualityTier;
    snapshot_.continuityCode = event.continuityCode;
    snapshot_.continuityApplied = event.continuityApplied;
    snapshot_.transitionCode = event.transitionCode;
    snapshot_.transitionHeldPrevious = event.transitionHeldPrevious;
    snapshot_.telemetryCode = event.telemetryCode;
    snapshot_.telemetryIndexEntryCount = event.telemetryIndexEntryCount;
    snapshot_.telemetryCandidatesConsidered = event.telemetryCandidatesConsidered;
    snapshot_.telemetryBudgetSaturated = event.telemetryBudgetSaturated;
    snapshot_.provenance = event.provenance;
    snapshot_.database = event.database;
    if (snapshot_.message.size() > kMotionQueryMaximumDebugMessageBytesUVE) {
        snapshot_.message.resize(kMotionQueryMaximumDebugMessageBytesUVE);
    }
    ++snapshot_.generation;
}

UVE::Core::DiagnosticCaptureResultUVE MotionQueryTraceProfilerAdapterUVE::RecordMatchUVE(
    UVE::Core::ProfilerCaptureUVE& profiler, const MotionQueryTraceEventUVE& event) const {
    const std::string spanName = event.kind.empty() ? "motion_query" : event.kind;
    return profiler.RecordSpanUVE(UVE::Core::DiagnosticCategoryUVE::Animation, spanName,
                                  event.timestampNanoseconds, event.timestampNanoseconds,
                                  0U, event.frameNumber);
}

MotionQueryAutomatedValidationReportUVE ValidateMotionQueryRuntimeUVE(
    const std::vector<UVE::Core::MotionQueryDatabaseContractUVE>& databases,
    const std::vector<MotionQuerySearchIndexUVE>& indices,
    const std::vector<MotionQueryInventoryReviewUVE>& inventoryReviews) {
    MotionQueryAutomatedValidationReportUVE report;
    report.checkedDatabases = databases.size();
    report.checkedInventoryFiles = inventoryReviews.size();
    report.inventoryReviews = inventoryReviews;
    if (databases.size() != indices.size()) {
        report.diagnostics.push_back("database and search-index counts differ");
    }
    for (std::size_t index = 0U; index < databases.size(); ++index) {
        const auto validation = UVE::Core::ValidateMotionQueryDatabaseContractUVE(databases[index]);
        if (!validation.IsValidUVE()) {
            report.diagnostics.push_back("database " + std::to_string(index) + ": " + validation.message);
            continue;
        }
        report.checkedCandidates += databases[index].database.candidates.size();
        if (index >= indices.size() || !indices[index].IsBuiltUVE() ||
            indices[index].SizeUVE() != databases[index].database.candidates.size()) {
            report.diagnostics.push_back("database " + std::to_string(index) +
                                         ": search index is missing or has a candidate count mismatch");
        }
    }
    for (std::size_t index = 0U; index < inventoryReviews.size(); ++index) {
        if (!IsValidReviewUVE(inventoryReviews[index])) {
            report.diagnostics.push_back("inventory review " + std::to_string(index) +
                                         ": disposition requires file name and rationale");
        }
    }
    report.valid = report.diagnostics.empty();
    return report;
}

} // namespace UVE::Plugins::Editor
