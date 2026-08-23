// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/plugins/motion_query_feature_channels_uve.h"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

namespace UVE::Core {
namespace {

MotionQueryUVE MakeQueryUVE() {
    MotionQueryUVE query;
    query.rootVelocity = {2.0F, 0.0F, 0.0F};
    query.facingDirection = {0.0F, 0.0F, 1.0F};
    query.trajectory = {
        MotionTrajectorySampleUVE{0.25, {1.0F, 0.0F, 0.0F}},
        MotionTrajectorySampleUVE{0.5, {2.0F, 0.0F, 0.0F}},
    };
    return query;
}

MotionQueryFeatureSchemaUVE MakeSchemaUVE() {
    MotionQueryFeatureSchemaUVE schema;
    schema.channels = {
        MotionQueryFeatureChannelUVE{"velocity", MotionQueryFeatureChannelKindUVE::RootVelocity, 0U, 0.5F},
        MotionQueryFeatureChannelUVE{"facing", MotionQueryFeatureChannelKindUVE::FacingDirection, 0U, 1.0F},
        MotionQueryFeatureChannelUVE{"trajectory_time", MotionQueryFeatureChannelKindUVE::TrajectoryTime, 1U, 2.0F},
        MotionQueryFeatureChannelUVE{"trajectory_distance", MotionQueryFeatureChannelKindUVE::TrajectoryDistance, 1U, 1.0F},
    };
    return schema;
}

} // namespace

TEST(MotionQueryFeatureChannelsUVETest, ValidateSchemaUVE_AndBuildVectorUVE_AreDeterministic) {
    const MotionQueryUVE query = MakeQueryUVE();
    const MotionQueryFeatureSchemaUVE schema = MakeSchemaUVE();
    MotionQueryFeatureVectorUVE vector;

    ASSERT_TRUE(ValidateMotionQueryFeatureSchemaUVE(schema).IsValidUVE());
    ASSERT_TRUE(TryBuildMotionQueryFeatureVectorUVE(query, schema, vector).IsValidUVE());
    ASSERT_EQ(vector.values.size(), 4U);
    EXPECT_FLOAT_EQ(vector.values[0], 2.0F); // |velocity|^2 * 0.5
    EXPECT_FLOAT_EQ(vector.values[1], 1.0F); // facing z
    EXPECT_FLOAT_EQ(vector.values[2], 1.0F); // 0.5 * 2.0
    EXPECT_FLOAT_EQ(vector.values[3], 1.0F); // delta (1,0,0) squared
    EXPECT_FLOAT_EQ(vector.totalWeight, 4.5F);
}

TEST(MotionQueryFeatureChannelsUVETest, ValidateSchemaUVE_RejectsDuplicatesWeightsAndCapacity) {
    MotionQueryFeatureSchemaUVE schema = MakeSchemaUVE();
    schema.channels[1].id = schema.channels[0].id;
    EXPECT_EQ(ValidateMotionQueryFeatureSchemaUVE(schema).code,
              MotionQueryFeatureValidationCodeUVE::DuplicateChannelId);

    schema = MakeSchemaUVE();
    schema.channels[0].weight = -1.0F;
    EXPECT_EQ(ValidateMotionQueryFeatureSchemaUVE(schema).code,
              MotionQueryFeatureValidationCodeUVE::InvalidWeight);

    schema = MakeSchemaUVE();
    schema.channels[2].trajectorySampleIndex = MotionQueryUVE::kMaximumTrajectorySamplesUVE;
    EXPECT_EQ(ValidateMotionQueryFeatureSchemaUVE(schema).code,
              MotionQueryFeatureValidationCodeUVE::InvalidSampleIndex);
}

TEST(MotionQueryFeatureChannelsUVETest, TryBuildVectorUVE_PreservesFiniteExtremeSquaredFeatures) {
    const float maximum = std::numeric_limits<float>::max();
    MotionQueryUVE query;
    query.rootVelocity = {maximum, 0.0F, 0.0F};
    query.trajectory = {
        MotionTrajectorySampleUVE{0.25, {-maximum, 0.0F, 0.0F}},
        MotionTrajectorySampleUVE{0.5, {maximum, 0.0F, 0.0F}},
    };
    MotionQueryFeatureSchemaUVE schema;
    schema.channels = {
        MotionQueryFeatureChannelUVE{"velocity", MotionQueryFeatureChannelKindUVE::RootVelocity, 0U,
                                     1.0e-40F},
        MotionQueryFeatureChannelUVE{"distance", MotionQueryFeatureChannelKindUVE::TrajectoryDistance, 1U,
                                     1.0e-40F},
    };
    MotionQueryFeatureVectorUVE vector;

    const MotionQueryFeatureValidationResultUVE result =
        TryBuildMotionQueryFeatureVectorUVE(query, schema, vector);
    ASSERT_TRUE(result.IsValidUVE()) << result.message;
    ASSERT_EQ(vector.values.size(), 2U);
    EXPECT_TRUE(std::isfinite(vector.values[0]));
    EXPECT_TRUE(std::isfinite(vector.values[1]));
    EXPECT_GT(vector.values[0], 0.0F);
    EXPECT_GT(vector.values[1], 0.0F);
    EXPECT_FLOAT_EQ(vector.totalWeight, 2.0e-40F);
}

TEST(MotionQueryFeatureChannelsUVETest, TryBuildVectorUVE_RejectsNonFiniteWeightedValueWithoutPublishing) {
    MotionQueryFeatureSchemaUVE schema;
    schema.channels = {MotionQueryFeatureChannelUVE{
        "velocity", MotionQueryFeatureChannelKindUVE::RootVelocity, 0U,
        std::numeric_limits<float>::max()}};
    MotionQueryUVE query = MakeQueryUVE();
    MotionQueryFeatureVectorUVE vector{{7.0F}, 3.0F};

    const MotionQueryFeatureValidationResultUVE result =
        TryBuildMotionQueryFeatureVectorUVE(query, schema, vector);
    EXPECT_EQ(result.code, MotionQueryFeatureValidationCodeUVE::InvalidQuery);
    EXPECT_EQ(vector.values, (std::vector<float>{7.0F}));
    EXPECT_FLOAT_EQ(vector.totalWeight, 3.0F);
}

TEST(MotionQueryFeatureChannelsUVETest, TryBuildVectorUVE_RejectsNonFiniteTotalWeightWithoutPublishing) {
    MotionQueryFeatureSchemaUVE schema;
    schema.channels = {
        MotionQueryFeatureChannelUVE{"first", MotionQueryFeatureChannelKindUVE::FacingDirection, 0U,
                                     std::numeric_limits<float>::max()},
        MotionQueryFeatureChannelUVE{"second", MotionQueryFeatureChannelKindUVE::FacingDirection, 0U,
                                     std::numeric_limits<float>::max()},
    };
    const MotionQueryUVE query = MakeQueryUVE();
    MotionQueryFeatureVectorUVE vector{{7.0F}, 3.0F};

    const MotionQueryFeatureValidationResultUVE result =
        TryBuildMotionQueryFeatureVectorUVE(query, schema, vector);
    EXPECT_EQ(result.code, MotionQueryFeatureValidationCodeUVE::InvalidQuery);
    EXPECT_EQ(vector.values, (std::vector<float>{7.0F}));
    EXPECT_FLOAT_EQ(vector.totalWeight, 3.0F);
}

TEST(MotionQueryFeatureChannelsUVETest, TryBuildVectorUVE_RejectsQueryTrajectoryMismatch) {
    MotionQueryFeatureSchemaUVE schema;
    schema.channels = {MotionQueryFeatureChannelUVE{
        "future", MotionQueryFeatureChannelKindUVE::TrajectoryPosition, 1U, 1.0F}};
    MotionQueryUVE query = MakeQueryUVE();
    query.trajectory.clear();

    MotionQueryFeatureVectorUVE vector;
    const MotionQueryFeatureValidationResultUVE result =
        TryBuildMotionQueryFeatureVectorUVE(query, schema, vector);
    EXPECT_EQ(result.code, MotionQueryFeatureValidationCodeUVE::InvalidSampleIndex);
}

TEST(MotionQueryFeatureChannelsUVETest, CandidateFilterAndChooserRowUVE_ExposeCopiedFacts) {
    MotionMatchingCandidateUVE candidate;
    candidate.candidateId = "walk_1";
    candidate.sourceClipId = "walk";
    candidate.sampleTimeSeconds = 0.75;
    MotionMatchingResultUVE result{MotionMatchingResultCodeUVE::Matched, 2U, 8U, 1.5F, "matched"};

    MotionQueryCandidateFilterUVE filter;
    filter.sourceClipId = "walk";
    filter.minimumSampleTimeSeconds = 0.5;
    filter.maximumSampleTimeSeconds = 1.0;
    filter.maximumCost = 2.0F;
    EXPECT_EQ(EvaluateMotionQueryCandidateFilterUVE(candidate, result, filter),
              MotionQueryCandidateFilterCodeUVE::Accepted);

    const MotionQueryChooserRowUVE row = BuildMotionQueryChooserRowUVE(candidate, result);
    EXPECT_EQ(row.candidateId, "walk_1");
    EXPECT_EQ(row.sampleTimeSeconds, 0.75);
    EXPECT_FLOAT_EQ(row.cost, 1.5F);
    EXPECT_EQ(row.candidatesEvaluated, 8U);

    filter.sourceClipId = "run";
    EXPECT_EQ(EvaluateMotionQueryCandidateFilterUVE(candidate, result, filter),
              MotionQueryCandidateFilterCodeUVE::SourceClipMismatch);
}

} // namespace UVE::Core
