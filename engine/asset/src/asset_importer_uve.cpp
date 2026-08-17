// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


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

[[nodiscard]] AssetImportSourceKindUVE ClassifySourceKindUVE(const std::string& extension) {
    if (extension == "txt") {
        return AssetImportSourceKindUVE::PlainText;
    }
    if (extension == "uvescene") {
        return AssetImportSourceKindUVE::SceneEnvelope;
    }
    if (extension == "uveprefab") {
        return AssetImportSourceKindUVE::PrefabEnvelope;
    }
    if (extension == "uvemodel") {
        return AssetImportSourceKindUVE::MeshEnvelope;
    }
    if (extension == "uvetex") {
        return AssetImportSourceKindUVE::TextureEnvelope;
    }
    if (extension == "uveshader") {
        return AssetImportSourceKindUVE::ShaderEnvelope;
    }
    if (extension == "uvemat") {
        return AssetImportSourceKindUVE::MaterialEnvelope;
    }
    if (extension == "fbx" || extension == "obj" || extension == "gltf" || extension == "glb" ||
        extension == "dae") {
        return AssetImportSourceKindUVE::RawModel;
    }
    if (extension == "png" || extension == "jpg" || extension == "jpeg" || extension == "tga" ||
        extension == "bmp" || extension == "webp" || extension == "hdr" || extension == "exr") {
        return AssetImportSourceKindUVE::RawTexture;
    }
    if (extension == "material" || extension == "mat" || extension == "mtl") {
        return AssetImportSourceKindUVE::RawMaterial;
    }
    return AssetImportSourceKindUVE::Unknown;
}

[[nodiscard]] bool RequiresFormatSpecificParserUVE(const AssetImportSourceKindUVE kind) {
    return kind == AssetImportSourceKindUVE::RawModel || kind == AssetImportSourceKindUVE::RawTexture ||
           kind == AssetImportSourceKindUVE::RawMaterial;
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

    // Typed UVE envelopes are already validated by their corresponding asset loaders. Importing
    // them here is an intentionally format-neutral, deterministic copy/re-register operation;
    // source-format conversion (FBX/OBJ/glTF/PNG/etc.) remains a separate parser-owned increment.
    RegisterImporterUVE("uvemodel", &GenericFileImportUVE);
    RegisterImporterUVE("uvetex", &GenericFileImportUVE);
    RegisterImporterUVE("uveshader", &GenericFileImportUVE);
    RegisterImporterUVE("uvemat", &GenericFileImportUVE);
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

AssetImportSourceClassificationUVE AssetImporterUVE::ClassifySourceUVE(
    const std::filesystem::path& sourcePath) const {
    AssetImportSourceClassificationUVE classification;
    classification.normalizedExtension = NormalizeExtensionUVE(sourcePath.extension().string());
    classification.kind = ClassifySourceKindUVE(classification.normalizedExtension);
    classification.requiresFormatSpecificParser = RequiresFormatSpecificParserUVE(classification.kind);
    {
        const std::lock_guard<std::mutex> lock(m_impl->mutex);
        classification.importerRegistered =
            m_impl->importers.find(classification.normalizedExtension) != m_impl->importers.end();
    }

    if (classification.requiresFormatSpecificParser) {
        classification.diagnostic = classification.importerRegistered
                                         ? "format-specific parser is registered"
                                         : "format-specific parser is not registered";
    } else if (classification.kind == AssetImportSourceKindUVE::Unknown) {
        classification.diagnostic = classification.importerRegistered
                                         ? "custom importer is registered without built-in classification"
                                         : "unsupported source extension";
    } else {
        classification.diagnostic = classification.importerRegistered
                                         ? "built-in generic copy importer is registered"
                                         : "source format is classified but no importer is registered";
    }
    return classification;
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
