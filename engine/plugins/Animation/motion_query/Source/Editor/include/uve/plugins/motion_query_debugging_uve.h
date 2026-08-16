// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/asset/resource_dependency_graph_uve.h"
#include "uve/plugins/motion_query_database_contract_uve.h"
#include "uve/core/profiler_diagnostics_uve.h"
#include "uve/plugins/motion_query_search_index_uve.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace UVE::Plugins {
struct MotionQueryAnimationNodeResultUVE;
}

namespace UVE::Plugins::Editor {

inline constexpr std::size_t kMotionQueryMaximumTraceEventsUVE = 512U;
inline constexpr std::size_t kMotionQueryMaximumDebugMessageBytesUVE = 256U;

struct MotionQueryTraceEventUVE final {
    std::uint64_t sequence = 0U;
    std::uint64_t timestampNanoseconds = 0U;
    std::uint64_t frameNumber = 0U;
    std::string kind;
    std::optional<UVE::Asset::ResourceHandleUVE> database;
    std::size_t candidatesConsidered = 0U;
    std::size_t candidatesEvaluated = 0U;
    float cost = 0.0F;
    std::optional<std::size_t> selectedCandidateIndex;
    std::uint8_t qualityTier = 0U;
    std::uint8_t continuityCode = 0U;
    bool continuityApplied = false;
    std::uint8_t transitionCode = 0U;
    bool transitionHeldPrevious = false;
    std::uint8_t telemetryCode = 0U;
    std::size_t telemetryIndexEntryCount = 0U;
    std::size_t telemetryCandidatesConsidered = 0U;
    bool telemetryBudgetSaturated = false;
    std::string provenance;
    std::string message;
    std::string comment;
    std::string category;
    bool pinned = false;

    [[nodiscard]] bool operator==(const MotionQueryTraceEventUVE&) const = default;
};

struct MotionQueryTraceSnapshotUVE final {
    std::uint64_t generation = 0U;
    bool truncated = false;
    std::vector<MotionQueryTraceEventUVE> events;

    [[nodiscard]] bool operator==(const MotionQueryTraceSnapshotUVE&) const = default;
};

enum class MotionQueryTraceCodeUVE : std::uint8_t {
    Accepted = 0,
    InvalidEvent,
    CapacityExceeded,
    NonMonotonicTimestamp,
    NonMonotonicFrame,
};

struct MotionQueryTraceResultUVE final {
    MotionQueryTraceCodeUVE code = MotionQueryTraceCodeUVE::InvalidEvent;
    std::size_t index = 0U;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == MotionQueryTraceCodeUVE::Accepted;
    }
};

class MotionQueryTraceLoggerUVE final {
public:
    [[nodiscard]] MotionQueryTraceResultUVE RecordUVE(
        MotionQueryTraceEventUVE event) noexcept;
    [[nodiscard]] MotionQueryTraceResultUVE RemoveEventUVE(std::uint64_t sequence) noexcept;
    [[nodiscard]] MotionQueryTraceResultUVE TogglePinUVE(std::uint64_t sequence) noexcept;
    [[nodiscard]] MotionQueryTraceResultUVE SetCommentUVE(std::uint64_t sequence, std::string comment) noexcept;
    [[nodiscard]] MotionQueryTraceResultUVE SetCategoryUVE(std::uint64_t sequence, std::string category) noexcept;
    [[nodiscard]] MotionQueryTraceResultUVE RestoreUVE(MotionQueryTraceSnapshotUVE snapshot) noexcept;
    void ClearUVE() noexcept;

    [[nodiscard]] MotionQueryTraceSnapshotUVE GetSnapshotUVE() const noexcept {
        return snapshot_;
    }

private:
    MotionQueryTraceSnapshotUVE snapshot_;
    std::uint64_t nextSequence_ = 1U;
};

struct MotionQueryDebuggerSnapshotUVE final {
    bool attached = false;
    std::uint64_t generation = 0U;
    std::optional<UVE::Asset::ResourceHandleUVE> database;
    std::optional<std::size_t> selectedCandidateIndex;
    std::size_t candidateCount = 0U;
    std::size_t candidatesEvaluated = 0U;
    float selectedCost = 0.0F;
    std::string selectedCandidateId;
    std::string selectedSourceClipId;
    std::uint8_t qualityTier = 0U;
    std::uint8_t continuityCode = 0U;
    bool continuityApplied = false;
    std::uint8_t transitionCode = 0U;
    bool transitionHeldPrevious = false;
    std::uint8_t telemetryCode = 0U;
    std::size_t telemetryIndexEntryCount = 0U;
    std::size_t telemetryCandidatesConsidered = 0U;
    bool telemetryBudgetSaturated = false;
    std::string provenance;
    std::string message;

    [[nodiscard]] bool operator==(const MotionQueryDebuggerSnapshotUVE&) const = default;
};

class MotionQueryDebuggerUVE final {
public:
    void AttachUVE(UVE::Asset::ResourceHandleUVE database,
                   const UVE::Core::MotionMatchingDatabaseUVE& value) noexcept;
    void DetachUVE() noexcept;
    void PublishMatchUVE(std::size_t candidateIndex, std::size_t candidatesEvaluated,
                         float cost, std::string_view message) noexcept;
    void PublishMatchUVE(const UVE::Plugins::MotionQueryAnimationNodeResultUVE& result) noexcept;
    void InspectEventUVE(const MotionQueryTraceEventUVE& event) noexcept;

    [[nodiscard]] MotionQueryDebuggerSnapshotUVE GetSnapshotUVE() const noexcept {
        return snapshot_;
    }

private:
    MotionQueryDebuggerSnapshotUVE snapshot_;
    std::vector<std::pair<std::string, std::string>> candidateMetadata_;
};

class MotionQueryTraceProfilerAdapterUVE final {
public:
    [[nodiscard]] UVE::Core::DiagnosticCaptureResultUVE RecordMatchUVE(
        UVE::Core::ProfilerCaptureUVE& profiler, const MotionQueryTraceEventUVE& event) const;
};

enum class MotionQueryInventoryDispositionUVE : std::uint8_t {
    Implemented = 0,
    RewrittenNative,
    RejectedUnrealSpecific,
    Deferred,
};

struct MotionQueryInventoryReviewUVE final {
    std::string fileName;
    MotionQueryInventoryDispositionUVE disposition = MotionQueryInventoryDispositionUVE::Deferred;
    std::string rationale;

    [[nodiscard]] bool operator==(const MotionQueryInventoryReviewUVE&) const = default;
};

struct MotionQueryAutomatedValidationReportUVE final {
    bool valid = false;
    std::size_t checkedDatabases = 0U;
    std::size_t checkedCandidates = 0U;
    std::size_t checkedInventoryFiles = 0U;
    std::vector<std::string> diagnostics;
    std::vector<MotionQueryInventoryReviewUVE> inventoryReviews;
};

[[nodiscard]] MotionQueryAutomatedValidationReportUVE ValidateMotionQueryRuntimeUVE(
    const std::vector<UVE::Core::MotionQueryDatabaseContractUVE>& databases,
    const std::vector<MotionQuerySearchIndexUVE>& indices,
    const std::vector<MotionQueryInventoryReviewUVE>& inventoryReviews);

} // namespace UVE::Plugins::Editor
