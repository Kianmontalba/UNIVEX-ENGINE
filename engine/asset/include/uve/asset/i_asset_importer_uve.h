// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <filesystem>
#include <functional>
#include <string>

#include "uve/asset/asset_guid_uve.h"
#include "uve/asset/i_asset_database_uve.h"

namespace UVE::Asset {

/// Base for per-asset-type import settings (e.g. a future MeshImportSettingsUVE adding a
/// normal-map-flip flag or LOD generation count). Empty this increment — no format-specific
/// importer exists yet to need settings beyond the generic copy-and-register behavior — but the
/// type exists now so future importers extend it rather than ImportUVE() gaining a new parameter
/// per format later.
struct AssetImportSettingsUVE {
    virtual ~AssetImportSettingsUVE() = default;

    /// Stable cache discriminator for all import behavior represented by this settings object.
    /// Future format-specific settings override this when a conversion option changes derived
    /// output. The default keeps the current generic copy-and-register importer deterministic.
    [[nodiscard]] virtual std::string GetCacheVersionUVE() const { return "generic-v1"; }
};

/// IAssetImporterUVE is the spec's "import pipeline with settings per asset type" (Part 7.4):
/// register an import function per source file extension, then ImportUVE() any source file
/// through whichever one matches. Ships with exactly one built-in importer this increment — a
/// generic "copy the source file into the project and register its GUID" importer, registered
/// for a handful of extensions that have no format-specific parsing need — since no FBX/OBJ/
/// GLTF/PNG/WAV parsing library exists in this codebase yet. Format-specific importers are
/// registered by future Rendering/Audio increments via RegisterImporterUVE().
/// Thread-safety: thread-safe. Every method is guarded by an internal mutex, matching
/// ConfigManagerUVE's/AssetDatabaseUVE's contract.
class IAssetImporterUVE {
public:
    virtual ~IAssetImporterUVE() = default;

    /// Registers `importFunc` for `sourceExtension` (matched case-insensitively, with or without
    /// a leading dot — ".png" and "png" register the same key): `importFunc(source, destination,
    /// settings)` should copy/convert `source` to `destination` and return true on success.
    /// Replaces any importer already registered for that extension, including a built-in one.
    virtual void RegisterImporterUVE(
        std::string sourceExtension,
        std::function<bool(const std::filesystem::path&, const std::filesystem::path&,
                            const AssetImportSettingsUVE&)>
            importFunc) = 0;

    /// Imports `sourcePath` to `destinationPath` using whichever importer is registered for
    /// `sourcePath`'s extension, registers `destinationPath` in `assetDatabase`, and returns its
    /// GUID. Returns `kInvalidAssetGuidUVE` (logging the reason) if no importer is registered for
    /// that extension, or if the registered importer itself reports failure.
    [[nodiscard]] virtual AssetGuidUVE ImportUVE(const std::filesystem::path& sourcePath,
                                                  const std::filesystem::path& destinationPath,
                                                  IAssetDatabaseUVE& assetDatabase,
                                                  const AssetImportSettingsUVE& settings = {}) = 0;
};

} // namespace UVE::Asset
