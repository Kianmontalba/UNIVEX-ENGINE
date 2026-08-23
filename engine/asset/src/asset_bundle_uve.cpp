// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/asset_bundle_uve.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
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

[[nodiscard]] bool IsSafeRelativePathUVE(std::string_view name) {
    if (name.empty() || name.size() > kMaximumAssetBundleEntryNameBytesUVE ||
        name.find('\0') != std::string_view::npos || name.find('\\') != std::string_view::npos) {
        return false;
    }
    const std::filesystem::path path{std::string(name)};
    if (path.is_absolute() || path.has_root_name() || path.has_root_directory()) {
        return false;
    }
    return std::none_of(path.begin(), path.end(), [](const std::filesystem::path& component) {
        return component == "." || component == "..";
    });
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

/// Reads and validates `bundlePath` as a `.uve*` envelope with `AssetKindUVE::Bundle`, returning
/// its payload bytes. Returns `std::nullopt` (already logged, by `ReadUveFileUVE` or this
/// function) if the file is invalid, unreadable, or not actually a bundle.
[[nodiscard]] std::optional<std::vector<std::byte>> LoadBundlePayloadUVE(const std::filesystem::path& bundlePath) {
    std::optional<std::pair<UveFileHeaderUVE, std::vector<std::byte>>> file = ReadUveFileUVE(bundlePath);
    if (!file.has_value()) {
        return std::nullopt; // ReadUveFileUVE already logged the specific reason.
    }
    if (file->first.assetType != AssetKindUVE::Bundle) {
        UVE_ERROR("AssetBundleUVE: \"{}\" is not a bundle file (asset type {})", bundlePath.string(),
                   static_cast<std::uint32_t>(file->first.assetType));
        return std::nullopt;
    }
    return std::move(file->second);
}

/// Scans `payload`'s entry table (as written by PackUVE), calling `visitor(name, dataOffset,
/// dataLength)` for each entry in order. `visitor` returns true to keep scanning, false to stop
/// early (e.g. once a name match is found or a write fails) — stopping early is not itself an
/// error. Returns false only if the payload's own structure is truncated/malformed (already
/// logged, naming `bundlePath` for context).
[[nodiscard]] bool ScanBundleEntriesUVE(
    const std::filesystem::path& bundlePath, const std::vector<std::byte>& payload,
    const std::function<bool(std::string_view name, std::size_t dataOffset, std::uint64_t dataLength)>& visitor) {
    std::size_t offset = 0;
    std::uint32_t entryCount = 0;
    if (!ReadUint32FromBufferUVE(payload, offset, entryCount)) {
        UVE_ERROR("AssetBundleUVE: \"{}\" has a truncated entry count", bundlePath.string());
        return false;
    }
    if (entryCount > kMaximumAssetBundleEntriesUVE) {
        UVE_ERROR("AssetBundleUVE: \"{}\" exceeds the bounded entry-count limit", bundlePath.string());
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
        static_cast<void>(guidValue); // not needed for scanning/lookup, only the name/data ranges are
        if (nameLength == 0U || nameLength > kMaximumAssetBundleEntryNameBytesUVE ||
            offset > payload.size() || nameLength > payload.size() - offset) {
            UVE_ERROR("AssetBundleUVE: \"{}\" has a truncated entry name", bundlePath.string());
            return false;
        }
        const std::string_view name(reinterpret_cast<const char*>(payload.data() + offset), nameLength);
        offset += nameLength;
        if (!IsSafeRelativePathUVE(name)) {
            UVE_ERROR("AssetBundleUVE: \"{}\" contains an unsafe entry name", bundlePath.string());
            return false;
        }

        std::uint64_t dataLength = 0;
        if (!ReadUint64FromBufferUVE(payload, offset, dataLength)) {
            UVE_ERROR("AssetBundleUVE: \"{}\" has a truncated entry data length", bundlePath.string());
            return false;
        }
        if (dataLength > static_cast<std::uint64_t>(payload.size() - offset)) {
            UVE_ERROR("AssetBundleUVE: \"{}\" has truncated entry data", bundlePath.string());
            return false;
        }

        if (!visitor(name, offset, dataLength)) {
            return true; // visitor found what it needed and asked to stop - not an error
        }
        offset += dataLength;
    }
    return true;
}

} // namespace

