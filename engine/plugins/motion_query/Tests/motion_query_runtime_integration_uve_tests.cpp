// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/events/event_system_uve.h"
#include "uve/plugins/motion_query_animation_node_uve.h"
#include "uve/plugins/motion_query_asset_ingestion_uve.h"
#include "uve/plugins/motion_query_search_index_uve.h"

#include <gtest/gtest.h>

#include <utility>

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

UVE::Core::MotionMatchingCandidateUVE MakeCandidateUVE(const char* id, const float velocityX,
                                                        const double sampleTime) {
    UVE::Core::MotionMatchingCandidateUVE candidate;
    candidate.candidateId = id;
    candidate.sourceClipId = "walk";
    candidate.sampleTimeSeconds = sampleTime;
    candidate.feature.rootVelocity = UVE::Math::Vector3UVE{velocityX, 0.0F, 0.0F};
    candidate.feature.facingDirection = UVE::Math::Vector3UVE{0.0F, 0.0F, 1.0F};
    return candidate;
}

UVE::Core::MotionMatchingDatabaseUVE MakeDatabaseUVE() {
    UVE::Core::MotionMatchingDatabaseUVE database;
    database.candidates = {
        MakeCandidateUVE("far", 5.0F, 0.25),
        MakeCandidateUVE("near", 1.0F, 0.75),
    };
    return database;
}

std::vector<UVE::Core::AnimationClipUVE> MakeClipsUVE() {
    UVE::Core::AnimationClipUVE clip;
    clip.clipId = "walk";
    clip.durationSeconds = 1.0;
    clip.samples = {
        UVE::Core::PoseSampleUVE{0.0,
                                 UVE::Core::TransformPoseUVE{
                                     UVE::Math::Vector3UVE{0.0F, 0.0F, 0.0F},
                                     UVE::Math::QuaternionUVE{},
                                     UVE::Math::Vector3UVE{1.0F, 1.0F, 1.0F}}},
        UVE::Core::PoseSampleUVE{1.0,
                                 UVE::Core::TransformPoseUVE{
                                     UVE::Math::Vector3UVE{2.0F, 0.0F, 0.0F},
                                     UVE::Math::QuaternionUVE{},
                                     UVE::Math::Vector3UVE{1.0F, 1.0F, 1.0F}}},
    };
    clip.samples[0].pose.rotation.w = 1.0F;
    clip.samples[1].pose.rotation.w = 1.0F;
    return {clip};
}

Asset::ResourceHandleUVE MakeHandleUVE(const std::uint64_t guid,
                                       const std::uint64_t generation = 1U) {
    return Asset::ResourceHandleUVE{Asset::AssetGuidUVE{guid}, generation};
}

MotionQueryAssetIngestionRequestUVE MakeRequestUVE(const Asset::ResourceHandleUVE source) {
    MotionQueryAssetIngestionRequestUVE request;
    request.source = source;
    request.key.source = source;
    request.key.schemaVersion = 2U;
    request.key.samplerVersion = 3U;
    request.key.normalizationVersion = 4U;
    return request;
}

MotionQueryDerivedDataUVE MakeDerivedDataUVE(
    const MotionQueryAssetIngestionRequestUVE& request, const float value) {
    MotionQueryDerivedDataUVE data;
    data.key = request.key;
    UVE::Core::MotionQueryFeatureVectorUVE sample;
    sample.values = {value, 1.0F - value};
    sample.totalWeight = 1.0F;
    data.normalizedSamples = {sample};
    return data;
}

UVE::Core::MotionQueryUVE MakeQueryUVE() {
    UVE::Core::MotionQueryUVE query;
    query.rootVelocity = UVE::Math::Vector3UVE{1.0F, 0.0F, 0.0F};
    query.facingDirection = UVE::Math::Vector3UVE{0.0F, 0.0F, 1.0F};
    return query;
}
} // namespace

