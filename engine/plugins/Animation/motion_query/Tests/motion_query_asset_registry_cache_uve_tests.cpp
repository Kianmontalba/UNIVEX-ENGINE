// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_asset_registry_cache_uve.h"

#include <gtest/gtest.h>

namespace UVE::Plugins {
namespace {
Asset::ResourceHandleUVE MakeHandleUVE(const std::uint64_t guid, const std::uint64_t generation = 1U) {
    return Asset::ResourceHandleUVE{Asset::AssetGuidUVE{guid}, generation};
}

MotionQueryDerivedDataUVE MakeDerivedUVE(const Asset::ResourceHandleUVE source, const float value = 0.5F) {
    MotionQueryDerivedDataUVE data;
    data.key.source = source;
    data.key.schemaVersion = 2U;
    data.key.samplerVersion = 3U;
    data.key.normalizationVersion = 4U;
    UVE::Core::MotionQueryFeatureVectorUVE sample;
    sample.values = {value, 1.0F - value};
    sample.totalWeight = 1.0F;
    data.normalizedSamples = {sample};
    return data;
}
} // namespace

TEST(MotionQueryAssetRegistryCacheUVETest, RegistryUVE_TracksExactGenerationsAndDependencies) {
    MotionQueryAssetRegistryUVE registry;
    const Asset::ResourceHandleUVE source = MakeHandleUVE(1U);
    const Asset::ResourceHandleUVE dependency = MakeHandleUVE(2U);
    ASSERT_TRUE(registry.RegisterUVE(source).IsAppliedUVE());
    ASSERT_TRUE(registry.RegisterUVE(dependency).IsAppliedUVE());
    ASSERT_TRUE(registry.SetDependenciesUVE(source, {dependency}).IsAppliedUVE());
    EXPECT_TRUE(registry.ContainsUVE(source));
    EXPECT_FALSE(registry.ContainsUVE(MakeHandleUVE(1U, 2U)));

    EXPECT_EQ(registry.UpdateGenerationUVE(source, MakeHandleUVE(1U, 2U)).code,
              Asset::ResourceDependencyCodeUVE::Updated);
    EXPECT_FALSE(registry.ContainsUVE(source));
    EXPECT_TRUE(registry.ContainsUVE(MakeHandleUVE(1U, 2U)));
    EXPECT_EQ(registry.UpdateGenerationUVE(dependency, MakeHandleUVE(2U, 2U)).code,
              Asset::ResourceDependencyCodeUVE::HasDependents);
}

TEST(MotionQueryAssetRegistryCacheUVETest, CacheUVE_ReturnsCurrentDataAndRejectsGraphStaleness) {
    MotionQueryAssetRegistryUVE registry;
    const Asset::ResourceHandleUVE source = MakeHandleUVE(10U);
    const Asset::ResourceHandleUVE dependency = MakeHandleUVE(11U);
    ASSERT_TRUE(registry.RegisterUVE(source).IsAppliedUVE());
    ASSERT_TRUE(registry.RegisterUVE(dependency).IsAppliedUVE());
    MotionQueryDerivedDataCacheUVE cache;
    const MotionQueryDerivedDataUVE derived = MakeDerivedUVE(source);
    ASSERT_TRUE(cache.RegisterUVE(derived, registry).IsAcceptedUVE());
    MotionQueryDerivedDataUVE copied;
    ASSERT_TRUE(cache.FindUVE(derived.key, registry, copied).IsAcceptedUVE());
    EXPECT_EQ(copied, derived);

    ASSERT_TRUE(registry.SetDependenciesUVE(source, {dependency}).IsAppliedUVE());
    EXPECT_EQ(cache.FindUVE(derived.key, registry, copied).code,
              MotionQueryDerivedDataCacheCodeUVE::StaleSource);
    ASSERT_TRUE(cache.RegisterUVE(derived, registry).IsAcceptedUVE());
    EXPECT_TRUE(cache.FindUVE(derived.key, registry, copied).IsAcceptedUVE());
}

TEST(MotionQueryAssetRegistryCacheUVETest, CacheUVE_UpdatesDuplicateKeyAndInvalidatesSource) {
    MotionQueryAssetRegistryUVE registry;
    const Asset::ResourceHandleUVE source = MakeHandleUVE(20U);
    ASSERT_TRUE(registry.RegisterUVE(source).IsAppliedUVE());
    MotionQueryDerivedDataCacheUVE cache;
    ASSERT_TRUE(cache.RegisterUVE(MakeDerivedUVE(source, 0.25F), registry).IsAcceptedUVE());
    EXPECT_EQ(cache.RegisterUVE(MakeDerivedUVE(source, 0.75F), registry).code,
              MotionQueryDerivedDataCacheCodeUVE::Updated);
    MotionQueryDerivedDataUVE copied;
    ASSERT_TRUE(cache.FindUVE(MakeDerivedUVE(source).key, registry, copied).IsAcceptedUVE());
    EXPECT_FLOAT_EQ(copied.normalizedSamples.front().values.front(), 0.75F);
    EXPECT_EQ(cache.InvalidateSourceUVE(source).code, MotionQueryDerivedDataCacheCodeUVE::Updated);
    EXPECT_EQ(cache.GetSnapshotUVE().entryCount, 0U);
}

TEST(MotionQueryAssetRegistryCacheUVETest, CacheUVE_RejectsInvalidNormalizedValues) {
    MotionQueryAssetRegistryUVE registry;
    const Asset::ResourceHandleUVE source = MakeHandleUVE(30U);
    ASSERT_TRUE(registry.RegisterUVE(source).IsAppliedUVE());
    MotionQueryDerivedDataUVE invalid = MakeDerivedUVE(source, 0.5F);
    invalid.normalizedSamples.front().values.front() = 1.5F;
    MotionQueryDerivedDataCacheUVE cache;
    EXPECT_EQ(cache.RegisterUVE(invalid, registry).code,
              MotionQueryDerivedDataCacheCodeUVE::InvalidDerivedData);
}

TEST(MotionQueryAssetRegistryCacheUVETest, CacheUVE_EvictsOldestEntryAtBound) {
    MotionQueryAssetRegistryUVE registry;
    MotionQueryDerivedDataCacheUVE cache;
    for (std::size_t index = 0U; index < kMotionQueryMaximumCachedDerivedEntriesUVE + 1U; ++index) {
        const Asset::ResourceHandleUVE source = MakeHandleUVE(1000U + index);
        ASSERT_TRUE(registry.RegisterUVE(source).IsAppliedUVE());
        const MotionQueryDerivedDataCacheResultUVE result = cache.RegisterUVE(
            MakeDerivedUVE(source), registry);
        ASSERT_TRUE(result.IsAcceptedUVE()) << result.message;
        if (index == kMotionQueryMaximumCachedDerivedEntriesUVE) {
            EXPECT_EQ(result.code, MotionQueryDerivedDataCacheCodeUVE::Evicted);
            EXPECT_EQ(result.evictedEntries, 1U);
        }
    }
    EXPECT_EQ(cache.GetSnapshotUVE().entryCount, kMotionQueryMaximumCachedDerivedEntriesUVE);
    MotionQueryDerivedDataUVE copied;
    EXPECT_EQ(cache.FindUVE(MakeDerivedUVE(MakeHandleUVE(1000U)).key, registry, copied).code,
              MotionQueryDerivedDataCacheCodeUVE::NotFound);
}
} // namespace UVE::Plugins
