// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <filesystem>
#include <memory>

#include "uve/asset/i_derived_artifact_cache_uve.h"

namespace UVE::Asset {

/// Engine-standard project-local metadata cache for import-derived artifacts. Cache records are
/// JSON metadata files under `cacheRoot`; source and destination paths are never created, written,
/// renamed, or removed by this service. Thread-safe: public operations serialize cache-file access.
class DerivedArtifactCacheUVE final : public IDerivedArtifactCacheUVE {
public:
    explicit DerivedArtifactCacheUVE(std::filesystem::path cacheRoot);
    ~DerivedArtifactCacheUVE() override;

    DerivedArtifactCacheUVE(const DerivedArtifactCacheUVE&) = delete;
    DerivedArtifactCacheUVE& operator=(const DerivedArtifactCacheUVE&) = delete;

    [[nodiscard]] std::optional<DerivedArtifactCacheRecordUVE>
    LoadImportRecordUVE(const std::filesystem::path& destinationPath) const override;
    [[nodiscard]] bool StoreImportRecordUVE(const std::filesystem::path& destinationPath,
                                             const DerivedArtifactCacheRecordUVE& record) override;
    [[nodiscard]] std::size_t MarkStaleForSourceUVE(const std::filesystem::path& sourcePath) override;
    [[nodiscard]] std::filesystem::path GetCacheRootUVE() const override;

private:
    struct ImplUVE;
    std::unique_ptr<ImplUVE> m_impl;
};

} // namespace UVE::Asset
