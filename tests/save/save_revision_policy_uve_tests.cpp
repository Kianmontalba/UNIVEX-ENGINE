#include "uve/save/save_revision_policy_uve.h"

#include <gtest/gtest.h>

namespace UVE::Save::Tests {

TEST(SaveRevisionPolicyUVETest, MapsRevisionStatusesToSyncActions) {
    EXPECT_EQ(EvaluateSaveSyncActionUVE(SaveRevisionStatusUVE::Unchanged), SaveSyncActionUVE::NoOp);
    EXPECT_EQ(EvaluateSaveSyncActionUVE(SaveRevisionStatusUVE::LocalAhead), SaveSyncActionUVE::Upload);
    EXPECT_EQ(EvaluateSaveSyncActionUVE(SaveRevisionStatusUVE::RemoteAhead), SaveSyncActionUVE::Download);
    EXPECT_EQ(EvaluateSaveSyncActionUVE(SaveRevisionStatusUVE::Conflict), SaveSyncActionUVE::Conflict);
    EXPECT_EQ(EvaluateSaveSyncActionUVE(SaveRevisionStatusUVE::Invalid), SaveSyncActionUVE::Invalid);
}

TEST(SaveRevisionPolicyUVETest, ClassifiesUnchangedLocalAheadRemoteAheadAndConflict) {
    EXPECT_EQ(EvaluateSaveRevisionUVE(4U, 4U, 4U), SaveRevisionStatusUVE::Unchanged);
    EXPECT_EQ(EvaluateSaveRevisionUVE(4U, 5U, 4U), SaveRevisionStatusUVE::LocalAhead);
    EXPECT_EQ(EvaluateSaveRevisionUVE(4U, 4U, 5U), SaveRevisionStatusUVE::RemoteAhead);
    EXPECT_EQ(EvaluateSaveRevisionUVE(4U, 5U, 6U), SaveRevisionStatusUVE::Conflict);
}

TEST(SaveRevisionPolicyUVETest, RejectsZeroRevisionFacts) {
    EXPECT_EQ(EvaluateSaveRevisionUVE(0U, 4U, 4U), SaveRevisionStatusUVE::Invalid);
    EXPECT_EQ(EvaluateSaveRevisionUVE(4U, 0U, 4U), SaveRevisionStatusUVE::Invalid);
    EXPECT_EQ(EvaluateSaveRevisionUVE(4U, 4U, 0U), SaveRevisionStatusUVE::Invalid);
}

} // namespace UVE::Save::Tests
