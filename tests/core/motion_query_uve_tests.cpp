// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/core/motion_query_uve.h"

#include <gtest/gtest.h>

namespace UVE::Core {
namespace {

TransformPoseUVE MakePoseUVE(float x, float y = 0.0F, float z = 0.0F) {
    return TransformPoseUVE{{x, y, z}, {}, {1.0F, 1.0F, 1.0F}};
}

MotionQueryUVE MakeFeatureUVE(float velocityX, float trajectoryX = 0.0F) {
    MotionQueryUVE feature;
    feature.rootVelocity = {velocityX, 0.0F, 0.0F};
    feature.facingDirection = {0.0F, 0.0F, 1.0F};
    feature.trajectory = {
        MotionTrajectorySampleUVE{0.25, {trajectoryX, 0.0F, 0.0F}},
        MotionTrajectorySampleUVE{0.5, {trajectoryX * 2.0F, 0.0F, 0.0F}},
    };
    return feature;
}

MotionMatchingDatabaseUVE MakeDatabaseUVE() {
    MotionMatchingDatabaseUVE database;
    database.candidates = {
        MotionMatchingCandidateUVE{"slow", "locomotion", 0.25, MakeFeatureUVE(0.5F, 0.5F)},
        MotionMatchingCandidateUVE{"fast", "locomotion", 1.25, MakeFeatureUVE(2.0F, 2.0F)},
        MotionMatchingCandidateUVE{"query", "locomotion", 2.25, MakeFeatureUVE(1.0F, 1.0F)},
    };
    return database;
}

} // namespace

TEST(MotionQueryUVETest, TryBuildMotionQueryUVE_DerivesVelocityAndCopiesTrajectory) {
    const std::vector<MotionTrajectorySampleUVE> trajectory = {
        MotionTrajectorySampleUVE{0.25, {0.5F, 0.0F, 0.0F}},
        MotionTrajectorySampleUVE{0.5, {1.0F, 0.0F, 0.0F}},
    };
    MotionQueryUVE query;

    ASSERT_TRUE(TryBuildMotionQueryUVE(MakePoseUVE(0.0F), MakePoseUVE(1.0F), 0.5, trajectory,
                                       query));
    EXPECT_NEAR(query.rootVelocity.x, 2.0F, 1.0e-5F);
    EXPECT_NEAR(query.rootVelocity.y, 0.0F, 1.0e-5F);
    EXPECT_FLOAT_EQ(query.facingDirection.z, 1.0F);
    EXPECT_EQ(query.trajectory, trajectory);
    EXPECT_TRUE(ValidateMotionQueryUVE(query).IsValidUVE());
}

TEST(MotionQueryUVETest, ValidateMotionQueryUVE_RejectsUnsortedAndZeroFacingData) {
    MotionQueryUVE query = MakeFeatureUVE(1.0F);
    query.trajectory[1].offsetSeconds = 0.1;
    EXPECT_EQ(ValidateMotionQueryUVE(query).code,
              MotionQueryValidationCodeUVE::UnsortedTrajectory);

    query = MakeFeatureUVE(1.0F);
    query.facingDirection = {};
    EXPECT_EQ(ValidateMotionQueryUVE(query).code,
              MotionQueryValidationCodeUVE::InvalidVector);
}

TEST(MotionQueryUVETest, ValidateMotionMatchingDatabaseUVE_RequiresUniqueConsistentCandidates) {
    MotionMatchingDatabaseUVE database = MakeDatabaseUVE();
    EXPECT_TRUE(ValidateMotionMatchingDatabaseUVE(database).IsValidUVE());

    database.candidates[1].candidateId = database.candidates[0].candidateId;
    EXPECT_EQ(ValidateMotionMatchingDatabaseUVE(database).code,
              MotionMatchingDatabaseValidationCodeUVE::DuplicateCandidateIdentifier);

    database = MakeDatabaseUVE();
    database.candidates[1].feature.trajectory.pop_back();
    EXPECT_EQ(ValidateMotionMatchingDatabaseUVE(database).code,
              MotionMatchingDatabaseValidationCodeUVE::InconsistentTrajectorySchema);
}

TEST(MotionQueryUVETest, FindBestMotionMatchUVE_SelectsLowestWeightedFeatureCost) {
    const MotionQueryUVE query = MakeFeatureUVE(1.0F, 1.0F);
    const MotionMatchingResultUVE result =
        FindBestMotionMatchUVE(query, MakeDatabaseUVE(), MotionMatchingWeightsUVE{});

    ASSERT_TRUE(result.IsMatchUVE());
    EXPECT_EQ(result.candidateIndex, 2U);
    EXPECT_EQ(result.candidatesEvaluated, 3U);
    EXPECT_NEAR(result.cost, 0.0F, 1.0e-5F);
}

TEST(MotionQueryUVETest, FindBestMotionMatchUVE_UsesStableIdentifierTieBreak) {
    MotionMatchingDatabaseUVE database;
    database.candidates = {
        MotionMatchingCandidateUVE{"zeta", "locomotion", 0.5, MakeFeatureUVE(1.0F, 1.0F)},
        MotionMatchingCandidateUVE{"alpha", "locomotion", 0.5, MakeFeatureUVE(1.0F, 1.0F)},
    };

    const MotionMatchingResultUVE result =
        FindBestMotionMatchUVE(MakeFeatureUVE(1.0F, 1.0F), database, MotionMatchingWeightsUVE{});
    ASSERT_TRUE(result.IsMatchUVE());
    EXPECT_EQ(result.candidateIndex, 1U);
}

TEST(MotionQueryUVETest, FindBestMotionMatchUVE_RejectsInvalidWeights) {
    MotionMatchingWeightsUVE weights;
    weights.velocityWeight = 0.0F;
    weights.facingWeight = 0.0F;
    weights.trajectoryWeight = 0.0F;

    const MotionMatchingResultUVE result =
        FindBestMotionMatchUVE(MakeFeatureUVE(1.0F), MakeDatabaseUVE(), weights);
    EXPECT_EQ(result.code, MotionMatchingResultCodeUVE::InvalidWeights);
}

} // namespace UVE::Core
