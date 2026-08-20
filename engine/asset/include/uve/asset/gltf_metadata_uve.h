// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace UVE::Asset {

enum class GltfContainerKindUVE : std::uint8_t { Json, Binary };

inline constexpr std::uint64_t kMaximumGltfAccessorElementsUVE = 1'000'000ULL;
inline constexpr std::size_t kMaximumGltfResourceUriBytesUVE = 4'096U;

enum class GltfResourceUriKindUVE : std::uint8_t { Invalid, RelativePath, DataUri };

/// Classifies one bounded glTF image/buffer URI without filesystem access or data decoding. Relative
/// paths must be traversal-safe; data URIs are recognized only as bounded non-empty tokens.
[[nodiscard]] GltfResourceUriKindUVE ClassifyGltfResourceUriUVE(std::string_view uri) noexcept;

/// Resolves one safe relative glTF resource URI against the asset's directory into a bounded VFS
/// path. Data URIs, filesystem I/O, and resource decoding are intentionally outside this contract.
[[nodiscard]] bool ResolveGltfResourceVirtualPathUVE(
    std::string_view assetVirtualPath, std::string_view resourceUri, std::string& outVirtualPath);

struct GltfMetadataUVE final {
    GltfContainerKindUVE container = GltfContainerKindUVE::Json;
    std::uint32_t nodeCount = 0U;
    std::uint32_t meshCount = 0U;
    std::uint32_t materialCount = 0U;
    std::uint32_t imageCount = 0U;
    std::uint32_t bufferCount = 0U;
    bool hasBinaryChunk = false;
};

/// Validates one copied accessor byte span against a caller-owned buffer length. The check is
/// overflow-safe and does not parse JSON, read buffers, decode components, or allocate output.
[[nodiscard]] bool ValidateGltfAccessorSpanUVE(
    std::uint64_t bufferByteLength,
    std::uint64_t byteOffset,
    std::uint64_t elementCount,
    std::uint64_t elementStride,
    std::uint64_t elementSize,
    std::uint64_t maximumElements = kMaximumGltfAccessorElementsUVE) noexcept;

/// Validates bounded glTF 2.0 JSON or GLB structure and returns copied top-level counts only.
/// It does not resolve buffers, decode images, convert meshes, load external resources, or own
/// renderer/importer state.
[[nodiscard]] std::optional<GltfMetadataUVE> ParseGltfMetadataUVE(std::string_view jsonSource);
[[nodiscard]] std::optional<GltfMetadataUVE> ParseGlbMetadataUVE(const std::vector<std::byte>& bytes);

} // namespace UVE::Asset
