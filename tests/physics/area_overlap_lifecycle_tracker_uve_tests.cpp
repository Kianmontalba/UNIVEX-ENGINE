// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/physics/area_overlap_lifecycle_tracker_uve.h"

#include <cstddef>
#include <vector>

#include <gtest/gtest.h>

namespace UVE::Physics::Tests {
namespace {

[[nodiscard]] AreaOverlapPairUVE PairUVE(const std::uint32_t areaIndex, const std::uint32_t areaGeneration,
                                         const std::uint32_t otherIndex, const std::uint32_t otherGeneration,
                                         const float depth = 0.5F) {
    return {{areaIndex, areaGeneration}, {otherIndex, otherGeneration}, depth};
}

TEST(AreaOverlapLifecycleTrackerUVETest, UpdateUVE_ReportsEnteredStableAndExitedTransitions) {
    AreaOverlapLifecycleTrackerUVE tracker;
    const AreaOverlapPairUVE pair = PairUVE(1U, 1U, 2U, 1U);

    AreaOverlapQueryResultUVE enteredSnapshot;
    enteredSnapshot.overlaps = {pair};
    const AreaOverlapLifecycleReportUVE entered = tracker.UpdateUVE(enteredSnapshot);
    ASSERT_EQ(entered.transitions.size(), 1U);
    EXPECT_EQ(entered.transitions.front().kind, AreaOverlapTransitionKindUVE::Entered);
    EXPECT_EQ(entered.transitions.front().pair, pair);
    EXPECT_EQ(entered.currentActiveCount, 1U);

    const AreaOverlapLifecycleReportUVE stable = tracker.UpdateUVE(enteredSnapshot);
    EXPECT_TRUE(stable.transitions.empty());
    EXPECT_EQ(stable.previousActiveCount, 1U);
    EXPECT_EQ(stable.currentActiveCount, 1U);

    const AreaOverlapLifecycleReportUVE exited = tracker.UpdateUVE({});
    ASSERT_EQ(exited.transitions.size(), 1U);
    EXPECT_EQ(exited.transitions.front().kind, AreaOverlapTransitionKindUVE::Exited);
    EXPECT_EQ(exited.transitions.front().pair, pair);
    EXPECT_EQ(exited.currentActiveCount, 0U);
}

TEST(AreaOverlapLifecycleTrackerUVETest, UpdateUVE_EntityGenerationChangeIsExitThenEnter) {
    AreaOverlapLifecycleTrackerUVE tracker;
    AreaOverlapQueryResultUVE firstSnapshot;
    firstSnapshot.overlaps = {PairUVE(3U, 1U, 4U, 1U)};
    ASSERT_EQ(tracker.UpdateUVE(firstSnapshot).transitions.size(), 1U);

    AreaOverlapQueryResultUVE replacementSnapshot;
    replacementSnapshot.overlaps = {PairUVE(3U, 2U, 4U, 1U)};
    const AreaOverlapLifecycleReportUVE report = tracker.UpdateUVE(replacementSnapshot);

    ASSERT_EQ(report.transitions.size(), 2U);
    EXPECT_EQ(report.transitions[0].kind, AreaOverlapTransitionKindUVE::Exited);
    EXPECT_EQ(report.transitions[0].pair.area.generation, 1U);
    EXPECT_EQ(report.transitions[1].kind, AreaOverlapTransitionKindUVE::Entered);
    EXPECT_EQ(report.transitions[1].pair.area.generation, 2U);
}

TEST(AreaOverlapLifecycleTrackerUVETest, UpdateUVE_TruncatedInputRetainsBaselineAndInfersNoExit) {
    AreaOverlapLifecycleTrackerUVE tracker;
    const AreaOverlapPairUVE pair = PairUVE(5U, 1U, 6U, 1U);
    AreaOverlapQueryResultUVE initial;
    initial.overlaps = {pair};
    ASSERT_EQ(tracker.UpdateUVE(initial).currentActiveCount, 1U);

    AreaOverlapQueryResultUVE truncated;
    truncated.truncated = true;
    const AreaOverlapLifecycleReportUVE report = tracker.UpdateUVE(truncated);

    EXPECT_TRUE(report.inputSnapshotTruncated);
    EXPECT_TRUE(report.IsTruncatedUVE());
    EXPECT_TRUE(report.transitions.empty());
    EXPECT_EQ(report.previousActiveCount, 1U);
    EXPECT_EQ(report.currentActiveCount, 1U);
    EXPECT_EQ(tracker.GetActiveCountUVE(), 1U);
}

TEST(AreaOverlapLifecycleTrackerUVETest, UpdateUVE_TransitionCapTruncatesReportButCommitsCompleteCurrentBaseline) {
    AreaOverlapLifecycleTrackerUVE tracker;
    AreaOverlapQueryResultUVE initial;
    initial.overlaps = {PairUVE(7U, 1U, 8U, 1U), PairUVE(7U, 1U, 9U, 1U)};
    ASSERT_EQ(tracker.UpdateUVE(initial).currentActiveCount, 2U);

    const AreaOverlapLifecycleReportUVE report = tracker.UpdateUVE({}, 1U);

    EXPECT_TRUE(report.transitionsTruncated);
    EXPECT_TRUE(report.IsTruncatedUVE());
    EXPECT_EQ(report.transitions.size(), 1U);
    EXPECT_EQ(report.currentActiveCount, 0U);
    EXPECT_EQ(tracker.GetActiveCountUVE(), 0U);
}

TEST(AreaOverlapLifecycleTrackerUVETest, ResetUVE_DiscardsBaselineWithoutFabricatingTransitions) {
    AreaOverlapLifecycleTrackerUVE tracker;
    AreaOverlapQueryResultUVE initial;
    initial.overlaps = {PairUVE(10U, 1U, 11U, 1U)};
    ASSERT_EQ(tracker.UpdateUVE(initial).currentActiveCount, 1U);

    tracker.ResetUVE();
    EXPECT_EQ(tracker.GetActiveCountUVE(), 0U);
    EXPECT_TRUE(tracker.UpdateUVE({}).transitions.empty());
}

} // namespace
} // namespace UVE::Physics::Tests
