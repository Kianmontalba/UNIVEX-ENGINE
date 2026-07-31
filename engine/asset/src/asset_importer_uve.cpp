//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/asset/asset_importer_uve.h"

#include <cctype>
#include <mutex>
#include <system_error>
#include <unordered_map>
#include <utility>

#include "uve/debug/logging_macros_uve.h"

namespace UVE::Asset {

namespace {

using ImportFuncUVE = std::function<bool(const std::filesystem::path&, const std::filesystem::path&,
                                          const AssetImportSettingsUVE&)>;

[[nodiscard]] std::string NormalizeExtensionUVE(std::string extension) {
    if (!extension.empty() && extension.front() == '.') {
        extension.erase(extension.begin());
    }
    for (char& character : extension) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    return extension;
}

/// The one built-in importer: copies `source` to `destination` verbatim, applying no
/// format-specific transformation. `settings` is unused (AssetImportSettingsUVE carries nothing
/// yet) — a future format-specific importer would read fields off a derived settings type here.
[[nodiscard]] bool GenericFileImportUVE(const std::filesystem::path& source,
                                         const std::filesystem::path& destination,
                                         const AssetImportSettingsUVE& /*settings*/) {
    std::error_code errorCode;
    std::filesystem::copy_file(source, destination, std::filesystem::copy_options::overwrite_existing,
                                errorCode);
    if (errorCode) {
        UVE_ERROR("AssetImporterUVE: failed to copy \"{}\" to \"{}\": {}", source.string(),
                   destination.string(), errorCode.message());
        return false;
    }
    return true;
}

} // namespace

struct AssetImporterUVE::ImplUVE {
    mutable std::mutex mutex;
    std::unordered_map<std::string, ImportFuncUVE> importers;
};

AssetImporterUVE::AssetImporterUVE() : m_impl(std::make_unique<ImplUVE>()) {
    RegisterImporterUVE("txt", &GenericFileImportUVE);
    RegisterImporterUVE("uvescene", &GenericFileImportUVE);
    RegisterImporterUVE("uveprefab", &GenericFileImportUVE);
}

AssetImporterUVE::~AssetImporterUVE() = default;

void AssetImporterUVE::RegisterImporterUVE(
    std::string sourceExtension,
    std::function<bool(const std::filesystem::path&, const std::filesystem::path&,
                        const AssetImportSettingsUVE&)>
        importFunc) {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->importers[NormalizeExtensionUVE(std::move(sourceExtension))] = std::move(importFunc);
}

AssetGuidUVE AssetImporterUVE::ImportUVE(const std::filesystem::path& sourcePath,
                                          const std::filesystem::path& destinationPath,
                                          IAssetDatabaseUVE& assetDatabase,
                                          const AssetImportSettingsUVE& settings) {
    ImportFuncUVE importFunc;
    const std::string key = NormalizeExtensionUVE(sourcePath.extension().string());
    {
        const std::lock_guard<std::mutex> lock(m_impl->mutex);
        const auto it = m_impl->importers.find(key);
        if (it == m_impl->importers.end()) {
            UVE_ERROR("AssetImporterUVE: no importer registered for extension \"{}\" (source \"{}\")", key,
                       sourcePath.string());
            return kInvalidAssetGuidUVE;
        }
        importFunc = it->second;
    }

    if (!importFunc(sourcePath, destinationPath, settings)) {
        UVE_ERROR("AssetImporterUVE: import failed for \"{}\"", sourcePath.string());
        return kInvalidAssetGuidUVE;
    }

    return assetDatabase.RegisterUVE(destinationPath);
}

} // namespace UVE::Asset
