// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <memory>

#include "uve/asset/i_asset_importer_uve.h"

namespace UVE::Asset {

/// AssetImporterUVE is the concrete, engine-standard implementation of IAssetImporterUVE. Its
/// constructor registers built-in importers for plain project documents, the existing typed UVE
/// envelopes, and bounded raw PNG-to-`.uvetex`, OBJ-to-`.uvemodel`, MTL-to-`.uvemat`, and
/// glTF/GLB-to-`.uvemodel` bridges.
/// Envelope imports are deterministic reimport/copy contracts; the source importers decode/convert
/// only their documented bounded forms, while raw FBX/JPEG, broader glTF scene/material/image
/// conversion, MTL texture/shader resolution, and other texture/audio conversion remains independently deferred.
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
    [[nodiscard]] AssetImportSourceClassificationUVE ClassifySourceUVE(
        const std::filesystem::path& sourcePath) const override;
    [[nodiscard]] AssetGuidUVE ImportUVE(const std::filesystem::path& sourcePath,
                                          const std::filesystem::path& destinationPath,
                                          IAssetDatabaseUVE& assetDatabase,
                                          const AssetImportSettingsUVE& settings = {}) override;

private:
    struct ImplUVE;
    std::unique_ptr<ImplUVE> m_impl;
};

} // namespace UVE::Asset
