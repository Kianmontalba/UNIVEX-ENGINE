// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_runtime_telemetry_uve.h"

namespace UVE::Plugins {
namespace {
[[nodiscard]] MotionQueryRuntimeTelemetryUVE MakeTelemetryUVE(
    const MotionQueryRuntimeTelemetryCodeUVE code,
    const char* message) noexcept {
    MotionQueryRuntimeTelemetryUVE telemetry;
    telemetry.code = code;
    telemetry.message = message;
    return telemetry;
}
}

MotionQueryRuntimeTelemetryUVE BuildMotionQueryRuntimeTelemetryUVE(
    const std::size_t indexEntryCount, const MotionQuerySearchBudgetUVE& budget,
    const std::size_t candidatesConsidered, const std::size_t candidatesEvaluated) noexcept {
    if (indexEntryCount > MotionQueryRuntimeTelemetryUVE::kMaximumCounterUVE ||
        candidatesConsidered > MotionQueryRuntimeTelemetryUVE::kMaximumCounterUVE ||
        candidatesEvaluated > MotionQueryRuntimeTelemetryUVE::kMaximumCounterUVE ||
        candidatesEvaluated > candidatesConsidered || budget.requestedResults == 0U ||
        budget.requestedResults > MotionQueryRuntimeTelemetryUVE::kMaximumCounterUVE ||
        budget.effectiveResults == 0U ||
        budget.effectiveResults > MotionQueryRuntimeTelemetryUVE::kMaximumCounterUVE ||
        budget.effectiveResults > budget.requestedResults) {
        return MakeTelemetryUVE(MotionQueryRuntimeTelemetryCodeUVE::InvalidCounters,
                                "motion query runtime telemetry counters are invalid");
    }
    MotionQueryRuntimeTelemetryUVE telemetry;
    telemetry.code = MotionQueryRuntimeTelemetryCodeUVE::Accepted;
    telemetry.indexEntryCount = indexEntryCount;
    telemetry.candidatesConsidered = candidatesConsidered;
    telemetry.candidatesEvaluated = candidatesEvaluated;
    telemetry.requestedSearchResults = budget.requestedResults;
    telemetry.effectiveSearchResults = budget.effectiveResults;
    telemetry.searchBudgetDowngraded = budget.WasDowngradedUVE();
    telemetry.searchBudgetSaturated = budget.effectiveResults < budget.requestedResults;
    telemetry.message = telemetry.searchBudgetSaturated
                            ? "motion query runtime telemetry recorded a saturated search budget"
                            : "motion query runtime telemetry recorded full search budget";
    return telemetry;
}

} // namespace UVE::Plugins
