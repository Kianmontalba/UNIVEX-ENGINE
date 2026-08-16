// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_asset_ingestion_uve.h"

#include <algorithm>
#include <utility>

namespace UVE::Plugins {
namespace {
[[nodiscard]] MotionQueryAssetIngestionResultUVE MakeIngestionResultUVE(
    const MotionQueryAssetIngestionCodeUVE code, const char* message) noexcept {
    return MotionQueryAssetIngestionResultUVE{code, message};
}

[[nodiscard]] bool SameKeyUVE(const MotionQueryDerivedDataKeyUVE& lhs,
                              const MotionQueryDerivedDataKeyUVE& rhs) noexcept {
    return lhs == rhs;
}
} // namespace

MotionQueryAssetIngestionCoordinatorUVE::MotionQueryAssetIngestionCoordinatorUVE(
    Events::IEventSystemUVE& eventSystem,
    MotionQueryAssetRegistryUVE& registry,
    MotionQueryDerivedDataCacheUVE& cache)
    : eventSystem_(eventSystem), registry_(registry), cache_(cache) {
    reloadedSubscription_ = eventSystem_.Subscribe<Asset::AssetReloadedEventUVE>(
        [this](const Asset::AssetReloadedEventUVE& event) { OnAssetReloadedUVE(event); });
    loadCompletedSubscription_ = eventSystem_.Subscribe<Asset::AssetLoadCompletedEventUVE>(
        [this](const Asset::AssetLoadCompletedEventUVE& event) { OnAssetLoadCompletedUVE(event); });
}

MotionQueryAssetIngestionCoordinatorUVE::~MotionQueryAssetIngestionCoordinatorUVE() {
    if (reloadedSubscription_.IsValidUVE()) {
        eventSystem_.Unsubscribe(reloadedSubscription_);
    }
    if (loadCompletedSubscription_.IsValidUVE()) {
        eventSystem_.Unsubscribe(loadCompletedSubscription_);
    }
}

MotionQueryAssetIngestionResultUVE MotionQueryAssetIngestionCoordinatorUVE::RegisterUVE(
    MotionQueryAssetIngestionRequestUVE request) noexcept {
    if (!IsValidHandleUVE(request.source) || request.key.source != request.source) {
        return MakeIngestionResultUVE(MotionQueryAssetIngestionCodeUVE::InvalidRequest,
                                      "Motion Query ingestion request has an invalid or mismatched source handle.");
    }
    if (!registry_.ContainsUVE(request.source)) {
        return MakeIngestionResultUVE(MotionQueryAssetIngestionCodeUVE::SourceNotRegistered,
                                      "Motion Query ingestion source is not registered.");
    }
    MotionQueryAssetIngestionEntryUVE* existing = FindUVE(request.source);
    if (existing != nullptr) {
        existing->request = request;
        existing->state = MotionQueryAssetIngestionStateUVE::Registered;
        existing->diagnostic = "Motion Query ingestion request was updated.";
        ++existing->generation;
        return MakeIngestionResultUVE(MotionQueryAssetIngestionCodeUVE::Updated,
                                      "Motion Query ingestion request was updated.");
    }
    if (entries_.size() >= kMaximumRequestsUVE) {
        return MakeIngestionResultUVE(MotionQueryAssetIngestionCodeUVE::InvalidRequest,
                                      "Motion Query ingestion request capacity has been reached.");
    }
    entries_.push_back(MotionQueryAssetIngestionEntryUVE{request,
                                                          MotionQueryAssetIngestionStateUVE::Registered,
                                                          1U, 0U, 0U,
                                                          "Motion Query ingestion request was registered."});
    return MakeIngestionResultUVE(MotionQueryAssetIngestionCodeUVE::Accepted,
                                  "Motion Query ingestion request was registered.");
}

MotionQueryAssetIngestionResultUVE MotionQueryAssetIngestionCoordinatorUVE::RemoveUVE(
    const Asset::ResourceHandleUVE source) noexcept {
    const auto iterator = std::find_if(entries_.begin(), entries_.end(), [source](const auto& entry) {
        return entry.request.source == source;
    });
    if (iterator == entries_.end()) {
        return MakeIngestionResultUVE(MotionQueryAssetIngestionCodeUVE::RequestNotFound,
                                      "Motion Query ingestion request was not found.");
    }
    static_cast<void>(cache_.InvalidateSourceUVE(source));
    entries_.erase(iterator);
    return MakeIngestionResultUVE(MotionQueryAssetIngestionCodeUVE::Accepted,
                                  "Motion Query ingestion request was removed.");
}

MotionQueryAssetIngestionResultUVE MotionQueryAssetIngestionCoordinatorUVE::SubmitDerivedDataUVE(
    MotionQueryDerivedDataUVE derivedData) noexcept {
    return SubmitDerivedDataInternalUVE(std::move(derivedData), nullptr);
}

MotionQueryAssetIngestionResultUVE MotionQueryAssetIngestionCoordinatorUVE::SubmitDerivedDataUVE(
    MotionQueryDerivedDataUVE derivedData,
    const UVE::Core::MotionQueryFeatureSchemaUVE& schema) noexcept {
    return SubmitDerivedDataInternalUVE(std::move(derivedData), &schema);
}

MotionQueryAssetIngestionResultUVE
MotionQueryAssetIngestionCoordinatorUVE::SubmitDerivedDataInternalUVE(
    MotionQueryDerivedDataUVE derivedData,
    const UVE::Core::MotionQueryFeatureSchemaUVE* schema) noexcept {
    if (schema != nullptr) {
        const MotionQuerySchemaCompatibilityResultUVE compatibility =
            ValidateMotionQueryDerivedDataSchemaUVE(derivedData, *schema);
        if (!compatibility.IsCompatibleUVE()) {
            const MotionQueryAssetIngestionCodeUVE code =
                compatibility.code == MotionQuerySchemaCompatibilityCodeUVE::SchemaVersionMismatch
                    ? MotionQueryAssetIngestionCodeUVE::SchemaVersionMismatch
                    : compatibility.code == MotionQuerySchemaCompatibilityCodeUVE::FeatureDimensionMismatch
                          ? MotionQueryAssetIngestionCodeUVE::FeatureDimensionMismatch
                          : MotionQueryAssetIngestionCodeUVE::InvalidRequest;
            return MotionQueryAssetIngestionResultUVE{code, compatibility.message};
        }
    }
    MotionQueryAssetIngestionEntryUVE* entry = FindUVE(derivedData.key.source);
    if (entry == nullptr) {
        return MakeIngestionResultUVE(MotionQueryAssetIngestionCodeUVE::RequestNotFound,
                                      "Motion Query derived data source has no ingestion request.");
    }
    if (!SameKeyUVE(derivedData.key, entry->request.key)) {
        return MakeIngestionResultUVE(MotionQueryAssetIngestionCodeUVE::InvalidRequest,
                                      "Motion Query derived data key does not match the ingestion request.");
    }
    const MotionQueryDerivedDataCacheResultUVE cached = cache_.RegisterUVE(derivedData, registry_);
    if (!cached.IsAcceptedUVE()) {
        entry->state = MotionQueryAssetIngestionStateUVE::ReloadFailed;
        entry->diagnostic = cached.message;
        ++entry->generation;
        return MakeIngestionResultUVE(MotionQueryAssetIngestionCodeUVE::CacheRejected,
                                      cached.message.c_str());
    }
    entry->state = MotionQueryAssetIngestionStateUVE::Ready;
    ++entry->rebuildCount;
    ++entry->generation;
    entry->diagnostic = "Motion Query derived data was accepted by the cache.";
    return MakeIngestionResultUVE(MotionQueryAssetIngestionCodeUVE::Accepted,
                                  "Motion Query derived data was accepted by the cache.");
}

MotionQueryAssetIngestionResultUVE MotionQueryAssetIngestionCoordinatorUVE::MarkRebuiltUVE(
    const Asset::ResourceHandleUVE source) noexcept {
    MotionQueryAssetIngestionEntryUVE* entry = FindUVE(source);
    if (entry == nullptr) {
        return MakeIngestionResultUVE(MotionQueryAssetIngestionCodeUVE::RequestNotFound,
                                      "Motion Query ingestion request was not found.");
    }
    entry->state = MotionQueryAssetIngestionStateUVE::AwaitingDerivedData;
    ++entry->generation;
    entry->diagnostic = "Motion Query rebuild is awaiting sampled derived data.";
    return MakeIngestionResultUVE(MotionQueryAssetIngestionCodeUVE::Updated,
                                  "Motion Query rebuild is awaiting sampled derived data.");
}

std::vector<MotionQueryAssetIngestionEntryUVE>
MotionQueryAssetIngestionCoordinatorUVE::GetEntriesUVE() const {
    std::vector<MotionQueryAssetIngestionEntryUVE> result = entries_;
    std::sort(result.begin(), result.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.request.source.guid.value != rhs.request.source.guid.value) {
            return lhs.request.source.guid.value < rhs.request.source.guid.value;
        }
        return lhs.request.source.generation < rhs.request.source.generation;
    });
    return result;
}

