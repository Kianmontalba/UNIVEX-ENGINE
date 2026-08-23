// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_search_index_uve.h"

#include <gtest/gtest.h>

#include <limits>

namespace UVE::Plugins {
namespace {
UVE::Core::MotionQueryFeatureSchemaUVE MakeSchemaUVE() {
    UVE::Core::MotionQueryFeatureSchemaUVE schema;
    schema.channels = {
        UVE::Core::MotionQueryFeatureChannelUVE{"velocity", UVE::Core::MotionQueryFeatureChannelKindUVE::RootVelocity, 0U, 1.0F},
        UVE::Core::MotionQueryFeatureChannelUVE{"facing", UVE::Core::MotionQueryFeatureChannelKindUVE::FacingDirection, 0U, 1.0F},
    };
    return schema;
}

UVE::Core::MotionMatchingCandidateUVE MakeCandidateUVE(const char* id, float velocityX,
                                                        float facingZ) {
    UVE::Core::MotionMatchingCandidateUVE candidate;
    candidate.candidateId = id;
    candidate.sourceClipId = "walk";
    candidate.feature.rootVelocity = UVE::Math::Vector3UVE{velocityX, 0.0F, 0.0F};
    candidate.feature.facingDirection = UVE::Math::Vector3UVE{0.0F, 0.0F, facingZ};
    return candidate;
}

UVE::Core::MotionMatchingDatabaseUVE MakeDatabaseUVE() {
    UVE::Core::MotionMatchingDatabaseUVE database;
    database.candidates = {
        MakeCandidateUVE("far", 5.0F, 1.0F),
        MakeCandidateUVE("near", 1.0F, 1.0F),
        MakeCandidateUVE("tie", 1.0F, 1.0F),
    };
    return database;
}
} // namespace

TEST(MotionQuerySearchIndexUVETest, BuildUVE_UsesCanonicalDatabaseFeaturesAndFindsNearestIndices) {
    MotionQuerySearchIndexUVE index;
    ASSERT_TRUE(index.BuildUVE(MakeDatabaseUVE(), MakeSchemaUVE()).IsAcceptedUVE());
    EXPECT_TRUE(index.IsBuiltUVE());
    EXPECT_EQ(index.SizeUVE(), 3U);
    EXPECT_EQ(index.DimensionUVE(), 2U);

    UVE::Core::MotionQueryFeatureVectorUVE query;
    ASSERT_TRUE(UVE::Core::TryBuildMotionQueryFeatureVectorUVE(
                    MakeCandidateUVE("query", 1.0F, 1.0F).feature, MakeSchemaUVE(), query)
                    .IsValidUVE());
    std::vector<std::size_t> candidates;
    const MotionQuerySearchIndexResultUVE result = index.FindNearestUVE(query, 2U, candidates);
    ASSERT_TRUE(result.IsAcceptedUVE()) << result.message;
    ASSERT_EQ(candidates.size(), 2U);
    EXPECT_EQ(candidates[0], 1U);
    EXPECT_EQ(candidates[1], 2U);
}

TEST(MotionQuerySearchIndexUVETest, FindNearestUVE_IsDeterministicForEqualDistances) {
    MotionQuerySearchIndexUVE index;
    ASSERT_TRUE(index.BuildUVE(MakeDatabaseUVE(), MakeSchemaUVE()).IsAcceptedUVE());
    const UVE::Core::MotionQueryFeatureVectorUVE query{{1.0F, 1.0F}, 2.0F};
    std::vector<std::size_t> first;
    std::vector<std::size_t> second;
    ASSERT_TRUE(index.FindNearestUVE(query, 3U, first).IsAcceptedUVE());
    ASSERT_TRUE(index.FindNearestUVE(query, 3U, second).IsAcceptedUVE());
    EXPECT_EQ(first, second);
    ASSERT_EQ(first.size(), 3U);
    EXPECT_EQ(first[0], 1U);
    EXPECT_EQ(first[1], 2U);
}

TEST(MotionQuerySearchIndexUVETest, BuildFromEntriesUVE_RejectsDuplicateAndInconsistentEntries) {
    MotionQuerySearchIndexUVE index;
    const std::vector<MotionQuerySearchEntryUVE> duplicate = {
        MotionQuerySearchEntryUVE{4U, UVE::Core::MotionQueryFeatureVectorUVE{{1.0F}, 1.0F}},
        MotionQuerySearchEntryUVE{4U, UVE::Core::MotionQueryFeatureVectorUVE{{2.0F}, 1.0F}},
    };
    EXPECT_EQ(index.BuildFromEntriesUVE(duplicate).code,
              MotionQuerySearchIndexResultUVE::Code::DuplicateCandidateIndex);

    const std::vector<MotionQuerySearchEntryUVE> inconsistent = {
        MotionQuerySearchEntryUVE{4U, UVE::Core::MotionQueryFeatureVectorUVE{{1.0F}, 1.0F}},
        MotionQuerySearchEntryUVE{5U, UVE::Core::MotionQueryFeatureVectorUVE{{2.0F, 3.0F}, 1.0F}},
    };
    EXPECT_EQ(index.BuildFromEntriesUVE(inconsistent).code,
              MotionQuerySearchIndexResultUVE::Code::InconsistentFeatureDimensions);
}

TEST(MotionQuerySearchIndexUVETest, FindNearestUVE_PreservesFiniteExtremeFeatureDistances) {
    const float maximumValue = std::numeric_limits<float>::max();
    MotionQuerySearchIndexUVE index;
    const std::vector<MotionQuerySearchEntryUVE> entries = {
        MotionQuerySearchEntryUVE{0U, UVE::Core::MotionQueryFeatureVectorUVE{{-maximumValue}, 1.0F}},
        MotionQuerySearchEntryUVE{1U, UVE::Core::MotionQueryFeatureVectorUVE{{maximumValue}, 1.0F}},
    };
    ASSERT_TRUE(index.BuildFromEntriesUVE(entries).IsAcceptedUVE());

    std::vector<std::size_t> candidates;
    const MotionQuerySearchIndexResultUVE result = index.FindNearestUVE(
        UVE::Core::MotionQueryFeatureVectorUVE{{maximumValue}, 1.0F}, 2U, candidates);
    ASSERT_TRUE(result.IsAcceptedUVE()) << result.message;
    ASSERT_EQ(candidates.size(), 2U);
    EXPECT_EQ(candidates[0], 1U);
    EXPECT_EQ(candidates[1], 0U);
}

TEST(MotionQuerySearchIndexUVETest, FindNearestUVE_RejectsInvalidQueryAndResultCount) {
    MotionQuerySearchIndexUVE index;
    ASSERT_TRUE(index.BuildUVE(MakeDatabaseUVE(), MakeSchemaUVE()).IsAcceptedUVE());
    std::vector<std::size_t> candidates;
    EXPECT_EQ(index.FindNearestUVE(UVE::Core::MotionQueryFeatureVectorUVE{{1.0F}, 1.0F}, 1U,
                                   candidates)
                  .code,
              MotionQuerySearchIndexResultUVE::Code::InvalidQuery);
    EXPECT_EQ(index.FindNearestUVE(UVE::Core::MotionQueryFeatureVectorUVE{{1.0F, 1.0F}, 2.0F}, 0U,
                                   candidates)
                  .code,
              MotionQuerySearchIndexResultUVE::Code::InvalidResultCount);
}
} // namespace UVE::Plugins
