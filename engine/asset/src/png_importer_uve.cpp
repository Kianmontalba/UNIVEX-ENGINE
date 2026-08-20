// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/png_importer_uve.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <system_error>
#include <vector>
#include <utility>

#include "uve/asset/png_metadata_uve.h"
#include "uve/asset/texture_asset_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Asset {
namespace {

constexpr std::uint64_t kMaximumPngImporterSourceBytesUVE = 64ULL * 1024ULL * 1024ULL;

[[nodiscard]] bool ReadPngSourceBytesUVE(const std::filesystem::path& sourcePath,
                                         std::vector<std::byte>& outBytes) {
    std::ifstream input(sourcePath, std::ios::binary | std::ios::ate);
    if (!input.is_open()) {
        UVE_ERROR("PngImporterUVE: failed to open source \"{}\"", sourcePath.string());
        return false;
    }
    const std::streamoff fileSize = input.tellg();
    if (fileSize < 0 || static_cast<std::uint64_t>(fileSize) > kMaximumPngImporterSourceBytesUVE ||
        static_cast<std::uint64_t>(fileSize) > std::numeric_limits<std::size_t>::max()) {
        UVE_ERROR("PngImporterUVE: source \"{}\" exceeds the bounded source-size limit", sourcePath.string());
        return false;
    }
    std::vector<std::byte> bytes(static_cast<std::size_t>(fileSize));
    input.seekg(0, std::ios::beg);
    if (!bytes.empty()) {
        input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!input) {
            UVE_ERROR("PngImporterUVE: source \"{}\" could not be read completely", sourcePath.string());
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
            UVE_ERROR("PngImporterUVE: failed to create destination directory \"{}\": {}", parent.string(),
                      errorCode.message());
            return false;
        }
    }

    const std::filesystem::path temporaryPath = destinationPath.string() + ".uve_png_tmp";
    std::filesystem::remove(temporaryPath, errorCode);
    if (!SaveTextureAssetUVE(texture, temporaryPath)) {
        std::filesystem::remove(temporaryPath, errorCode);
        return false;
    }
    std::filesystem::rename(temporaryPath, destinationPath, errorCode);
    if (errorCode) {
        UVE_ERROR("PngImporterUVE: failed to publish destination \"{}\": {}", destinationPath.string(),
                  errorCode.message());
        std::filesystem::remove(temporaryPath, errorCode);
        return false;
    }
    return true;
}

[[nodiscard]] bool ImportPngSourceUVE(const std::filesystem::path& sourcePath,
                                      const std::filesystem::path& destinationPath,
                                      const AssetImportSettingsUVE& /*settings*/) {
    if (destinationPath.extension() != ".uvetex") {
        UVE_ERROR("PngImporterUVE: destination \"{}\" must use the .uvetex extension", destinationPath.string());
        return false;
    }

    std::vector<std::byte> sourceBytes;
    if (!ReadPngSourceBytesUVE(sourcePath, sourceBytes)) {
        return false;
    }

    PngRgba8ImageUVE decoded;
    if (!DecodePngRgba8ImageUVE(sourceBytes, decoded)) {
        UVE_ERROR("PngImporterUVE: source \"{}\" failed bounded PNG RGBA8 decoding", sourcePath.string());
        return false;
    }

    const std::uint64_t expectedPixelBytes = static_cast<std::uint64_t>(decoded.width) *
                                               static_cast<std::uint64_t>(decoded.height) * 4ULL;
    if (expectedPixelBytes != decoded.pixels.size()) {
        UVE_ERROR("PngImporterUVE: decoder published inconsistent RGBA8 pixel bytes for \"{}\"",
                  sourcePath.string());
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

void RegisterPngImporterUVE(IAssetImporterUVE& importer) {
    importer.RegisterImporterUVE("png", &ImportPngSourceUVE);
}

} // namespace UVE::Asset