void MotionQueryAssetIngestionCoordinatorUVE::OnAssetReloadedUVE(
    const Asset::AssetReloadedEventUVE& event) noexcept {
    for (MotionQueryAssetIngestionEntryUVE& entry : entries_) {
        if (entry.request.source.guid != event.guid || !entry.request.rebuildOnReload) {
            continue;
        }
        static_cast<void>(cache_.InvalidateSourceUVE(entry.request.source));
        entry.state = MotionQueryAssetIngestionStateUVE::RebuildRequired;
        entry.lastReloadSequence = ++eventSequence_;
        ++entry.generation;
        entry.diagnostic = "Motion Query asset reload invalidated derived data; rebuild is required.";
    }
}

void MotionQueryAssetIngestionCoordinatorUVE::OnAssetLoadCompletedUVE(
    const Asset::AssetLoadCompletedEventUVE& event) noexcept {
    for (MotionQueryAssetIngestionEntryUVE& entry : entries_) {
        if (entry.request.source.guid != event.guid) {
            continue;
        }
        if (!event.success) {
            entry.state = MotionQueryAssetIngestionStateUVE::ReloadFailed;
            ++entry.generation;
            entry.diagnostic = "Motion Query asset load failed; derived data remains unavailable.";
            continue;
        }
        if (entry.state == MotionQueryAssetIngestionStateUVE::RebuildRequired) {
            entry.state = MotionQueryAssetIngestionStateUVE::AwaitingDerivedData;
            ++entry.generation;
            entry.diagnostic = "Motion Query asset load completed; awaiting derived-data rebuild.";
        }
    }
}

bool MotionQueryAssetIngestionCoordinatorUVE::IsValidHandleUVE(
    const Asset::ResourceHandleUVE handle) noexcept {
    return handle.guid != Asset::kInvalidAssetGuidUVE && handle.generation != 0U;
}

MotionQueryAssetIngestionEntryUVE* MotionQueryAssetIngestionCoordinatorUVE::FindUVE(
    const Asset::ResourceHandleUVE source) noexcept {
    const auto iterator = std::find_if(entries_.begin(), entries_.end(), [source](const auto& entry) {
        return entry.request.source == source;
    });
    return iterator == entries_.end() ? nullptr : &*iterator;
}

const MotionQueryAssetIngestionEntryUVE* MotionQueryAssetIngestionCoordinatorUVE::FindUVE(
    const Asset::ResourceHandleUVE source) const noexcept {
    const auto iterator = std::find_if(entries_.cbegin(), entries_.cend(), [source](const auto& entry) {
        return entry.request.source == source;
    });
    return iterator == entries_.cend() ? nullptr : &*iterator;
}

} // namespace UVE::Plugins
