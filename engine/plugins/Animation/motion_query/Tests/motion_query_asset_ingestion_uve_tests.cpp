// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/events/event_system_uve.h"
#include "uve/plugins/motion_query_asset_ingestion_uve.h"

#include <gtest/gtest.h>

namespace UVE::Plugins {
namespace {
Asset::ResourceHandleUVE MakeHandleUVE(const std::uint64_t guid, const std::uint64_t generation = 1U) {
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

MotionQueryDerivedDataUVE MakeDerivedUVE(const MotionQueryAssetIngestionRequestUVE& request) {
    MotionQueryDerivedDataUVE data;
    data.key = request.key;
    UVE::Core::MotionQueryFeatureVectorUVE sample;
    sample.values = {0.25F, 0.75F};
    sample.totalWeight = 1.0F;
    data.normalizedSamples = {sample};
    return data;
}
} // namespace

TEST(MotionQueryAssetIngestionUVETest, CoordinatorUVE_RebuildsAfterReloadAndLoadCompletion) {
    UVE::Events::EventSystemUVE events;
    MotionQueryAssetRegistryUVE registry;
    MotionQueryDerivedDataCacheUVE cache;
    const Asset::ResourceHandleUVE source = MakeHandleUVE(1U);
    ASSERT_TRUE(registry.RegisterUVE(source).IsAppliedUVE());
    MotionQueryAssetIngestionCoordinatorUVE coordinator(events, registry, cache);
    const MotionQueryAssetIngestionRequestUVE request = MakeRequestUVE(source);
    ASSERT_TRUE(coordinator.RegisterUVE(request).IsAcceptedUVE());

    events.Publish(Asset::AssetReloadedEventUVE{source.guid});
    ASSERT_EQ(coordinator.GetEntriesUVE().front().state,
              MotionQueryAssetIngestionStateUVE::RebuildRequired);
    events.Publish(Asset::AssetLoadCompletedEventUVE{source.guid, true});
    ASSERT_EQ(coordinator.GetEntriesUVE().front().state,
              MotionQueryAssetIngestionStateUVE::AwaitingDerivedData);

    ASSERT_TRUE(coordinator.SubmitDerivedDataUVE(MakeDerivedUVE(request)).IsAcceptedUVE());
    const auto entries = coordinator.GetEntriesUVE();
    ASSERT_EQ(entries.front().state, MotionQueryAssetIngestionStateUVE::Ready);
    EXPECT_EQ(entries.front().rebuildCount, 1U);
    MotionQueryDerivedDataUVE copied;
    EXPECT_TRUE(cache.FindUVE(request.key, registry, copied).IsAcceptedUVE());
}

TEST(MotionQueryAssetIngestionUVETest, CoordinatorUVE_RecordsLoadFailureAndRejectsMismatchedData) {
    UVE::Events::EventSystemUVE events;
    MotionQueryAssetRegistryUVE registry;
    MotionQueryDerivedDataCacheUVE cache;
    const Asset::ResourceHandleUVE source = MakeHandleUVE(2U);
    ASSERT_TRUE(registry.RegisterUVE(source).IsAppliedUVE());
    MotionQueryAssetIngestionCoordinatorUVE coordinator(events, registry, cache);
    const MotionQueryAssetIngestionRequestUVE request = MakeRequestUVE(source);
    ASSERT_TRUE(coordinator.RegisterUVE(request).IsAcceptedUVE());
    events.Publish(Asset::AssetReloadedEventUVE{source.guid});
    events.Publish(Asset::AssetLoadCompletedEventUVE{source.guid, false});
    EXPECT_EQ(coordinator.GetEntriesUVE().front().state,
              MotionQueryAssetIngestionStateUVE::ReloadFailed);

    MotionQueryDerivedDataUVE mismatched = MakeDerivedUVE(request);
    mismatched.key.samplerVersion = 99U;
    EXPECT_EQ(coordinator.SubmitDerivedDataUVE(std::move(mismatched)).code,
              MotionQueryAssetIngestionCodeUVE::InvalidRequest);
}

TEST(MotionQueryAssetIngestionUVETest, CoordinatorUVE_RespectsRebuildOptOutAndRAIIUnsubscribes) {
    UVE::Events::EventSystemUVE events;
    MotionQueryAssetRegistryUVE registry;
    MotionQueryDerivedDataCacheUVE cache;
    const Asset::ResourceHandleUVE source = MakeHandleUVE(3U);
    ASSERT_TRUE(registry.RegisterUVE(source).IsAppliedUVE());
    {
        MotionQueryAssetIngestionCoordinatorUVE coordinator(events, registry, cache);
        MotionQueryAssetIngestionRequestUVE request = MakeRequestUVE(source);
        request.rebuildOnReload = false;
        ASSERT_TRUE(coordinator.RegisterUVE(request).IsAcceptedUVE());
        events.Publish(Asset::AssetReloadedEventUVE{source.guid});
        EXPECT_EQ(coordinator.GetEntriesUVE().front().state,
                  MotionQueryAssetIngestionStateUVE::Registered);
    }
    events.Publish(Asset::AssetReloadedEventUVE{source.guid});
    events.Publish(Asset::AssetLoadCompletedEventUVE{source.guid, false});
}

TEST(MotionQueryAssetIngestionUVETest, CoordinatorUVE_RejectsUnknownSourcesAndSupportsRemoval) {
    UVE::Events::EventSystemUVE events;
    MotionQueryAssetRegistryUVE registry;
    MotionQueryDerivedDataCacheUVE cache;
    MotionQueryAssetIngestionCoordinatorUVE coordinator(events, registry, cache);
    const Asset::ResourceHandleUVE source = MakeHandleUVE(4U);
    EXPECT_EQ(coordinator.RegisterUVE(MakeRequestUVE(source)).code,
              MotionQueryAssetIngestionCodeUVE::SourceNotRegistered);
    ASSERT_TRUE(registry.RegisterUVE(source).IsAppliedUVE());
    ASSERT_TRUE(coordinator.RegisterUVE(MakeRequestUVE(source)).IsAcceptedUVE());
    EXPECT_TRUE(coordinator.RemoveUVE(source).IsAcceptedUVE());
    EXPECT_EQ(coordinator.RemoveUVE(source).code, MotionQueryAssetIngestionCodeUVE::RequestNotFound);
}
} // namespace UVE::Plugins
