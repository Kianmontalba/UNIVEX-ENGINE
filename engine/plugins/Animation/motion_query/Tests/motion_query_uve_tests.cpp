// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/plugins/motion_query_uve.h"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

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

TEST(MotionQueryUVETest, TryBuildMotionQueryUVE_PreservesFiniteExtremePositionDelta) {
    const float maximum = std::numeric_limits<float>::max();
    MotionQueryUVE query;
    ASSERT_TRUE(TryBuildMotionQueryUVE(MakePoseUVE(-maximum), MakePoseUVE(maximum), 2.0, {}, query));
    EXPECT_FLOAT_EQ(query.rootVelocity.x, maximum);
    EXPECT_FLOAT_EQ(query.rootVelocity.y, 0.0F);
    EXPECT_FLOAT_EQ(query.rootVelocity.z, 0.0F);
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

TEST(MotionQueryUVETest, FindBestMotionMatchUVE_AcceptsFiniteExtremeFacingDirection) {
    const float maximum = std::numeric_limits<float>::max();
    MotionQueryUVE query = MakeFeatureUVE(1.0F, 1.0F);
    query.facingDirection = {0.0F, 0.0F, maximum};
    MotionMatchingDatabaseUVE database = MakeDatabaseUVE();
    for (MotionMatchingCandidateUVE& candidate : database.candidates) {
        candidate.feature.facingDirection = {0.0F, 0.0F, maximum};
    }

    const MotionMatchingResultUVE result =
        FindBestMotionMatchUVE(query, database, MotionMatchingWeightsUVE{});
    ASSERT_TRUE(result.IsMatchUVE()) << result.message;
    EXPECT_EQ(result.candidatesEvaluated, database.candidates.size());
    EXPECT_EQ(result.candidateIndex, 2U);
    EXPECT_FLOAT_EQ(result.cost, 0.0F);
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

TEST(MotionQueryUVETest, SharedSkeletonPoseAndEvaluationContext_AreAcceptedAndCopied) {
    MotionQueryUVE query = MakeFeatureUVE(1.0F, 1.0F);
    query.skeleton = SkeletonDefinitionUVE{
        "locomotion", {SkeletonJointUVE{"root", ""}, SkeletonJointUVE{"spine", "root"}}};
    query.pose = PoseBufferUVE{
        "locomotion", {MakePoseUVE(0.0F), MakePoseUVE(0.0F, 1.0F)}};
    query.evaluationContext.time.animationTimeSeconds = 2.0;
    query.evaluationContext.time.animationDeltaSeconds = 1.0 / 60.0;
    query.evaluationContext.sampleTimeSeconds = 2.0;

    EXPECT_TRUE(ValidateMotionQueryUVE(query).IsValidUVE());
    const MotionQueryUVE copied = query;
    EXPECT_EQ(copied.skeleton, query.skeleton);
    EXPECT_EQ(copied.pose, query.pose);
    EXPECT_EQ(copied.evaluationContext, query.evaluationContext);
}

TEST(MotionQueryUVETest, SharedSkeletonPoseAndEvaluationContext_RejectMismatches) {
    MotionQueryUVE query = MakeFeatureUVE(1.0F);
    query.skeleton = SkeletonDefinitionUVE{"locomotion", {SkeletonJointUVE{"root", ""}}};
    query.pose = PoseBufferUVE{"other", {MakePoseUVE(0.0F)}};
    EXPECT_EQ(ValidateMotionQueryUVE(query).code, MotionQueryValidationCodeUVE::InvalidPose);

    query = MakeFeatureUVE(1.0F);
    query.evaluationContext.time.animationDeltaSeconds = -0.01;
    EXPECT_EQ(ValidateMotionQueryUVE(query).code,
              MotionQueryValidationCodeUVE::InvalidEvaluationTime);
}

TEST(MotionQueryUVETest, FindBestMotionMatchUVE_PreservesFiniteExtremeVelocityDistance) {
    const float maximum = std::numeric_limits<float>::max();
    MotionQueryUVE query;
    query.rootVelocity = {maximum, 0.0F, 0.0F};
    MotionMatchingCandidateUVE candidate;
    candidate.candidateId = "extreme-velocity";
    candidate.sourceClipId = "locomotion";
    candidate.feature.rootVelocity = {-maximum, 0.0F, 0.0F};
    MotionMatchingDatabaseUVE database;
    database.candidates = {candidate};

    const MotionMatchingResultUVE result = FindBestMotionMatchUVE(
        query, database, MotionMatchingWeightsUVE{1.0e-40F, 0.0F, 0.0F});
    ASSERT_TRUE(result.IsMatchUVE()) << result.message;
    EXPECT_EQ(result.candidateIndex, 0U);
    EXPECT_EQ(result.candidatesEvaluated, 1U);
    EXPECT_TRUE(std::isfinite(result.cost));
    EXPECT_GT(result.cost, 0.0F);
}

TEST(MotionQueryUVETest, FindBestMotionMatchUVE_PreservesFiniteTrajectoryMean) {
    const float extent = std::sqrt(std::numeric_limits<float>::max());
    MotionQueryUVE query;
    query.facingDirection = {0.0F, 0.0F, 1.0F};
    MotionMatchingCandidateUVE candidate;
    candidate.candidateId = "extreme-trajectory";
    candidate.sourceClipId = "locomotion";
    candidate.feature.facingDirection = {0.0F, 0.0F, 1.0F};
    for (std::size_t index = 0U; index < MotionQueryUVE::kMaximumTrajectorySamplesUVE; ++index) {
        const double offsetSeconds = static_cast<double>(index) * 0.25;
        query.trajectory.push_back(MotionTrajectorySampleUVE{offsetSeconds, {0.0F, 0.0F, 0.0F}});
        candidate.feature.trajectory.push_back(
            MotionTrajectorySampleUVE{offsetSeconds, {extent, 0.0F, 0.0F}});
    }
    MotionMatchingDatabaseUVE database;
    database.candidates = {candidate};

    const MotionMatchingResultUVE result = FindBestMotionMatchUVE(
        query, database, MotionMatchingWeightsUVE{0.0F, 0.0F, 1.0F});
    ASSERT_TRUE(result.IsMatchUVE()) << result.message;
    EXPECT_EQ(result.candidateIndex, 0U);
    EXPECT_EQ(result.candidatesEvaluated, 1U);
    EXPECT_TRUE(std::isfinite(result.cost));
    EXPECT_FLOAT_EQ(result.cost, std::numeric_limits<float>::max());
}

TEST(MotionQueryUVETest, FindBestMotionMatchUVE_RejectsNonFiniteWeightTotal) {
    const float maximumWeight = std::numeric_limits<float>::max();
    const MotionMatchingWeightsUVE weights{maximumWeight, maximumWeight, 0.0F};

    const MotionMatchingResultUVE result =
        FindBestMotionMatchUVE(MakeFeatureUVE(1.0F), MakeDatabaseUVE(), weights);
    EXPECT_EQ(result.code, MotionMatchingResultCodeUVE::InvalidWeights);
    EXPECT_EQ(result.candidatesEvaluated, 0U);
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
