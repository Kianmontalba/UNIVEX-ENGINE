// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/asset/resource_dependency_graph_uve.h"
#include "uve/plugins/motion_query_asset_sampling_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace UVE::Plugins {

inline constexpr std::size_t kMotionQueryMaximumRegisteredAssetsUVE = 1024U;
inline constexpr std::size_t kMotionQueryMaximumCachedDerivedEntriesUVE = 256U;
inline constexpr std::size_t kMotionQueryMaximumCachedFeatureValuesUVE = 1048576U;

class MotionQueryAssetRegistryUVE final {
public:
    [[nodiscard]] Asset::ResourceDependencyResultUVE RegisterUVE(
        Asset::ResourceHandleUVE handle) noexcept;
    [[nodiscard]] Asset::ResourceDependencyResultUVE SetDependenciesUVE(
        Asset::ResourceHandleUVE handle,
        std::vector<Asset::ResourceHandleUVE> dependencies) noexcept;
    [[nodiscard]] Asset::ResourceDependencyResultUVE RemoveUVE(
        Asset::ResourceHandleUVE handle) noexcept;
    [[nodiscard]] Asset::ResourceDependencyResultUVE UpdateGenerationUVE(
        Asset::ResourceHandleUVE previous,
        Asset::ResourceHandleUVE replacement) noexcept;
    [[nodiscard]] bool ContainsUVE(Asset::ResourceHandleUVE handle) const noexcept;
    [[nodiscard]] bool TryGetSnapshotUVE(
        Asset::ResourceHandleUVE handle,
        Asset::ResourceDependencySnapshotUVE& destination) const noexcept;
    [[nodiscard]] Asset::ResourceDependencySnapshotUVE GetSnapshotUVE() const;

private:
    Asset::ResourceDependencyGraphUVE graph_;
};

enum class MotionQueryDerivedDataCacheCodeUVE : std::uint8_t {
    Accepted = 0,
    Updated,
    Evicted,
    InvalidDerivedData,
    SourceNotRegistered,
    NotFound,
    StaleSource,
    CapacityExceeded,
};

struct MotionQueryDerivedDataCacheResultUVE final {
    MotionQueryDerivedDataCacheCodeUVE code = MotionQueryDerivedDataCacheCodeUVE::InvalidDerivedData;
    std::size_t evictedEntries = 0U;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == MotionQueryDerivedDataCacheCodeUVE::Accepted ||
               code == MotionQueryDerivedDataCacheCodeUVE::Updated ||
               code == MotionQueryDerivedDataCacheCodeUVE::Evicted;
    }
};

struct MotionQueryDerivedDataCacheSnapshotUVE final {
    std::uint64_t generation = 0U;
    std::size_t entryCount = 0U;
    std::size_t featureValueCount = 0U;
    std::size_t maximumEntries = kMotionQueryMaximumCachedDerivedEntriesUVE;
    std::size_t maximumFeatureValues = kMotionQueryMaximumCachedFeatureValuesUVE;

    [[nodiscard]] bool operator==(const MotionQueryDerivedDataCacheSnapshotUVE&) const = default;
};

class MotionQueryDerivedDataCacheUVE final {
public:
    [[nodiscard]] MotionQueryDerivedDataCacheResultUVE RegisterUVE(
        const MotionQueryDerivedDataUVE& derivedData,
        const MotionQueryAssetRegistryUVE& registry) noexcept;
    [[nodiscard]] MotionQueryDerivedDataCacheResultUVE FindUVE(
        MotionQueryDerivedDataKeyUVE key,
        const MotionQueryAssetRegistryUVE& registry,
        MotionQueryDerivedDataUVE& destination) const noexcept;
    [[nodiscard]] MotionQueryDerivedDataCacheResultUVE InvalidateSourceUVE(
        Asset::ResourceHandleUVE source) noexcept;
    void ClearUVE() noexcept;
    [[nodiscard]] MotionQueryDerivedDataCacheSnapshotUVE GetSnapshotUVE() const noexcept;

private:
    struct EntryUVE final {
        MotionQueryDerivedDataUVE data;
        std::size_t featureValueCount = 0U;
        std::uint64_t dependencyGraphGeneration = 0U;
    };

    [[nodiscard]] static bool IsValidHandleUVE(Asset::ResourceHandleUVE handle) noexcept;
    [[nodiscard]] static bool IsValidDerivedDataUVE(
        const MotionQueryDerivedDataUVE& derivedData,
        std::size_t& featureValueCount) noexcept;
    [[nodiscard]] static bool IsSameKeyUVE(const MotionQueryDerivedDataKeyUVE& lhs,
                                            const MotionQueryDerivedDataKeyUVE& rhs) noexcept;

    std::vector<EntryUVE> entries_;
    std::size_t featureValueCount_ = 0U;
    std::uint64_t generation_ = 0U;
};

} // namespace UVE::Plugins
