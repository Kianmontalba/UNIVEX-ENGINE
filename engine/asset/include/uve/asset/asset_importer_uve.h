// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <memory>

#include "uve/asset/i_asset_importer_uve.h"

namespace UVE::Asset {

inline constexpr std::size_t kMaximumTextImportBytesUVE = 16U * 1024U * 1024U;

/// AssetImporterUVE is the concrete, engine-standard implementation of IAssetImporterUVE. Its
/// constructor registers built-in importers for plain project documents, the existing typed UVE
/// envelopes, and bounded raw BMP/PNG/TGA-to-`.uvetex`, OBJ-to-`.uvemodel`, MTL-to-`.uvemat`,
/// glTF/GLB-to-`.uvemodel`, and JPEG-to-`.uvetex` bridges.
/// Envelope imports are deterministic reimport/copy contracts; the source importers decode/convert
/// only their documented bounded forms, while raw FBX/audio, broader glTF scene/material/image
/// conversion, MTL texture/shader resolution, and other audio conversion remains independently deferred;
/// EngineCoreUVE composes the cycle-safe WAV-to-`.uveaudio` bridge after constructing this importer.
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
