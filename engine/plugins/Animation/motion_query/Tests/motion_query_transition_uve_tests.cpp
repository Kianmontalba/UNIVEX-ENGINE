// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_transition_uve.h"

#include <gtest/gtest.h>

namespace UVE::Plugins {

TEST(MotionQueryTransitionUVETest, NoPreviousUVE_ReportsAcceptedBaseline) {
    MotionQueryTransitionRequestUVE request;
    request.currentCandidateIndex = 4U;
    const auto result = ArbitrateMotionQueryTransitionUVE(request, MotionQueryTransitionSettingsUVE{});
    EXPECT_EQ(result.code, MotionQueryTransitionCodeUVE::NoPreviousSelection);
    EXPECT_TRUE(result.IsAcceptedUVE());
    EXPECT_FALSE(result.ShouldHoldPreviousUVE());
}

TEST(MotionQueryTransitionUVETest, SameCandidateUVE_IsStable) {
    MotionQueryTransitionRequestUVE request;
    request.hasPreviousSelection = true;
    request.previousCandidateIndex = 4U;
    request.currentCandidateIndex = 4U;
    request.previousCost = 1.0F;
    request.currentCost = 1.0F;
    const auto result = ArbitrateMotionQueryTransitionUVE(request, MotionQueryTransitionSettingsUVE{});
    EXPECT_EQ(result.code, MotionQueryTransitionCodeUVE::SameCandidate);
    EXPECT_FLOAT_EQ(result.costImprovement, 0.0F);
}

TEST(MotionQueryTransitionUVETest, ImprovedCandidateUVE_SwitchesAfterThreshold) {
    MotionQueryTransitionRequestUVE request;
    request.hasPreviousSelection = true;
    request.previousCandidateIndex = 1U;
    request.currentCandidateIndex = 2U;
    request.previousCost = 1.0F;
    request.currentCost = 0.5F;
    request.elapsedSeconds = 0.2;
    const auto result = ArbitrateMotionQueryTransitionUVE(request, MotionQueryTransitionSettingsUVE{});
    EXPECT_EQ(result.code, MotionQueryTransitionCodeUVE::SwitchedCandidate);
    EXPECT_FLOAT_EQ(result.costImprovement, 0.5F);
}

TEST(MotionQueryTransitionUVETest, SmallImprovementUVE_HoldsWithinWindow) {
    MotionQueryTransitionRequestUVE request;
    request.hasPreviousSelection = true;
    request.previousCandidateIndex = 1U;
    request.currentCandidateIndex = 2U;
    request.previousCost = 1.0F;
    request.currentCost = 0.98F;
    request.elapsedSeconds = 0.05;
    const auto result = ArbitrateMotionQueryTransitionUVE(request, MotionQueryTransitionSettingsUVE{});
    EXPECT_EQ(result.code, MotionQueryTransitionCodeUVE::HeldPreviousCandidate);
    EXPECT_TRUE(result.ShouldHoldPreviousUVE());
}

TEST(MotionQueryTransitionUVETest, ExpiredWindowUVE_DoesNotHoldStaleCandidate) {
    MotionQueryTransitionRequestUVE request;
    request.hasPreviousSelection = true;
    request.previousCandidateIndex = 1U;
    request.currentCandidateIndex = 2U;
    request.previousCost = 1.0F;
    request.currentCost = 0.99F;
    request.elapsedSeconds = 1.0;
    const auto result = ArbitrateMotionQueryTransitionUVE(request, MotionQueryTransitionSettingsUVE{});
    EXPECT_EQ(result.code, MotionQueryTransitionCodeUVE::HoldWindowExpired);
    EXPECT_FALSE(result.ShouldHoldPreviousUVE());
}

TEST(MotionQueryTransitionUVETest, InvalidRequestUVE_RejectsNonFiniteOrNegativeValues) {
    MotionQueryTransitionRequestUVE request;
    request.hasPreviousSelection = true;
    request.previousCandidateIndex = 1U;
    request.currentCandidateIndex = 2U;
    request.previousCost = -1.0F;
    const auto result = ArbitrateMotionQueryTransitionUVE(request, MotionQueryTransitionSettingsUVE{});
    EXPECT_EQ(result.code, MotionQueryTransitionCodeUVE::InvalidRequest);
    EXPECT_FALSE(result.IsAcceptedUVE());
}

} // namespace UVE::Plugins
