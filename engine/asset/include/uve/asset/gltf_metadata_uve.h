// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace UVE::Asset {

enum class GltfContainerKindUVE : std::uint8_t { Json, Binary }; 

struct GltfMetadataUVE final {
    GltfContainerKindUVE container = GltfContainerKindUVE::Json;
    std::uint32_t nodeCount = 0U;
    std::uint32_t meshCount = 0U;
    std::uint32_t materialCount = 0U;
    std::uint32_t imageCount = 0U;
    std::uint32_t bufferCount = 0U;
    bool hasBinaryChunk = false;
};

/// Validates bounded glTF 2.0 JSON or GLB structure and returns copied top-level counts only.
/// It does not resolve buffers, decode images, convert meshes, load external resources, or own
/// renderer/importer state.
[[nodiscard]] std::optional<GltfMetadataUVE> ParseGltfMetadataUVE(std::string_view jsonSource);
[[nodiscard]] std::optional<GltfMetadataUVE> ParseGlbMetadataUVE(const std::vector<std::byte>& bytes);

} // namespace UVE::Asset
