// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/bmp_importer_uve.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <system_error>
#include <utility>
#include <vector>

#include "uve/asset/bmp_metadata_uve.h"
#include "uve/asset/texture_asset_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Asset {
namespace {

constexpr std::uint64_t kMaximumBmpImporterSourceBytesUVE = 64ULL * 1024ULL * 1024ULL;

[[nodiscard]] bool ReadBmpSourceBytesUVE(const std::filesystem::path& sourcePath,
                                         std::vector<std::byte>& outBytes) {
    std::ifstream input(sourcePath, std::ios::binary | std::ios::ate);
    if (!input.is_open()) {
        UVE_ERROR("BmpImporterUVE: failed to open source \"{}\"", sourcePath.string());
        return false;
    }
    const std::streamoff fileSize = input.tellg();
    if (fileSize < 0 || static_cast<std::uint64_t>(fileSize) > kMaximumBmpImporterSourceBytesUVE ||
        static_cast<std::uint64_t>(fileSize) > std::numeric_limits<std::size_t>::max()) {
        UVE_ERROR("BmpImporterUVE: source \"{}\" exceeds the bounded source-size limit", sourcePath.string());
        return false;
    }
    std::vector<std::byte> bytes(static_cast<std::size_t>(fileSize));
    input.seekg(0, std::ios::beg);
    if (!bytes.empty()) {
        input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!input) {
            UVE_ERROR("BmpImporterUVE: source \"{}\" could not be read completely", sourcePath.string());
            return false;
        }
    }
    outBytes = std::move(bytes);
    return true;
}

[[nodiscard]] bool SaveTextureAssetAtomicallyUVE(const TextureAssetUVE& texture,
                                                 const std::filesystem::path& destinationPath) {
    std::error_code errorCode;
    if (const std::filesystem::path parent = destinationPath.parent_path(); !parent.empty()) {
        std::filesystem::create_directories(parent, errorCode);
        if (errorCode) {
            UVE_ERROR("BmpImporterUVE: failed to create destination directory \"{}\": {}", parent.string(),
                      errorCode.message());
            return false;
        }
    }
    const std::filesystem::path temporaryPath = destinationPath.string() + ".uve_bmp_tmp";
    std::filesystem::remove(temporaryPath, errorCode);
    if (!SaveTextureAssetUVE(texture, temporaryPath)) {
        std::filesystem::remove(temporaryPath, errorCode);
        return false;
    }
    std::filesystem::rename(temporaryPath, destinationPath, errorCode);
    if (errorCode) {
        UVE_ERROR("BmpImporterUVE: failed to publish destination \"{}\": {}", destinationPath.string(),
                  errorCode.message());
        std::filesystem::remove(temporaryPath, errorCode);
        return false;
    }
    return true;
}

[[nodiscard]] bool ImportBmpSourceUVE(const std::filesystem::path& sourcePath,
                                      const std::filesystem::path& destinationPath,
                                      const AssetImportSettingsUVE& /*settings*/) {
    if (destinationPath.extension() != ".uvetex") {
        UVE_ERROR("BmpImporterUVE: destination \"{}\" must use the .uvetex extension", destinationPath.string());
        return false;
    }
    std::vector<std::byte> sourceBytes;
    if (!ReadBmpSourceBytesUVE(sourcePath, sourceBytes)) {
        return false;
    }
    BmpRgba8ImageUVE decoded;
    if (!DecodeBmpRgba8ImageUVE(sourceBytes, decoded)) {
        UVE_ERROR("BmpImporterUVE: source \"{}\" failed bounded BMP RGBA8 decoding", sourcePath.string());
        return false;
    }
    TextureAssetUVE texture;
    texture.width = decoded.width;
    texture.height = decoded.height;
    texture.format = TextureFormatUVE::RGBA8Unorm;
    texture.pixels = std::move(decoded.pixels);
    return SaveTextureAssetAtomicallyUVE(texture, destinationPath);
}

} // namespace

void RegisterBmpImporterUVE(IAssetImporterUVE& importer) {
    importer.RegisterImporterUVE("bmp", &ImportBmpSourceUVE);
}

} // namespace UVE::Asset
