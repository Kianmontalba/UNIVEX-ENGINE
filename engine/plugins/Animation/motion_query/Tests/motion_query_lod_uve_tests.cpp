// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_animation_node_uve.h"
#include "uve/plugins/motion_query_lod_uve.h"

#include <gtest/gtest.h>

namespace UVE::Plugins {
namespace {
UVE::Core::MotionQueryFeatureSchemaUVE MakeSchemaUVE() {
    UVE::Core::MotionQueryFeatureSchemaUVE schema;
    schema.channels = {
        UVE::Core::MotionQueryFeatureChannelUVE{
            "velocity", UVE::Core::MotionQueryFeatureChannelKindUVE::RootVelocity, 0U, 1.0F},
        UVE::Core::MotionQueryFeatureChannelUVE{
            "facing", UVE::Core::MotionQueryFeatureChannelKindUVE::FacingDirection, 0U, 1.0F},
    };
    return schema;
}

UVE::Core::MotionMatchingDatabaseUVE MakeDatabaseUVE() {
    UVE::Core::MotionMatchingDatabaseUVE database;
    for (std::size_t index = 0U; index < 4U; ++index) {
        UVE::Core::MotionMatchingCandidateUVE candidate;
        candidate.candidateId = "candidate" + std::to_string(index);
        candidate.sourceClipId = "missing";
        candidate.sampleTimeSeconds = 0.5;
        candidate.feature.rootVelocity = UVE::Math::Vector3UVE{
            static_cast<float>(index + 1U), 0.0F, 0.0F};
        candidate.feature.facingDirection = UVE::Math::Vector3UVE{0.0F, 0.0F, 1.0F};
        database.candidates.push_back(candidate);
    }
    return database;
}

UVE::Core::MotionQueryUVE MakeQueryUVE() {
    UVE::Core::MotionQueryUVE query;
    query.rootVelocity = UVE::Math::Vector3UVE{1.0F, 0.0F, 0.0F};
    query.facingDirection = UVE::Math::Vector3UVE{0.0F, 0.0F, 1.0F};
    return query;
}
} // namespace

TEST(MotionQueryLodUVETest, ResolveBudgetUVE_UsesDeterministicTierCaps) {
    const MotionQuerySearchBudgetUVE full =
        ResolveMotionQuerySearchBudgetUVE(MotionQueryQualityTierUVE::Full, 32U);
    EXPECT_EQ(full.code, MotionQuerySearchBudgetCodeUVE::Accepted);
    EXPECT_EQ(full.effectiveResults, 32U);
    EXPECT_FALSE(full.WasDowngradedUVE());

    const MotionQuerySearchBudgetUVE reduced =
        ResolveMotionQuerySearchBudgetUVE(MotionQueryQualityTierUVE::Reduced, 32U);
    EXPECT_EQ(reduced.code, MotionQuerySearchBudgetCodeUVE::Downgraded);
    EXPECT_EQ(reduced.effectiveResults, MotionQuerySearchBudgetUVE::kReducedMaximumResultsUVE);
    EXPECT_TRUE(reduced.WasDowngradedUVE());

    const MotionQuerySearchBudgetUVE minimal =
        ResolveMotionQuerySearchBudgetUVE(MotionQueryQualityTierUVE::Minimal, 32U);
    EXPECT_EQ(minimal.effectiveResults, MotionQuerySearchBudgetUVE::kMinimalMaximumResultsUVE);
}

TEST(MotionQueryLodUVETest, ResolveBudgetUVE_RejectsOutOfRangeRequests) {
    EXPECT_EQ(ResolveMotionQuerySearchBudgetUVE(MotionQueryQualityTierUVE::Full, 0U).code,
              MotionQuerySearchBudgetCodeUVE::InvalidResultCount);
    EXPECT_EQ(ResolveMotionQuerySearchBudgetUVE(
                  MotionQueryQualityTierUVE::Full,
                  MotionQuerySearchBudgetUVE::kMaximumResultsUVE + 1U)
                  .code,
              MotionQuerySearchBudgetCodeUVE::InvalidResultCount);
    EXPECT_EQ(ResolveMotionQuerySearchBudgetUVE(static_cast<MotionQueryQualityTierUVE>(99U), 1U).code,
              MotionQuerySearchBudgetCodeUVE::InvalidQualityTier);
}

TEST(MotionQueryLodUVETest, EvaluateUVE_ReportsReducedBudgetWithoutChangingIndexOwnership) {
    const auto database = MakeDatabaseUVE();
    const auto schema = MakeSchemaUVE();
    MotionQuerySearchIndexUVE index;
    ASSERT_TRUE(index.BuildUVE(database, schema).IsAcceptedUVE());
    MotionQueryAnimationNodeSettingsUVE settings;
    settings.maximumSearchResults = 32U;
    settings.qualityTier = MotionQueryQualityTierUVE::Reduced;
    const MotionQueryAnimationNodeResultUVE result = EvaluateMotionQueryAnimationNodeUVE(
        MakeQueryUVE(), database, schema, index, {}, settings);
    EXPECT_EQ(result.code, MotionQueryAnimationNodeCodeUVE::MissingClip);
    EXPECT_EQ(result.requestedSearchResults, 32U);
    EXPECT_EQ(result.effectiveSearchResults, MotionQuerySearchBudgetUVE::kReducedMaximumResultsUVE);
    EXPECT_EQ(result.qualityTier, MotionQueryQualityTierUVE::Reduced);
    EXPECT_TRUE(result.searchBudgetDowngraded);
    EXPECT_EQ(index.SizeUVE(), database.candidates.size());
}

TEST(MotionQueryLodUVETest, EvaluateUVE_RejectsInvalidQualityBudgetBeforeSearch) {
    const auto database = MakeDatabaseUVE();
    const auto schema = MakeSchemaUVE();
    MotionQuerySearchIndexUVE index;
    ASSERT_TRUE(index.BuildUVE(database, schema).IsAcceptedUVE());
    MotionQueryAnimationNodeSettingsUVE settings;
    settings.maximumSearchResults = 0U;
    const MotionQueryAnimationNodeResultUVE result = EvaluateMotionQueryAnimationNodeUVE(
        MakeQueryUVE(), database, schema, index, {}, settings);
    EXPECT_EQ(result.code, MotionQueryAnimationNodeCodeUVE::InvalidSettings);
    EXPECT_EQ(index.SizeUVE(), database.candidates.size());
}
} // namespace UVE::Plugins
