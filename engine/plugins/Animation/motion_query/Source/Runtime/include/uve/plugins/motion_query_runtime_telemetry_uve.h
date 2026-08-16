// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/plugins/motion_query_lod_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace UVE::Plugins {

enum class MotionQueryRuntimeTelemetryCodeUVE : std::uint8_t {
    Accepted = 0,
    InvalidCounters,
};

struct MotionQueryRuntimeTelemetryUVE final {
    static constexpr std::size_t kMaximumCounterUVE = MotionQuerySearchBudgetUVE::kMaximumResultsUVE;

    MotionQueryRuntimeTelemetryCodeUVE code = MotionQueryRuntimeTelemetryCodeUVE::InvalidCounters;
    std::size_t indexEntryCount = 0U;
    std::size_t candidatesConsidered = 0U;
    std::size_t candidatesEvaluated = 0U;
    std::size_t requestedSearchResults = 0U;
    std::size_t effectiveSearchResults = 0U;
    bool searchBudgetDowngraded = false;
    bool searchBudgetSaturated = false;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == MotionQueryRuntimeTelemetryCodeUVE::Accepted;
    }
};

[[nodiscard]] MotionQueryRuntimeTelemetryUVE BuildMotionQueryRuntimeTelemetryUVE(
    std::size_t indexEntryCount, const MotionQuerySearchBudgetUVE& budget,
    std::size_t candidatesConsidered, std::size_t candidatesEvaluated) noexcept;

} // namespace UVE::Plugins
