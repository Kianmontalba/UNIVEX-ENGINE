// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/gltf_metadata_uve.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <utility>

#include <nlohmann/json.hpp>

namespace UVE::Asset {
namespace {
constexpr std::uint32_t kMaximumCountUVE = 1'000'000U;
[[nodiscard]] std::uint32_t ReadU32LE(const std::vector<std::byte>& bytes, const std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(bytes[offset]) |
                                      (std::to_integer<std::uint32_t>(bytes[offset + 1U]) << 8U) |
                                      (std::to_integer<std::uint32_t>(bytes[offset + 2U]) << 16U) |
                                      (std::to_integer<std::uint32_t>(bytes[offset + 3U]) << 24U));
}
[[nodiscard]] std::optional<std::uint32_t> CountArrayUVE(const nlohmann::json& document, const char* key) {
    if (!document.contains(key)) return 0U;
    const auto& value = document.at(key);
    if (!value.is_array() || value.size() > kMaximumCountUVE) return std::nullopt;
    return static_cast<std::uint32_t>(value.size());
}
[[nodiscard]] std::optional<GltfMetadataUVE> ParseJsonUVE(const std::string_view source, const GltfContainerKindUVE kind,
                                                          const bool hasBinaryChunk) {
    try {
        const nlohmann::json document = nlohmann::json::parse(source);
        if (!document.is_object() || !document.contains("asset") || !document.at("asset").is_object() ||
            !document.at("asset").contains("version") || !document.at("asset").at("version").is_string() ||
            document.at("asset").at("version").get<std::string>() != "2.0") return std::nullopt;
        GltfMetadataUVE metadata;
        metadata.container = kind;
        metadata.hasBinaryChunk = hasBinaryChunk;
        const auto nodes = CountArrayUVE(document, "nodes");
        const auto meshes = CountArrayUVE(document, "meshes");
        const auto materials = CountArrayUVE(document, "materials");
        const auto images = CountArrayUVE(document, "images");
        const auto buffers = CountArrayUVE(document, "buffers");
        if (!nodes || !meshes || !materials || !images || !buffers) return std::nullopt;
        metadata.nodeCount = *nodes; metadata.meshCount = *meshes; metadata.materialCount = *materials;
        metadata.imageCount = *images; metadata.bufferCount = *buffers;
        return metadata;
    } catch (const nlohmann::json::exception&) { return std::nullopt; }
}
} // namespace

GltfResourceUriKindUVE ClassifyGltfResourceUriUVE(const std::string_view uri) noexcept {
    if (uri.empty() || uri.size() > kMaximumGltfResourceUriBytesUVE) {
        return GltfResourceUriKindUVE::Invalid;
    }
    if (uri.starts_with("data:")) {
        if (uri.find(',') == std::string_view::npos) {
            return GltfResourceUriKindUVE::Invalid;
        }
        for (const char rawCharacter : uri) {
            const unsigned char character = static_cast<unsigned char>(rawCharacter);
            if (character == 0U || character < 0x20U || character == 0x7FU) {
                return GltfResourceUriKindUVE::Invalid;
            }
        }
        return GltfResourceUriKindUVE::DataUri;
    }
    if (uri.front() == '/' || uri.front() == '\\') {
        return GltfResourceUriKindUVE::Invalid;
    }
    std::size_t segmentStart = 0U;
    for (std::size_t index = 0U; index < uri.size(); ++index) {
        const unsigned char character = static_cast<unsigned char>(uri[index]);
        if (character == 0U || character < 0x20U || character == 0x7FU || character == ':') {
            return GltfResourceUriKindUVE::Invalid;
        }
        if (character == '%') {
            if (index + 2U >= uri.size()) {
                return GltfResourceUriKindUVE::Invalid;
            }
            const char first = uri[index + 1U];
            const char second = uri[index + 2U];
            const auto isHex = [](const char value) noexcept {
                return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f') ||
                       (value >= 'A' && value <= 'F');
            };
            if (!isHex(first) || !isHex(second)) {
                return GltfResourceUriKindUVE::Invalid;
            }
            const auto hexValue = [](const char value) noexcept -> unsigned int {
                if (value >= '0' && value <= '9') return static_cast<unsigned int>(value - '0');
                if (value >= 'a' && value <= 'f') return static_cast<unsigned int>(value - 'a' + 10);
                return static_cast<unsigned int>(value - 'A' + 10);
            };
            const unsigned int decoded = (hexValue(first) << 4U) | hexValue(second);
            if (decoded == 0U || decoded == '.' || decoded == '/' || decoded == '\\') {
                return GltfResourceUriKindUVE::Invalid;
            }
            index += 2U;
            continue;
        }
        if (character != '/' && character != '\\') {
            continue;
        }
        const std::size_t segmentLength = index - segmentStart;
        if (segmentLength == 0U || (segmentLength == 1U && uri[segmentStart] == '.') ||
            (segmentLength == 2U && uri[segmentStart] == '.' && uri[segmentStart + 1U] == '.')) {
            return GltfResourceUriKindUVE::Invalid;
        }
        segmentStart = index + 1U;
    }
    const std::size_t finalSegmentLength = uri.size() - segmentStart;
    if (finalSegmentLength == 0U ||
        (finalSegmentLength == 1U && uri[segmentStart] == '.') ||
        (finalSegmentLength == 2U && uri[segmentStart] == '.' && uri[segmentStart + 1U] == '.')) {
        return GltfResourceUriKindUVE::Invalid;
    }
    return GltfResourceUriKindUVE::RelativePath;
}

