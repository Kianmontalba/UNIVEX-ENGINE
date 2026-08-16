// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_asset_registry_cache_uve.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace UVE::Plugins {
namespace {
[[nodiscard]] MotionQueryDerivedDataCacheResultUVE MakeCacheResultUVE(
    const MotionQueryDerivedDataCacheCodeUVE code, const std::size_t evicted,
    const char* message) noexcept {
    return MotionQueryDerivedDataCacheResultUVE{code, evicted, message};
}

[[nodiscard]] bool IsValidResourceHandleUVE(const Asset::ResourceHandleUVE handle) noexcept {
    return handle.guid != Asset::kInvalidAssetGuidUVE && handle.generation != 0U;
}
} // namespace

Asset::ResourceDependencyResultUVE MotionQueryAssetRegistryUVE::RegisterUVE(
    const Asset::ResourceHandleUVE handle) noexcept {
    if (!graph_.HasResourceUVE(handle) &&
        graph_.GetSnapshotUVE().entries.size() >= kMotionQueryMaximumRegisteredAssetsUVE) {
        return {Asset::ResourceDependencyCodeUVE::CapacityExceeded,
                "Motion Query asset registry capacity has been reached."};
    }
    return graph_.RegisterResourceUVE(handle);
}

Asset::ResourceDependencyResultUVE MotionQueryAssetRegistryUVE::SetDependenciesUVE(
    const Asset::ResourceHandleUVE handle,
    std::vector<Asset::ResourceHandleUVE> dependencies) noexcept {
    return graph_.SetDependenciesUVE(handle, std::move(dependencies));
}

Asset::ResourceDependencyResultUVE MotionQueryAssetRegistryUVE::RemoveUVE(
    const Asset::ResourceHandleUVE handle) noexcept {
    return graph_.RemoveResourceUVE(handle);
}

Asset::ResourceDependencyResultUVE MotionQueryAssetRegistryUVE::UpdateGenerationUVE(
    const Asset::ResourceHandleUVE previous,
    const Asset::ResourceHandleUVE replacement) noexcept {
    if (!IsValidResourceHandleUVE(previous) || !IsValidResourceHandleUVE(replacement)) {
        return {Asset::ResourceDependencyCodeUVE::InvalidHandle,
                "Motion Query asset generation update requires valid handles."};
    }
    if (previous.guid != replacement.guid || previous == replacement) {
        return {Asset::ResourceDependencyCodeUVE::InvalidHandle,
                "Motion Query asset generation update requires one GUID and a new generation."};
    }
    const Asset::ResourceDependencySnapshotUVE snapshot = graph_.GetSnapshotUVE();
    const auto previousEntry = std::find_if(snapshot.entries.cbegin(), snapshot.entries.cend(),
                                            [previous](const Asset::ResourceDependencyEntryUVE& entry) {
                                                return entry.handle == previous;
                                            });
    if (previousEntry == snapshot.entries.cend()) {
        return {Asset::ResourceDependencyCodeUVE::UnknownDependency,
                "Motion Query generation update referenced an unknown source handle."};
    }
    const std::vector<Asset::ResourceHandleUVE> dependencies = previousEntry->dependencies;
    const Asset::ResourceDependencyResultUVE removed = graph_.RemoveResourceUVE(previous);
    if (!removed.IsAppliedUVE()) {
        return removed;
    }
    const Asset::ResourceDependencyResultUVE registered = graph_.RegisterResourceUVE(replacement);
    if (!registered.IsAppliedUVE()) {
        return registered;
    }
    return graph_.SetDependenciesUVE(replacement, dependencies);
}

bool MotionQueryAssetRegistryUVE::ContainsUVE(const Asset::ResourceHandleUVE handle) const noexcept {
    return graph_.HasResourceUVE(handle);
}

bool MotionQueryAssetRegistryUVE::TryGetSnapshotUVE(
    const Asset::ResourceHandleUVE handle,
    Asset::ResourceDependencySnapshotUVE& destination) const noexcept {
    if (!graph_.HasResourceUVE(handle)) {
        return false;
    }
    destination = graph_.GetSnapshotUVE();
    return true;
}

Asset::ResourceDependencySnapshotUVE MotionQueryAssetRegistryUVE::GetSnapshotUVE() const {
    return graph_.GetSnapshotUVE();
}

