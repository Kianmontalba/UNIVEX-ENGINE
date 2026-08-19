// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/gltf_metadata_uve.h"

#include <cstddef>
#include <cstdint>
#include <limits>

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
    const bool hasBinaryChunk = bytes.size() > 20U + jsonLength;
    return ParseJsonUVE(json, GltfContainerKindUVE::Binary, hasBinaryChunk);
}
} // namespace UVE::Asset
