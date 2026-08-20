// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/obj_importer_uve.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "uve/asset/mesh_asset_uve.h"
#include "uve/asset/obj_mesh_converter_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Asset {
namespace {

[[nodiscard]] bool ReadObjSourceBytesUVE(const std::filesystem::path& sourcePath,
                                         std::vector<std::byte>& outBytes) {
    std::ifstream input(sourcePath, std::ios::binary | std::ios::ate);
    if (!input.is_open()) {
        UVE_ERROR("ObjImporterUVE: failed to open source \"{}\"", sourcePath.string());
        return false;
    }
    const std::streamoff fileSize = input.tellg();
    if (fileSize < 0 || static_cast<std::uint64_t>(fileSize) > kMaximumObjMeshSourceBytesUVE ||
        static_cast<std::uint64_t>(fileSize) > std::numeric_limits<std::size_t>::max()) {
        UVE_ERROR("ObjImporterUVE: source \"{}\" exceeds the bounded source-size limit", sourcePath.string());
        return false;
    }
    std::vector<std::byte> bytes(static_cast<std::size_t>(fileSize));
    input.seekg(0, std::ios::beg);
    if (!bytes.empty()) {
        input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!input) {
            UVE_ERROR("ObjImporterUVE: source \"{}\" could not be read completely", sourcePath.string());
            return false;
        }
    }
    outBytes = std::move(bytes);
    return true;
}

[[nodiscard]] bool SaveMeshAssetAtomicallyUVE(const MeshAssetUVE& mesh,
                                              const std::filesystem::path& destinationPath) {
    std::error_code errorCode;
    if (const std::filesystem::path parent = destinationPath.parent_path(); !parent.empty()) {
        std::filesystem::create_directories(parent, errorCode);
        if (errorCode) {
            UVE_ERROR("ObjImporterUVE: failed to create destination directory \"{}\": {}", parent.string(),
                      errorCode.message());
            return false;
        }
    }

    const std::filesystem::path temporaryPath = destinationPath.string() + ".uve_obj_tmp";
    std::filesystem::remove(temporaryPath, errorCode);
    if (!SaveMeshAssetUVE(mesh, temporaryPath)) {
        std::filesystem::remove(temporaryPath, errorCode);
        return false;
    }
    std::filesystem::rename(temporaryPath, destinationPath, errorCode);
    if (errorCode) {
        UVE_ERROR("ObjImporterUVE: failed to publish destination \"{}\": {}", destinationPath.string(),
                  errorCode.message());
        std::filesystem::remove(temporaryPath, errorCode);
        return false;
    }
    return true;
}

[[nodiscard]] bool ImportObjSourceUVE(const std::filesystem::path& sourcePath,
                                      const std::filesystem::path& destinationPath,
                                      const AssetImportSettingsUVE& /*settings*/) {
    if (destinationPath.extension() != ".uvemodel") {
        UVE_ERROR("ObjImporterUVE: destination \"{}\" must use the .uvemodel extension",
                  destinationPath.string());
        return false;
    }

    std::vector<std::byte> sourceBytes;
    if (!ReadObjSourceBytesUVE(sourcePath, sourceBytes)) {
        return false;
    }

    const std::string_view source(reinterpret_cast<const char*>(sourceBytes.data()), sourceBytes.size());
    MeshAssetUVE mesh;
    if (!ConvertObjMeshUVE(source, mesh)) {
        UVE_ERROR("ObjImporterUVE: source \"{}\" failed bounded OBJ mesh conversion", sourcePath.string());
        return false;
    }
    return SaveMeshAssetAtomicallyUVE(mesh, destinationPath);
}

} // namespace

void RegisterObjImporterUVE(IAssetImporterUVE& importer) {
    importer.RegisterImporterUVE("obj", &ImportObjSourceUVE);
}

} // namespace UVE::Asset