MotionQueryDerivedDataCacheResultUVE MotionQueryDerivedDataCacheUVE::RegisterUVE(
    const MotionQueryDerivedDataUVE& derivedData,
    const MotionQueryAssetRegistryUVE& registry) noexcept {
    std::size_t featureValueCount = 0U;
    if (!IsValidDerivedDataUVE(derivedData, featureValueCount)) {
        return MakeCacheResultUVE(MotionQueryDerivedDataCacheCodeUVE::InvalidDerivedData, 0U,
                                 "Motion Query derived data is invalid or non-finite.");
    }
    if (featureValueCount > kMotionQueryMaximumCachedFeatureValuesUVE) {
        return MakeCacheResultUVE(MotionQueryDerivedDataCacheCodeUVE::CapacityExceeded, 0U,
                                 "Motion Query derived data exceeds the cache feature-value capacity.");
    }
    Asset::ResourceDependencySnapshotUVE dependencies;
    if (!registry.TryGetSnapshotUVE(derivedData.key.source, dependencies)) {
        return MakeCacheResultUVE(MotionQueryDerivedDataCacheCodeUVE::SourceNotRegistered, 0U,
                                 "Motion Query derived data source is not registered.");
    }

    const auto existing = std::find_if(entries_.begin(), entries_.end(), [&derivedData](const EntryUVE& entry) {
        return IsSameKeyUVE(entry.data.key, derivedData.key);
    });
    const bool isUpdate = existing != entries_.end();
    const std::size_t projectedValues = featureValueCount_ - (isUpdate ? existing->featureValueCount : 0U) +
                                        featureValueCount;
    if (projectedValues > kMotionQueryMaximumCachedFeatureValuesUVE) {
        return MakeCacheResultUVE(MotionQueryDerivedDataCacheCodeUVE::CapacityExceeded, 0U,
                                 "Motion Query derived data cache feature-value capacity would be exceeded.");
    }

    if (isUpdate) {
        featureValueCount_ -= existing->featureValueCount;
        existing->data = derivedData;
        existing->featureValueCount = featureValueCount;
        existing->dependencyGraphGeneration = dependencies.graphGeneration;
        featureValueCount_ += featureValueCount;
        ++generation_;
        return MakeCacheResultUVE(MotionQueryDerivedDataCacheCodeUVE::Updated, 0U,
                                 "Motion Query derived data cache entry was updated.");
    }

    entries_.push_back(EntryUVE{derivedData, featureValueCount, dependencies.graphGeneration});
    featureValueCount_ += featureValueCount;
    std::size_t evicted = 0U;
    while (entries_.size() > kMotionQueryMaximumCachedDerivedEntriesUVE) {
        featureValueCount_ -= entries_.front().featureValueCount;
        entries_.erase(entries_.begin());
        ++evicted;
    }
    while (featureValueCount_ > kMotionQueryMaximumCachedFeatureValuesUVE && !entries_.empty()) {
        featureValueCount_ -= entries_.front().featureValueCount;
        entries_.erase(entries_.begin());
        ++evicted;
    }
    ++generation_;
    return MakeCacheResultUVE(evicted == 0U ? MotionQueryDerivedDataCacheCodeUVE::Accepted
                                           : MotionQueryDerivedDataCacheCodeUVE::Evicted,
                              evicted, evicted == 0U ? "Motion Query derived data was cached."
                                                     : "Motion Query derived data was cached with FIFO eviction.");
}

