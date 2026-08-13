// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <memory>

#include "uve/asset/i_asset_database_uve.h"
#include "uve/asset/i_asset_import_queue_uve.h"
#include "uve/asset/i_derived_artifact_cache_uve.h"

namespace UVE::Asset {

/// Engine-standard deterministic v1 import scheduler. It calls the already-registered generic
/// IAssetImporterUVE implementation synchronously from TickUVE(), with no hidden worker jobs or
/// automatic filesystem watches. Thread-safe queue access; callers still control when TickUVE()
/// runs, and EngineCoreUVE invokes it once from the main update pipeline.
class AssetImportQueueUVE final : public IAssetImportQueueUVE {
public:
    AssetImportQueueUVE(IAssetImporterUVE& importer, IAssetDatabaseUVE& assetDatabase,
                        IDerivedArtifactCacheUVE& derivedArtifactCache);
    ~AssetImportQueueUVE() override;

    AssetImportQueueUVE(const AssetImportQueueUVE&) = delete;
    AssetImportQueueUVE& operator=(const AssetImportQueueUVE&) = delete;

    [[nodiscard]] std::optional<AssetImportJobIdUVE> EnqueueUVE(AssetImportRequestUVE request) override;
    [[nodiscard]] bool TickUVE() override;
    [[nodiscard]] bool RetryUVE(AssetImportJobIdUVE id) override;
    [[nodiscard]] std::vector<AssetImportJobUVE> GetJobsUVE() const override;

private:
    struct ImplUVE;
    std::unique_ptr<ImplUVE> m_impl;
};

} // namespace UVE::Asset
