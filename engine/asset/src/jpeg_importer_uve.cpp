// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/jpeg_importer_uve.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <new>
#include <system_error>
#include <utility>
#include <vector>

#include "uve/asset/jpeg_metadata_uve.h"
#include "uve/asset/texture_asset_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Asset {
namespace {

constexpr std::uint64_t kMaximumJpegImporterSourceBytesUVE = 64ULL * 1024ULL * 1024ULL;

[[nodiscard]] bool ReadJpegSourceBytesUVE(const std::filesystem::path& sourcePath,
                                          std::vector<std::byte>& outBytes) {
    std::ifstream input(sourcePath, std::ios::binary | std::ios::ate);
    if (!input.is_open()) {
        UVE_ERROR("JpegImporterUVE: failed to open source \"{}\"", sourcePath.string());
        return false;
    }
    const std::streamoff fileSize = input.tellg();
    if (fileSize < 0 || static_cast<std::uint64_t>(fileSize) > kMaximumJpegImporterSourceBytesUVE ||
        static_cast<std::uint64_t>(fileSize) > std::numeric_limits<std::size_t>::max()) {
        UVE_ERROR("JpegImporterUVE: source \"{}\" exceeds the bounded source-size limit", sourcePath.string());
        return false;
    }
    std::vector<std::byte> bytes(static_cast<std::size_t>(fileSize));
    input.seekg(0, std::ios::beg);
    if (!bytes.empty()) {
        input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!input) {
            UVE_ERROR("JpegImporterUVE: source \"{}\" could not be read completely", sourcePath.string());
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
            UVE_ERROR("JpegImporterUVE: failed to create destination directory \"{}\": {}", parent.string(),
                      errorCode.message());
            return false;
        }
    }
    const std::filesystem::path temporaryPath = destinationPath.string() + ".uve_jpeg_tmp";
    std::filesystem::remove(temporaryPath, errorCode);
    if (!SaveTextureAssetUVE(texture, temporaryPath)) {
        std::filesystem::remove(temporaryPath, errorCode);
        return false;
    }
    std::filesystem::rename(temporaryPath, destinationPath, errorCode);
    if (errorCode) {
        UVE_ERROR("JpegImporterUVE: failed to publish destination \"{}\": {}", destinationPath.string(),
                  errorCode.message());
        std::filesystem::remove(temporaryPath, errorCode);
        return false;
    }
    return true;
}

[[nodiscard]] bool ImportJpegSourceUVE(const std::filesystem::path& sourcePath,
                                       const std::filesystem::path& destinationPath,
                                       const AssetImportSettingsUVE& /*settings*/) {
    try {
        if (destinationPath.extension() != ".uvetex") {
            UVE_ERROR("JpegImporterUVE: destination \"{}\" must use the .uvetex extension",
                      destinationPath.string());
            return false;
        }
        std::vector<std::byte> sourceBytes;
        if (!ReadJpegSourceBytesUVE(sourcePath, sourceBytes)) return false;
        JpegRgba8ImageUVE decoded;
        if (!DecodeJpegRgba8ImageUVE(sourceBytes, decoded)) {
            UVE_ERROR("JpegImporterUVE: source \"{}\" failed bounded JPEG RGBA8 decoding", sourcePath.string());
            return false;
        }
        const std::uint64_t expectedPixelBytes = static_cast<std::uint64_t>(decoded.width) *
                                                  static_cast<std::uint64_t>(decoded.height) * 4ULL;
        if (expectedPixelBytes != decoded.pixels.size()) {
            UVE_ERROR("JpegImporterUVE: decoder published inconsistent RGBA8 pixel bytes for \"{}\"",
                      sourcePath.string());
            return false;
        }
        TextureAssetUVE texture;
        texture.width = decoded.width;
        texture.height = decoded.height;
        texture.format = TextureFormatUVE::RGBA8Unorm;
        texture.pixels = std::move(decoded.pixels);
        return SaveTextureAssetAtomicallyUVE(texture, destinationPath);
    } catch (const std::bad_alloc&) {
        return false;
    }
}

} // namespace

void RegisterJpegImporterUVE(IAssetImporterUVE& importer) {
    importer.RegisterImporterUVE("jpg", &ImportJpegSourceUVE);
    importer.RegisterImporterUVE("jpeg", &ImportJpegSourceUVE);
}

} // namespace UVE::Asset