MotionQueryDerivedDataCacheResultUVE MotionQueryDerivedDataCacheUVE::FindUVE(
    const MotionQueryDerivedDataKeyUVE key,
    const MotionQueryAssetRegistryUVE& registry,
    MotionQueryDerivedDataUVE& destination) const noexcept {
    if (!IsValidHandleUVE(key.source)) {
        return MakeCacheResultUVE(MotionQueryDerivedDataCacheCodeUVE::InvalidDerivedData, 0U,
                                 "Motion Query cache lookup source handle is invalid.");
    }
    Asset::ResourceDependencySnapshotUVE dependencies;
    if (!registry.TryGetSnapshotUVE(key.source, dependencies)) {
        return MakeCacheResultUVE(MotionQueryDerivedDataCacheCodeUVE::SourceNotRegistered, 0U,
                                 "Motion Query cache lookup source is not registered.");
    }
    const auto entry = std::find_if(entries_.cbegin(), entries_.cend(), [key](const EntryUVE& value) {
        return IsSameKeyUVE(value.data.key, key);
    });
    if (entry == entries_.cend()) {
        return MakeCacheResultUVE(MotionQueryDerivedDataCacheCodeUVE::NotFound, 0U,
                                 "Motion Query derived data cache key was not found.");
    }
    if (entry->dependencyGraphGeneration != dependencies.graphGeneration) {
        return MakeCacheResultUVE(MotionQueryDerivedDataCacheCodeUVE::StaleSource, 0U,
                                 "Motion Query derived data cache entry is stale after dependency changes.");
    }
    destination = entry->data;
    return MakeCacheResultUVE(MotionQueryDerivedDataCacheCodeUVE::Accepted, 0U,
                             "Motion Query derived data cache entry was returned.");
}

MotionQueryDerivedDataCacheResultUVE MotionQueryDerivedDataCacheUVE::InvalidateSourceUVE(
    const Asset::ResourceHandleUVE source) noexcept {
    const auto oldSize = entries_.size();
    entries_.erase(std::remove_if(entries_.begin(), entries_.end(), [source](const EntryUVE& entry) {
        return entry.data.key.source == source;
    }), entries_.end());
    const std::size_t removed = oldSize - entries_.size();
    if (removed == 0U) {
        return MakeCacheResultUVE(MotionQueryDerivedDataCacheCodeUVE::NotFound, 0U,
                                 "Motion Query cache source had no entries to invalidate.");
    }
    featureValueCount_ = 0U;
    for (const EntryUVE& entry : entries_) {
        featureValueCount_ += entry.featureValueCount;
    }
    ++generation_;
    return MakeCacheResultUVE(MotionQueryDerivedDataCacheCodeUVE::Updated, removed,
                             "Motion Query cache source entries were invalidated.");
}

void MotionQueryDerivedDataCacheUVE::ClearUVE() noexcept {
    entries_.clear();
    featureValueCount_ = 0U;
    ++generation_;
}

MotionQueryDerivedDataCacheSnapshotUVE MotionQueryDerivedDataCacheUVE::GetSnapshotUVE() const noexcept {
    return MotionQueryDerivedDataCacheSnapshotUVE{generation_, entries_.size(), featureValueCount_,
                                                  kMotionQueryMaximumCachedDerivedEntriesUVE,
                                                  kMotionQueryMaximumCachedFeatureValuesUVE};
}

bool MotionQueryDerivedDataCacheUVE::IsValidHandleUVE(
    const Asset::ResourceHandleUVE handle) noexcept {
    return IsValidResourceHandleUVE(handle);
}

bool MotionQueryDerivedDataCacheUVE::IsValidDerivedDataUVE(
    const MotionQueryDerivedDataUVE& derivedData,
    std::size_t& featureValueCount) noexcept {
    if (!IsValidHandleUVE(derivedData.key.source) || derivedData.normalizedSamples.empty()) {
        return false;
    }
    const std::size_t dimension = derivedData.normalizedSamples.front().values.size();
    if (dimension == 0U) {
        return false;
    }
    featureValueCount = 0U;
    for (const UVE::Core::MotionQueryFeatureVectorUVE& sample : derivedData.normalizedSamples) {
        if (sample.values.size() != dimension || !std::isfinite(sample.totalWeight) ||
            sample.totalWeight < 0.0F) {
            return false;
        }
        for (const float value : sample.values) {
            if (!std::isfinite(value) || value < 0.0F || value > 1.0F) {
                return false;
            }
        }
        if (featureValueCount > std::numeric_limits<std::size_t>::max() - sample.values.size()) {
            return false;
        }
        featureValueCount += sample.values.size();
    }
    return true;
}

bool MotionQueryDerivedDataCacheUVE::IsSameKeyUVE(
    const MotionQueryDerivedDataKeyUVE& lhs,
    const MotionQueryDerivedDataKeyUVE& rhs) noexcept {
    return lhs == rhs;
}

} // namespace UVE::Plugins
