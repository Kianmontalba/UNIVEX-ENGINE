// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_runtime_telemetry_uve.h"

#include <gtest/gtest.h>

namespace UVE::Plugins {

TEST(MotionQueryRuntimeTelemetryUVETest, FullBudgetUVE_ReportsDeterministicCounters) {
    MotionQuerySearchBudgetUVE budget;
    budget.code = MotionQuerySearchBudgetCodeUVE::Accepted;
    budget.requestedResults = 32U;
    budget.effectiveResults = 32U;
    const auto telemetry = BuildMotionQueryRuntimeTelemetryUVE(128U, budget, 16U, 8U);
    ASSERT_TRUE(telemetry.IsAcceptedUVE());
    EXPECT_EQ(telemetry.indexEntryCount, 128U);
    EXPECT_EQ(telemetry.candidatesConsidered, 16U);
    EXPECT_EQ(telemetry.candidatesEvaluated, 8U);
    EXPECT_FALSE(telemetry.searchBudgetDowngraded);
    EXPECT_FALSE(telemetry.searchBudgetSaturated);
}

TEST(MotionQueryRuntimeTelemetryUVETest, DowngradedBudgetUVE_ReportsSaturationWithoutTiming) {
    MotionQuerySearchBudgetUVE budget;
    budget.code = MotionQuerySearchBudgetCodeUVE::Downgraded;
    budget.requestedResults = 32U;
    budget.effectiveResults = 8U;
    const auto telemetry = BuildMotionQueryRuntimeTelemetryUVE(64U, budget, 8U, 8U);
    ASSERT_TRUE(telemetry.IsAcceptedUVE());
    EXPECT_TRUE(telemetry.searchBudgetDowngraded);
    EXPECT_TRUE(telemetry.searchBudgetSaturated);
    EXPECT_EQ(telemetry.effectiveSearchResults, 8U);
}

TEST(MotionQueryRuntimeTelemetryUVETest, InvalidCountersUVE_RejectsInconsistentEvaluationCount) {
    MotionQuerySearchBudgetUVE budget;
    budget.code = MotionQuerySearchBudgetCodeUVE::Accepted;
    budget.requestedResults = 4U;
    budget.effectiveResults = 4U;
    const auto telemetry = BuildMotionQueryRuntimeTelemetryUVE(4U, budget, 2U, 3U);
    EXPECT_EQ(telemetry.code, MotionQueryRuntimeTelemetryCodeUVE::InvalidCounters);
    EXPECT_FALSE(telemetry.IsAcceptedUVE());
}

TEST(MotionQueryRuntimeTelemetryUVETest, InvalidCountersUVE_RejectsBudgetOutsideAuthoritativeCap) {
    MotionQuerySearchBudgetUVE budget;
    budget.code = MotionQuerySearchBudgetCodeUVE::Accepted;
    budget.requestedResults = MotionQuerySearchBudgetUVE::kMaximumResultsUVE + 1U;
    budget.effectiveResults = MotionQuerySearchBudgetUVE::kMaximumResultsUVE + 1U;
    const auto telemetry = BuildMotionQueryRuntimeTelemetryUVE(
        MotionQuerySearchBudgetUVE::kMaximumResultsUVE, budget, 1U, 1U);
    EXPECT_EQ(telemetry.code, MotionQueryRuntimeTelemetryCodeUVE::InvalidCounters);
    EXPECT_FALSE(telemetry.IsAcceptedUVE());
}

} // namespace UVE::Plugins