TEST(MotionQueryRuntimeIntegrationUVETest,
     RuntimeFlowUVE_IngestsRebuildsSearchesAndEvaluatesAnimation) {
    UVE::Events::EventSystemUVE events;
    MotionQueryAssetRegistryUVE registry;
    MotionQueryDerivedDataCacheUVE cache;
    MotionQueryAssetIngestionCoordinatorUVE ingestion(events, registry, cache);
    const Asset::ResourceHandleUVE source = MakeHandleUVE(500U);
    ASSERT_TRUE(registry.RegisterUVE(source).IsAppliedUVE());
    const MotionQueryAssetIngestionRequestUVE request = MakeRequestUVE(source);
    ASSERT_TRUE(ingestion.RegisterUVE(request).IsAcceptedUVE());

    ASSERT_TRUE(ingestion.SubmitDerivedDataUVE(MakeDerivedDataUVE(request, 0.25F)).IsAcceptedUVE());
    ASSERT_EQ(ingestion.GetEntriesUVE().front().state,
              MotionQueryAssetIngestionStateUVE::Ready);
    MotionQueryDerivedDataUVE copiedDerivedData;
    ASSERT_TRUE(cache.FindUVE(request.key, registry, copiedDerivedData).IsAcceptedUVE());
    ASSERT_FLOAT_EQ(copiedDerivedData.normalizedSamples.front().values.front(), 0.25F);

    const UVE::Core::MotionMatchingDatabaseUVE database = MakeDatabaseUVE();
    const UVE::Core::MotionQueryFeatureSchemaUVE schema = MakeSchemaUVE();
    MotionQuerySearchIndexUVE searchIndex;
    ASSERT_TRUE(searchIndex.BuildUVE(database, schema).IsAcceptedUVE());
    ASSERT_EQ(searchIndex.SizeUVE(), database.candidates.size());
    std::vector<std::size_t> nearestCandidates;
    ASSERT_TRUE(searchIndex.FindNearestUVE(
                           UVE::Core::MotionQueryFeatureVectorUVE{{1.0F, 0.0F}, 1.0F}, 1U,
                           nearestCandidates)
                    .IsAcceptedUVE());
    ASSERT_EQ(nearestCandidates, std::vector<std::size_t>{1U});

    const MotionQueryAnimationNodeResultUVE evaluated = EvaluateMotionQueryAnimationNodeUVE(
        MakeQueryUVE(), database, schema, searchIndex, MakeClipsUVE(),
        MotionQueryAnimationNodeSettingsUVE{});
    ASSERT_TRUE(evaluated.IsAcceptedUVE()) << evaluated.message;
    EXPECT_EQ(evaluated.candidateIndex, 1U);
    EXPECT_EQ(evaluated.sourceClipId, "walk");
    EXPECT_DOUBLE_EQ(evaluated.sampleTimeSeconds, 0.75);

    events.Publish(Asset::AssetReloadedEventUVE{source.guid});
    EXPECT_EQ(ingestion.GetEntriesUVE().front().state,
              MotionQueryAssetIngestionStateUVE::RebuildRequired);
    EXPECT_EQ(cache.FindUVE(request.key, registry, copiedDerivedData).code,
              MotionQueryDerivedDataCacheCodeUVE::NotFound);
    events.Publish(Asset::AssetLoadCompletedEventUVE{source.guid, true});
    EXPECT_EQ(ingestion.GetEntriesUVE().front().state,
              MotionQueryAssetIngestionStateUVE::AwaitingDerivedData);
    ASSERT_TRUE(ingestion.SubmitDerivedDataUVE(MakeDerivedDataUVE(request, 0.75F)).IsAcceptedUVE());
    EXPECT_EQ(ingestion.GetEntriesUVE().front().state,
              MotionQueryAssetIngestionStateUVE::Ready);
    ASSERT_TRUE(cache.FindUVE(request.key, registry, copiedDerivedData).IsAcceptedUVE());
    EXPECT_FLOAT_EQ(copiedDerivedData.normalizedSamples.front().values.front(), 0.75F);

    searchIndex.ClearUVE();
    ASSERT_TRUE(searchIndex.BuildUVE(database, schema).IsAcceptedUVE());
    const MotionQueryAnimationNodeResultUVE rebuiltEvaluation = EvaluateMotionQueryAnimationNodeUVE(
        MakeQueryUVE(), database, schema, searchIndex, MakeClipsUVE(),
        MotionQueryAnimationNodeSettingsUVE{});
    ASSERT_TRUE(rebuiltEvaluation.IsAcceptedUVE()) << rebuiltEvaluation.message;
    EXPECT_EQ(rebuiltEvaluation.candidateIndex, evaluated.candidateIndex);
    EXPECT_DOUBLE_EQ(rebuiltEvaluation.sampleTimeSeconds, evaluated.sampleTimeSeconds);
}

TEST(MotionQueryRuntimeIntegrationUVETest,
     RuntimeFlowUVE_RequiresFreshRegistrationAfterGenerationReplacement) {
    UVE::Events::EventSystemUVE events;
    MotionQueryAssetRegistryUVE registry;
    MotionQueryDerivedDataCacheUVE cache;
    MotionQueryAssetIngestionCoordinatorUVE ingestion(events, registry, cache);
    const Asset::ResourceHandleUVE source = MakeHandleUVE(501U);
    const Asset::ResourceHandleUVE replacement = MakeHandleUVE(501U, 2U);
    ASSERT_TRUE(registry.RegisterUVE(source).IsAppliedUVE());
    const MotionQueryAssetIngestionRequestUVE request = MakeRequestUVE(source);
    ASSERT_TRUE(ingestion.RegisterUVE(request).IsAcceptedUVE());
    ASSERT_TRUE(ingestion.SubmitDerivedDataUVE(MakeDerivedDataUVE(request, 0.5F)).IsAcceptedUVE());

    ASSERT_EQ(registry.UpdateGenerationUVE(source, replacement).code,
              Asset::ResourceDependencyCodeUVE::Updated);
    MotionQueryDerivedDataUVE copiedDerivedData;
    EXPECT_EQ(cache.FindUVE(request.key, registry, copiedDerivedData).code,
              MotionQueryDerivedDataCacheCodeUVE::SourceNotRegistered);
    EXPECT_EQ(ingestion.SubmitDerivedDataUVE(MakeDerivedDataUVE(request, 0.5F)).code,
              MotionQueryAssetIngestionCodeUVE::CacheRejected);
    ASSERT_TRUE(ingestion.RemoveUVE(source).IsAcceptedUVE());

    MotionQueryAssetIngestionRequestUVE replacementRequest = MakeRequestUVE(replacement);
    EXPECT_TRUE(ingestion.RegisterUVE(std::move(replacementRequest)).IsAcceptedUVE());
    const auto entries = ingestion.GetEntriesUVE();
    ASSERT_EQ(entries.size(), 1U);
    EXPECT_TRUE(registry.ContainsUVE(replacement));
    EXPECT_FALSE(registry.ContainsUVE(source));
}
} // namespace UVE::Plugins
