// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <memory>

#include "uve/asset/i_asset_importer_uve.h"

namespace UVE::Asset {

/// AssetImporterUVE is the concrete, engine-standard implementation of IAssetImporterUVE. Its
/// constructor registers the one built-in generic importer (copy source -> destination, then
/// register the destination in the asset database) for ".txt", ".uvescene", and ".uveprefab" —
/// the only source kinds this increment has any real, non-format-specific import need for.
class AssetImporterUVE final : public IAssetImporterUVE {
public:
    AssetImporterUVE();
    ~AssetImporterUVE() override;

    AssetImporterUVE(const AssetImporterUVE&) = delete;
    AssetImporterUVE& operator=(const AssetImporterUVE&) = delete;

    void RegisterImporterUVE(
        std::string sourceExtension,
        std::function<bool(const std::filesystem::path&, const std::filesystem::path&,
                            const AssetImportSettingsUVE&)>
            importFunc) override;
    [[nodiscard]] AssetGuidUVE ImportUVE(const std::filesystem::path& sourcePath,
                                          const std::filesystem::path& destinationPath,
                                          IAssetDatabaseUVE& assetDatabase,
                                          const AssetImportSettingsUVE& settings = {}) override;

private:
    struct ImplUVE;
    std::unique_ptr<ImplUVE> m_impl;
};

} // namespace UVE::Asset