bool DecodeGltfDataUriUVE(const std::string_view uri, std::vector<std::byte>& outBytes,
                          const std::size_t maximumBytes) {
    if (ClassifyGltfResourceUriUVE(uri) != GltfResourceUriKindUVE::DataUri) {
        return false;
    }
    const std::size_t comma = uri.find(',');
    const std::string_view metadata = uri.substr(5U, comma - 5U);
    const std::string_view payload = uri.substr(comma + 1U);
    const bool isBase64 = metadata.ends_with(";base64");
    try {
        std::vector<std::byte> decoded;
        const auto appendByte = [&](const unsigned int value) {
            if (decoded.size() >= maximumBytes) {
                return false;
            }
            decoded.push_back(std::byte{static_cast<unsigned char>(value)});
            return true;
        };
        if (!isBase64) {
            for (std::size_t index = 0U; index < payload.size(); ++index) {
                unsigned int value = static_cast<unsigned char>(payload[index]);
                if (payload[index] == '%') {
                    if (index + 2U >= payload.size()) {
                        return false;
                    }
                    const auto hexValue = [](const char character) noexcept -> int {
                        if (character >= '0' && character <= '9') return character - '0';
                        if (character >= 'a' && character <= 'f') return character - 'a' + 10;
                        if (character >= 'A' && character <= 'F') return character - 'A' + 10;
                        return -1;
                    };
                    const int high = hexValue(payload[index + 1U]);
                    const int low = hexValue(payload[index + 2U]);
                    if (high < 0 || low < 0) {
                        return false;
                    }
                    value = static_cast<unsigned int>((high << 4) | low);
                    index += 2U;
                }
                if (!appendByte(value)) {
                    return false;
                }
            }
        } else {
            if (payload.size() % 4U != 0U) {
                return false;
            }
            const auto base64Value = [](const char character) noexcept -> int {
                if (character >= 'A' && character <= 'Z') return character - 'A';
                if (character >= 'a' && character <= 'z') return character - 'a' + 26;
                if (character >= '0' && character <= '9') return character - '0' + 52;
                if (character == '+') return 62;
                if (character == '/') return 63;
                return -1;
            };
            for (std::size_t index = 0U; index < payload.size(); index += 4U) {
                const char first = payload[index];
                const char second = payload[index + 1U];
                const char third = payload[index + 2U];
                const char fourth = payload[index + 3U];
                const int firstValue = base64Value(first);
                const int secondValue = base64Value(second);
                const bool thirdPadding = third == '=';
                const bool fourthPadding = fourth == '=';
                if (firstValue < 0 || secondValue < 0 || (thirdPadding && !fourthPadding) ||
                    (!thirdPadding && base64Value(third) < 0) || (!fourthPadding && base64Value(fourth) < 0) ||
                    (thirdPadding && index + 4U != payload.size())) {
                    return false;
                }
                if (thirdPadding && (secondValue & 0x0F) != 0) {
                    return false;
                }
                if (fourthPadding && !thirdPadding && (base64Value(third) & 0x03) != 0) {
                    return false;
                }
                if (!appendByte(static_cast<unsigned int>((firstValue << 2) | (secondValue >> 4)))) {
                    return false;
                }
                if (thirdPadding) {
                    continue;
                }
                const int thirdValue = base64Value(third);
                if (!appendByte(static_cast<unsigned int>((secondValue << 4) | (thirdValue >> 2)))) {
                    return false;
                }
                if (fourthPadding) {
                    continue;
                }
                const int fourthValue = base64Value(fourth);
                if (!appendByte(static_cast<unsigned int>((thirdValue << 6) | fourthValue))) {
                    return false;
                }
            }
        }
        outBytes = std::move(decoded);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    }
}

