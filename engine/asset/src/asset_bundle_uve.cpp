//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/asset/asset_bundle_uve.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <system_error>
#include <utility>

#include "uve/asset/uve_file_envelope_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Asset {

namespace {

[[nodiscard]] std::string ToHexStringUVE(std::uint64_t value) {
    char buffer[17];
    std::snprintf(buffer, sizeof(buffer), "%016llx", static_cast<unsigned long long>(value));
    return std::string(buffer);
}

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

bool AssetBundleUVE::PackUVE(const std::vector<AssetBundleEntryUVE>& entries,
                              const std::filesystem::path& bundlePath) {
    std::vector<std::byte> payload;
    AppendUint32UVE(payload, static_cast<std::uint32_t>(entries.size()));

    for (const AssetBundleEntryUVE& entry : entries) {
        std::ifstream file(entry.sourcePath, std::ios::binary);
        if (!file.is_open()) {
            UVE_ERROR("AssetBundleUVE: failed to open \"{}\" for packing", entry.sourcePath.string());
            return false;
        }
        const std::vector<char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        const std::string name = ToHexStringUVE(entry.guid.value);
        AppendUint64UVE(payload, entry.guid.value);
        AppendUint32UVE(payload, static_cast<std::uint32_t>(name.size()));
        AppendBytesUVE(payload, name.data(), name.size());
        AppendUint64UVE(payload, data.size());
        AppendBytesUVE(payload, data.data(), data.size());
    }

    return WriteUveFileUVE(bundlePath, AssetKindUVE::Bundle, payload);
}

bool AssetBundleUVE::UnpackUVE(const std::filesystem::path& bundlePath,
                                const std::filesystem::path& outputDirectory) {
    const std::optional<std::pair<UveFileHeaderUVE, std::vector<std::byte>>> file = ReadUveFileUVE(bundlePath);
    if (!file.has_value()) {
        return false; // ReadUveFileUVE already logged the specific reason.
    }
    const auto& [header, payload] = file.value();
    if (header.assetType != AssetKindUVE::Bundle) {
        UVE_ERROR("AssetBundleUVE: \"{}\" is not a bundle file (asset type {})", bundlePath.string(),
                   static_cast<std::uint32_t>(header.assetType));
        return false;
    }

    std::error_code errorCode;
    std::filesystem::create_directories(outputDirectory, errorCode);
    if (errorCode && !std::filesystem::exists(outputDirectory)) {
        UVE_ERROR("AssetBundleUVE: failed to create output directory \"{}\": {}", outputDirectory.string(),
                   errorCode.message());
        return false;
    }

    std::size_t offset = 0;
    std::uint32_t entryCount = 0;
    if (!ReadUint32FromBufferUVE(payload, offset, entryCount)) {
        UVE_ERROR("AssetBundleUVE: \"{}\" has a truncated entry count", bundlePath.string());
        return false;
    }

    for (std::uint32_t index = 0; index < entryCount; ++index) {
        std::uint64_t guidValue = 0;
        std::uint32_t nameLength = 0;
        if (!ReadUint64FromBufferUVE(payload, offset, guidValue) ||
            !ReadUint32FromBufferUVE(payload, offset, nameLength)) {
            UVE_ERROR("AssetBundleUVE: \"{}\" has a truncated entry header", bundlePath.string());
            return false;
        }
        if (offset + nameLength > payload.size()) {
            UVE_ERROR("AssetBundleUVE: \"{}\" has a truncated entry name", bundlePath.string());
            return false;
        }
        const std::string name(reinterpret_cast<const char*>(payload.data() + offset), nameLength);
        offset += nameLength;

        std::uint64_t dataLength = 0;
        if (!ReadUint64FromBufferUVE(payload, offset, dataLength)) {
            UVE_ERROR("AssetBundleUVE: \"{}\" has a truncated entry data length", bundlePath.string());
            return false;
        }
        if (offset + dataLength > payload.size()) {
            UVE_ERROR("AssetBundleUVE: \"{}\" has truncated entry data", bundlePath.string());
            return false;
        }

        const std::filesystem::path outputPath = outputDirectory / name;
        std::ofstream outFile(outputPath, std::ios::binary);
        if (!outFile.is_open()) {
            UVE_ERROR("AssetBundleUVE: failed to open \"{}\" for writing", outputPath.string());
            return false;
        }
        outFile.write(reinterpret_cast<const char*>(payload.data() + offset),
                       static_cast<std::streamsize>(dataLength));
        offset += dataLength;
        if (!outFile.good()) {
            UVE_ERROR("AssetBundleUVE: failed to write \"{}\"", outputPath.string());
            return false;
        }
    }

    return true;
}

} // namespace UVE::Asset
