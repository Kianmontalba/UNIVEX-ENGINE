// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/events/event_system_uve.h"
#include "uve/plugins/motion_query_animation_node_uve.h"
#include "uve/plugins/motion_query_asset_ingestion_uve.h"
#include "uve/plugins/motion_query_schema_compatibility_uve.h"

#include <gtest/gtest.h>

namespace UVE::Plugins {
namespace {
UVE::Core::MotionQueryFeatureSchemaUVE MakeSchemaUVE() {
    UVE::Core::MotionQueryFeatureSchemaUVE schema;
    schema.version = 7U;
    schema.channels = {
        UVE::Core::MotionQueryFeatureChannelUVE{
            "velocity", UVE::Core::MotionQueryFeatureChannelKindUVE::RootVelocity, 0U, 1.0F},
        UVE::Core::MotionQueryFeatureChannelUVE{
            "facing", UVE::Core::MotionQueryFeatureChannelKindUVE::FacingDirection, 0U, 1.0F},
    };
    return schema;
}

Asset::ResourceHandleUVE MakeHandleUVE() {
    return Asset::ResourceHandleUVE{Asset::AssetGuidUVE{700U}, 1U};
}

MotionQueryDerivedDataUVE MakeDerivedDataUVE(const std::uint32_t schemaVersion,
                                             const std::size_t dimension) {
    MotionQueryDerivedDataUVE data;
    data.key.source = MakeHandleUVE();
    data.key.schemaVersion = schemaVersion;
    data.key.samplerVersion = 2U;
    data.key.normalizationVersion = 3U;
    UVE::Core::MotionQueryFeatureVectorUVE sample;
    sample.values.assign(dimension, 0.5F);
    sample.totalWeight = 1.0F;
    data.normalizedSamples = {sample};
    return data;
}

UVE::Core::MotionMatchingDatabaseUVE MakeDatabaseUVE() {
    UVE::Core::MotionMatchingDatabaseUVE database;
    UVE::Core::MotionMatchingCandidateUVE candidate;
    candidate.candidateId = "candidate";
    candidate.sourceClipId = "missing";
    candidate.sampleTimeSeconds = 0.5;
    candidate.feature.rootVelocity = UVE::Math::Vector3UVE{1.0F, 0.0F, 0.0F};
    candidate.feature.facingDirection = UVE::Math::Vector3UVE{0.0F, 0.0F, 1.0F};
    database.candidates = {candidate};
    return database;
}

UVE::Core::MotionQueryUVE MakeQueryUVE() {
    UVE::Core::MotionQueryUVE query;
    query.rootVelocity = UVE::Math::Vector3UVE{1.0F, 0.0F, 0.0F};
    query.facingDirection = UVE::Math::Vector3UVE{0.0F, 0.0F, 1.0F};
    return query;
}
} // namespace

TEST(MotionQuerySchemaCompatibilityUVETest, ValidatorUVE_AcceptsMatchingVersionAndDimension) {
    const auto schema = MakeSchemaUVE();
    const auto result = ValidateMotionQueryDerivedDataSchemaUVE(
        MakeDerivedDataUVE(schema.version, schema.channels.size()), schema);
    EXPECT_TRUE(result.IsCompatibleUVE());
    EXPECT_EQ(result.code, MotionQuerySchemaCompatibilityCodeUVE::Compatible);
}

TEST(MotionQuerySchemaCompatibilityUVETest, ValidatorUVE_RejectsSchemaVersionMismatch) {
    const auto schema = MakeSchemaUVE();
    const auto result = ValidateMotionQueryDerivedDataSchemaUVE(
        MakeDerivedDataUVE(schema.version + 1U, schema.channels.size()), schema);
    EXPECT_EQ(result.code, MotionQuerySchemaCompatibilityCodeUVE::SchemaVersionMismatch);
    EXPECT_FALSE(result.IsCompatibleUVE());
}

TEST(MotionQuerySchemaCompatibilityUVETest, ValidatorUVE_RejectsFeatureDimensionMismatch) {
    const auto schema = MakeSchemaUVE();
    const auto result = ValidateMotionQueryDerivedDataSchemaUVE(
        MakeDerivedDataUVE(schema.version, schema.channels.size() - 1U), schema);
    EXPECT_EQ(result.code, MotionQuerySchemaCompatibilityCodeUVE::FeatureDimensionMismatch);
    EXPECT_FALSE(result.IsCompatibleUVE());
}

TEST(MotionQuerySchemaCompatibilityUVETest, SearchIndexAndAnimationUVE_RejectChangedSchema) {
    const auto schema = MakeSchemaUVE();
    const auto database = MakeDatabaseUVE();
    MotionQuerySearchIndexUVE index;
    ASSERT_TRUE(index.BuildUVE(database, schema).IsAcceptedUVE());
    EXPECT_TRUE(index.IsCompatibleWithSchemaUVE(schema));
    auto changedSchema = schema;
    changedSchema.version += 1U;
    EXPECT_FALSE(index.IsCompatibleWithSchemaUVE(changedSchema));
    const MotionQueryAnimationNodeResultUVE result = EvaluateMotionQueryAnimationNodeUVE(
        MakeQueryUVE(), database, changedSchema, index, {}, MotionQueryAnimationNodeSettingsUVE{});
    EXPECT_EQ(result.code, MotionQueryAnimationNodeCodeUVE::SchemaMismatch);
}

TEST(MotionQuerySchemaCompatibilityUVETest, IngestionUVE_RejectsMismatchWithoutChangingReadyState) {
    UVE::Events::EventSystemUVE events;
    MotionQueryAssetRegistryUVE registry;
    MotionQueryDerivedDataCacheUVE cache;
    MotionQueryAssetIngestionCoordinatorUVE ingestion(events, registry, cache);
    const Asset::ResourceHandleUVE source = MakeHandleUVE();
    ASSERT_TRUE(registry.RegisterUVE(source).IsAppliedUVE());
    MotionQueryAssetIngestionRequestUVE request;
    request.source = source;
    request.key = MakeDerivedDataUVE(7U, 2U).key;
    ASSERT_TRUE(ingestion.RegisterUVE(request).IsAcceptedUVE());
    const auto schema = MakeSchemaUVE();
    ASSERT_TRUE(ingestion.SubmitDerivedDataUVE(
        MakeDerivedDataUVE(schema.version, schema.channels.size()), schema).IsAcceptedUVE());
    const auto readyBefore = ingestion.GetEntriesUVE().front();
    EXPECT_EQ(ingestion.SubmitDerivedDataUVE(
                  MakeDerivedDataUVE(schema.version + 1U, schema.channels.size()), schema)
                  .code,
              MotionQueryAssetIngestionCodeUVE::SchemaVersionMismatch);
    EXPECT_EQ(ingestion.SubmitDerivedDataUVE(
                  MakeDerivedDataUVE(schema.version, schema.channels.size() - 1U), schema)
                  .code,
              MotionQueryAssetIngestionCodeUVE::FeatureDimensionMismatch);
    const auto readyAfter = ingestion.GetEntriesUVE().front();
    EXPECT_EQ(readyAfter.state, MotionQueryAssetIngestionStateUVE::Ready);
    EXPECT_EQ(readyAfter.generation, readyBefore.generation);
    EXPECT_EQ(readyAfter.rebuildCount, readyBefore.rebuildCount);
}
} // namespace UVE::Plugins