bool ResolveGltfResourceVirtualPathUVE(const std::string_view assetVirtualPath,
                                          const std::string_view resourceUri,
                                          std::string& outVirtualPath) {
    if (assetVirtualPath.empty() || assetVirtualPath.size() > kMaximumGltfResourceUriBytesUVE ||
        resourceUri.empty() || resourceUri.size() > kMaximumGltfResourceUriBytesUVE ||
        ClassifyGltfResourceUriUVE(assetVirtualPath) != GltfResourceUriKindUVE::RelativePath ||
        ClassifyGltfResourceUriUVE(resourceUri) != GltfResourceUriKindUVE::RelativePath) {
        return false;
    }
    const std::size_t separator = assetVirtualPath.rfind('/');
    const std::size_t directoryBytes = separator == std::string_view::npos ? 0U : separator + 1U;
    if (directoryBytes + resourceUri.size() > kMaximumGltfResourceUriBytesUVE) {
        return false;
    }
    try {
        std::string resolved;
        resolved.reserve(directoryBytes + resourceUri.size());
        resolved.append(assetVirtualPath.substr(0U, directoryBytes));
        for (const char character : resourceUri) {
            resolved.push_back(character == '\\' ? '/' : character);
        }
        outVirtualPath = std::move(resolved);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    }
}

bool ValidateGltfAccessorSpanUVE(const std::uint64_t bufferByteLength,
                                 const std::uint64_t byteOffset,
                                 const std::uint64_t elementCount,
                                 const std::uint64_t elementStride,
                                 const std::uint64_t elementSize,
                                 const std::uint64_t maximumElements) noexcept {
    if (byteOffset > bufferByteLength || elementSize == 0U || elementStride < elementSize ||
        elementCount > maximumElements) {
        return false;
    }
    if (elementCount == 0U) {
        return true;
    }
    const std::uint64_t remainingBytes = bufferByteLength - byteOffset;
    const std::uint64_t trailingElements = elementCount - 1U;
    if (trailingElements > remainingBytes / elementStride) {
        return false;
    }
    const std::uint64_t lastElementOffset = trailingElements * elementStride;
    return elementSize <= remainingBytes - lastElementOffset;
}

std::optional<GltfMetadataUVE> ParseGltfMetadataUVE(const std::string_view jsonSource) {
    return ParseJsonUVE(jsonSource, GltfContainerKindUVE::Json, false);
}
std::optional<GltfMetadataUVE> ParseGlbMetadataUVE(const std::vector<std::byte>& bytes) {
    constexpr std::size_t kHeaderBytes = 12U;
    constexpr std::size_t kChunkHeaderBytes = 8U;
    if (bytes.size() < kHeaderBytes || ReadU32LE(bytes, 0U) != 0x46546C67U || ReadU32LE(bytes, 4U) != 2U) return std::nullopt;
    const std::uint32_t length = ReadU32LE(bytes, 8U);
    if (length != bytes.size() || bytes.size() < kHeaderBytes + kChunkHeaderBytes) return std::nullopt;
    const std::uint32_t jsonLength = ReadU32LE(bytes, 12U);
    if (ReadU32LE(bytes, 16U) != 0x4E4F534AU || jsonLength > bytes.size() - 20U) return std::nullopt;
    const auto* begin = reinterpret_cast<const char*>(bytes.data() + 20U);
    const std::string_view json{begin, jsonLength};
    bool hasBinaryChunk = false;
    std::size_t offset = 20U + jsonLength;
    while (offset < bytes.size()) {
        if (bytes.size() - offset < kChunkHeaderBytes) return std::nullopt;
        const std::uint32_t chunkLength = ReadU32LE(bytes, offset);
        const std::uint32_t chunkType = ReadU32LE(bytes, offset + 4U);
        offset += kChunkHeaderBytes;
        if (static_cast<std::size_t>(chunkLength) > bytes.size() - offset) return std::nullopt;
        if (chunkType == 0x004E4942U) {
            if (hasBinaryChunk) return std::nullopt;
            hasBinaryChunk = true;
        }
        offset += static_cast<std::size_t>(chunkLength);
    }
    return ParseJsonUVE(json, GltfContainerKindUVE::Binary, hasBinaryChunk);
}
} // namespace UVE::Asset
