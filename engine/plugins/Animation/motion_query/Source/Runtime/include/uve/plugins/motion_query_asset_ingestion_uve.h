// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/asset/asset_load_completed_event_uve.h"
#include "uve/asset/asset_reloaded_event_uve.h"
#include "uve/events/event_subscription_uve.h"
#include "uve/events/i_event_system_uve.h"
#include "uve/plugins/motion_query_asset_registry_cache_uve.h"
#include "uve/plugins/motion_query_schema_compatibility_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace UVE::Plugins {

enum class MotionQueryAssetIngestionStateUVE : std::uint8_t {
    Registered = 0,
    RebuildRequired,
    AwaitingDerivedData,
    Ready,
    ReloadFailed,
    Invalidated,
};

struct MotionQueryAssetIngestionRequestUVE final {
    Asset::ResourceHandleUVE source;
    MotionQueryDerivedDataKeyUVE key;
    bool rebuildOnReload = true;
};

struct MotionQueryAssetIngestionEntryUVE final {
    MotionQueryAssetIngestionRequestUVE request;
    MotionQueryAssetIngestionStateUVE state = MotionQueryAssetIngestionStateUVE::Registered;
    std::uint64_t generation = 0U;
    std::uint64_t lastReloadSequence = 0U;
    std::size_t rebuildCount = 0U;
    std::string diagnostic;

};

enum class MotionQueryAssetIngestionCodeUVE : std::uint8_t {
    Accepted = 0,
    Updated,
    InvalidRequest,
    SourceNotRegistered,
    DuplicateRequest,
    RequestNotFound,
    CacheRejected,
    SchemaVersionMismatch,
    FeatureDimensionMismatch,
};

struct MotionQueryAssetIngestionResultUVE final {
    MotionQueryAssetIngestionCodeUVE code = MotionQueryAssetIngestionCodeUVE::InvalidRequest;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == MotionQueryAssetIngestionCodeUVE::Accepted ||
               code == MotionQueryAssetIngestionCodeUVE::Updated;
    }
};

class MotionQueryAssetIngestionCoordinatorUVE final {
public:
    static constexpr std::size_t kMaximumRequestsUVE = 1024U;

    MotionQueryAssetIngestionCoordinatorUVE(
        Events::IEventSystemUVE& eventSystem,
        MotionQueryAssetRegistryUVE& registry,
        MotionQueryDerivedDataCacheUVE& cache);
    ~MotionQueryAssetIngestionCoordinatorUVE();

    MotionQueryAssetIngestionCoordinatorUVE(const MotionQueryAssetIngestionCoordinatorUVE&) = delete;
    MotionQueryAssetIngestionCoordinatorUVE& operator=(const MotionQueryAssetIngestionCoordinatorUVE&) = delete;

    [[nodiscard]] MotionQueryAssetIngestionResultUVE RegisterUVE(
        MotionQueryAssetIngestionRequestUVE request) noexcept;
    [[nodiscard]] MotionQueryAssetIngestionResultUVE RemoveUVE(
        Asset::ResourceHandleUVE source) noexcept;
    [[nodiscard]] MotionQueryAssetIngestionResultUVE SubmitDerivedDataUVE(
        MotionQueryDerivedDataUVE derivedData) noexcept;
    [[nodiscard]] MotionQueryAssetIngestionResultUVE SubmitDerivedDataUVE(
        MotionQueryDerivedDataUVE derivedData,
        const UVE::Core::MotionQueryFeatureSchemaUVE& schema) noexcept;
    [[nodiscard]] MotionQueryAssetIngestionResultUVE MarkRebuiltUVE(
        Asset::ResourceHandleUVE source) noexcept;
    [[nodiscard]] std::vector<MotionQueryAssetIngestionEntryUVE> GetEntriesUVE() const;

private:
    void OnAssetReloadedUVE(const Asset::AssetReloadedEventUVE& event) noexcept;
    void OnAssetLoadCompletedUVE(const Asset::AssetLoadCompletedEventUVE& event) noexcept;
    [[nodiscard]] MotionQueryAssetIngestionResultUVE SubmitDerivedDataInternalUVE(
        MotionQueryDerivedDataUVE derivedData,
        const UVE::Core::MotionQueryFeatureSchemaUVE* schema) noexcept;
    [[nodiscard]] static bool IsValidHandleUVE(Asset::ResourceHandleUVE handle) noexcept;
    [[nodiscard]] MotionQueryAssetIngestionEntryUVE* FindUVE(
        Asset::ResourceHandleUVE source) noexcept;
    [[nodiscard]] const MotionQueryAssetIngestionEntryUVE* FindUVE(
        Asset::ResourceHandleUVE source) const noexcept;

    Events::IEventSystemUVE& eventSystem_;
    MotionQueryAssetRegistryUVE& registry_;
    MotionQueryDerivedDataCacheUVE& cache_;
    Events::EventSubscriptionUVE reloadedSubscription_;
    Events::EventSubscriptionUVE loadCompletedSubscription_;
    std::vector<MotionQueryAssetIngestionEntryUVE> entries_;
    std::uint64_t eventSequence_ = 0U;
};

} // namespace UVE::Plugins
