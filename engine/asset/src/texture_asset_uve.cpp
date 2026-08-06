// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/texture_asset_uve.h"

#include <cstring>

#include "uve/asset/uve_file_envelope_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Asset {

namespace {

void AppendBytesUVE(std::vector<std::byte>& buffer, const void* data, std::size_t size) {
    const auto* const bytes = static_cast<const std::byte*>(data);
    buffer.insert(buffer.end(), bytes, bytes + size);
}

void AppendUint32UVE(std::vector<std::byte>& buffer, std::uint32_t value) {
    AppendBytesUVE(buffer, &value, sizeof(value));
}

void AppendUint64UVE(std::vector<std::byte>& buffer, std::uint64_t value) {
    AppendBytesUVE(buffer, &value, sizeof(value));
}

[[nodiscard]] bool ReadUint32FromBufferUVE(const std::vector<std::byte>& buffer, std::size_t& offset,
                                            std::uint32_t& outValue) {
    if (offset + sizeof(outValue) > buffer.size()) {
        return false;
    }
    std::memcpy(&outValue, buffer.data() + offset, sizeof(outValue));
    offset += sizeof(outValue);
    return true;
}

[[nodiscard]] bool ReadUint64FromBufferUVE(const std::vector<std::byte>& buffer, std::size_t& offset,
                                            std::uint64_t& outValue) {
    if (offset + sizeof(outValue) > buffer.size()) {
        return false;
    }
    std::memcpy(&outValue, buffer.data() + offset, sizeof(outValue));
    offset += sizeof(outValue);
    return true;
}

} // namespace

std::uint32_t BytesPerPixelUVE(TextureFormatUVE format) noexcept {
    switch (format) {
        case TextureFormatUVE::RGBA8Unorm:
            return 4;
        case TextureFormatUVE::RGBA16Float:
            return 8;
    }
    return 0; // unreachable for a valid enumerator; keeps this function total.
}

bool LoadTextureAssetUVE(const std::filesystem::path& path, TextureAssetUVE& outTexture) {
    const std::optional<std::pair<UveFileHeaderUVE, std::vector<std::byte>>> file = ReadUveFileUVE(path);
    if (!file.has_value()) {
        return false; // ReadUveFileUVE already logged the specific reason.
    }
    if (file->first.assetType != AssetKindUVE::Texture) {
        UVE_ERROR("TextureAssetUVE: \"{}\" is not a texture file (asset type {})", path.string(),
                   static_cast<std::uint32_t>(file->first.assetType));
        return false;
    }

    const std::vector<std::byte>& payload = file->second;
    std::size_t offset = 0;

    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t formatValue = 0;
    std::uint64_t pixelByteCount = 0;
    if (!ReadUint32FromBufferUVE(payload, offset, width) || !ReadUint32FromBufferUVE(payload, offset, height) ||
        !ReadUint32FromBufferUVE(payload, offset, formatValue) ||
        !ReadUint64FromBufferUVE(payload, offset, pixelByteCount)) {
        UVE_ERROR("TextureAssetUVE: \"{}\" has a truncated header", path.string());
        return false;
    }
    if (formatValue > static_cast<std::uint32_t>(TextureFormatUVE::RGBA16Float)) {
        UVE_ERROR("TextureAssetUVE: \"{}\" has an unknown texture format {}", path.string(), formatValue);
        return false;
    }
    const auto format = static_cast<TextureFormatUVE>(formatValue);

    if (offset + pixelByteCount > payload.size()) {
        UVE_ERROR("TextureAssetUVE: \"{}\" has truncated pixel data", path.string());
        return false;
    }

    const std::uint64_t expectedByteCount =
        static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height) * BytesPerPixelUVE(format);
    if (pixelByteCount != expectedByteCount) {
        UVE_ERROR("TextureAssetUVE: \"{}\" has {} pixel bytes, expected {} for a {}x{} texture", path.string(),
                   pixelByteCount, expectedByteCount, width, height);
        return false;
    }

    std::vector<std::byte> pixels(payload.begin() + static_cast<std::ptrdiff_t>(offset),
                                   payload.begin() + static_cast<std::ptrdiff_t>(offset + pixelByteCount));

    outTexture.width = width;
    outTexture.height = height;
    outTexture.format = format;
    outTexture.pixels = std::move(pixels);
    return true;
}

bool SaveTextureAssetUVE(const TextureAssetUVE& texture, const std::filesystem::path& path) {
    std::vector<std::byte> payload;
    AppendUint32UVE(payload, texture.width);
    AppendUint32UVE(payload, texture.height);
    AppendUint32UVE(payload, static_cast<std::uint32_t>(texture.format));
    AppendUint64UVE(payload, static_cast<std::uint64_t>(texture.pixels.size()));
    AppendBytesUVE(payload, texture.pixels.data(), texture.pixels.size());

    return WriteUveFileUVE(path, AssetKindUVE::Texture, payload);
}

} // namespace UVE::Asset
