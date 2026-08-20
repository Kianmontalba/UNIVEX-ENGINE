// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/mtl_importer_uve.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "uve/asset/material_asset_uve.h"
#include "uve/asset/mtl_material_converter_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Asset {
namespace {

[[nodiscard]] bool ReadMtlSourceBytesUVE(const std::filesystem::path& sourcePath,
                                         std::vector<std::byte>& outBytes) {
    std::ifstream input(sourcePath, std::ios::binary | std::ios::ate);
    if (!input.is_open()) {
        UVE_ERROR("MtlImporterUVE: failed to open source \"{}\"", sourcePath.string());
        return false;
    }
    const std::streamoff fileSize = input.tellg();
    if (fileSize < 0 || static_cast<std::uint64_t>(fileSize) > kMaximumMtlMaterialSourceBytesUVE ||
        static_cast<std::uint64_t>(fileSize) > std::numeric_limits<std::size_t>::max()) {
        UVE_ERROR("MtlImporterUVE: source \"{}\" exceeds the bounded source-size limit", sourcePath.string());
        return false;
    }
    std::vector<std::byte> bytes(static_cast<std::size_t>(fileSize));
    input.seekg(0, std::ios::beg);
    if (!bytes.empty()) {
        input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!input) {
            UVE_ERROR("MtlImporterUVE: source \"{}\" could not be read completely", sourcePath.string());
            return false;
        }
    }
    outBytes = std::move(bytes);
    return true;
}

[[nodiscard]] bool SaveMaterialAssetAtomicallyUVE(const MaterialAssetUVE& material,
                                                  const std::filesystem::path& destinationPath) {
    std::error_code errorCode;
    if (const std::filesystem::path parent = destinationPath.parent_path(); !parent.empty()) {
        std::filesystem::create_directories(parent, errorCode);
        if (errorCode) {
            UVE_ERROR("MtlImporterUVE: failed to create destination directory \"{}\": {}", parent.string(),
                      errorCode.message());
            return false;
        }
    }

    const std::filesystem::path temporaryPath = destinationPath.string() + ".uve_mtl_tmp";
    std::filesystem::remove(temporaryPath, errorCode);
    if (!SaveMaterialAssetUVE(material, temporaryPath)) {
        std::filesystem::remove(temporaryPath, errorCode);
        return false;
    }
    std::filesystem::rename(temporaryPath, destinationPath, errorCode);
    if (errorCode) {
        UVE_ERROR("MtlImporterUVE: failed to publish destination \"{}\": {}", destinationPath.string(),
                  errorCode.message());
        std::filesystem::remove(temporaryPath, errorCode);
        return false;
    }
    return true;
}

[[nodiscard]] bool ImportMtlSourceUVE(const std::filesystem::path& sourcePath,
                                      const std::filesystem::path& destinationPath,
                                      const AssetImportSettingsUVE& /*settings*/) {
    if (destinationPath.extension() != ".uvemat") {
        UVE_ERROR("MtlImporterUVE: destination \"{}\" must use the .uvemat extension",
                  destinationPath.string());
        return false;
    }

    std::vector<std::byte> sourceBytes;
    if (!ReadMtlSourceBytesUVE(sourcePath, sourceBytes)) {
        return false;
    }

    const std::string_view source(reinterpret_cast<const char*>(sourceBytes.data()), sourceBytes.size());
    MaterialAssetUVE material;
    if (!ConvertMtlMaterialUVE(source, material)) {
        UVE_ERROR("MtlImporterUVE: source \"{}\" failed bounded MTL material conversion", sourcePath.string());
        return false;
    }
    return SaveMaterialAssetAtomicallyUVE(material, destinationPath);
}

} // namespace

void RegisterMtlImporterUVE(IAssetImporterUVE& importer) {
    importer.RegisterImporterUVE("mtl", &ImportMtlSourceUVE);
}

} // namespace UVE::Asset