bool AssetBundleUVE::PackUVE(const std::vector<AssetBundleEntryUVE>& entries,
                              const std::filesystem::path& bundlePath) {
    if (entries.size() > kMaximumAssetBundleEntriesUVE) {
        UVE_ERROR("AssetBundleUVE: entry count exceeds the bounded limit");
        return false;
    }
    std::vector<std::byte> payload;
    AppendUint32UVE(payload, static_cast<std::uint32_t>(entries.size()));

    for (const AssetBundleEntryUVE& entry : entries) {
        std::ifstream file(entry.sourcePath, std::ios::binary);
        if (!file.is_open()) {
            UVE_ERROR("AssetBundleUVE: failed to open \"{}\" for packing", entry.sourcePath.string());
            return false;
        }
        const std::vector<char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        const std::string name = entry.virtualName.empty() ? ToHexStringUVE(entry.guid.value) : entry.virtualName;
        if (!IsSafeRelativePathUVE(name)) {
            UVE_ERROR("AssetBundleUVE: entry name \"{}\" is not a safe relative path", name);
            return false;
        }
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
    const std::optional<std::vector<std::byte>> payload = LoadBundlePayloadUVE(bundlePath);
    if (!payload.has_value()) {
        return false; // already logged
    }

    std::error_code errorCode;
    std::filesystem::create_directories(outputDirectory, errorCode);
    if (errorCode && !std::filesystem::exists(outputDirectory)) {
        UVE_ERROR("AssetBundleUVE: failed to create output directory \"{}\": {}", outputDirectory.string(),
                   errorCode.message());
        return false;
    }

    bool writeFailed = false;
    const bool scanOk = ScanBundleEntriesUVE(
        bundlePath, *payload,
        [&](std::string_view name, std::size_t dataOffset, std::uint64_t dataLength) -> bool {
            if (!IsSafeRelativePathUVE(name)) {
                UVE_ERROR("AssetBundleUVE: entry name \"{}\" is not a safe relative path", name);
                writeFailed = true;
                return false;
            }
            const std::filesystem::path outputPath = outputDirectory / std::string(name);
            std::ofstream outFile(outputPath, std::ios::binary);
            if (!outFile.is_open()) {
                UVE_ERROR("AssetBundleUVE: failed to open \"{}\" for writing", outputPath.string());
                writeFailed = true;
                return false;
            }
            outFile.write(reinterpret_cast<const char*>(payload->data() + dataOffset),
                          static_cast<std::streamsize>(dataLength));
            if (!outFile.good()) {
                UVE_ERROR("AssetBundleUVE: failed to write \"{}\"", outputPath.string());
                writeFailed = true;
                return false;
            }
            return true; // keep extracting the rest of the entries
        });

    return scanOk && !writeFailed;
}

bool AssetBundleUVE::HasEntryUVE(const std::filesystem::path& bundlePath, std::string_view entryName) const {
    const std::optional<std::vector<std::byte>> payload = LoadBundlePayloadUVE(bundlePath);
    if (!payload.has_value()) {
        return false; // already logged
    }

    bool found = false;
    static_cast<void>(ScanBundleEntriesUVE(
        bundlePath, *payload,
        [&](std::string_view name, std::size_t /*dataOffset*/, std::uint64_t /*dataLength*/) -> bool {
            if (name == entryName) {
                found = true;
                return false; // stop scanning
            }
            return true;
        }));
    return found;
}

std::optional<std::vector<std::byte>> AssetBundleUVE::ReadEntryUVE(const std::filesystem::path& bundlePath,
                                                                     std::string_view entryName) const {
    const std::optional<std::vector<std::byte>> payload = LoadBundlePayloadUVE(bundlePath);
    if (!payload.has_value()) {
        return std::nullopt; // already logged
    }

    std::optional<std::vector<std::byte>> result;
    static_cast<void>(ScanBundleEntriesUVE(
        bundlePath, *payload,
        [&](std::string_view name, std::size_t dataOffset, std::uint64_t dataLength) -> bool {
            if (name != entryName) {
                return true;
            }
            result = std::vector<std::byte>(payload->begin() + static_cast<std::ptrdiff_t>(dataOffset),
                                             payload->begin() + static_cast<std::ptrdiff_t>(dataOffset + dataLength));
            return false; // stop scanning
        }));

    if (!result.has_value()) {
        UVE_ERROR("AssetBundleUVE: \"{}\" has no entry named \"{}\"", bundlePath.string(), entryName);
    }
    return result;
}

} // namespace UVE::Asset
