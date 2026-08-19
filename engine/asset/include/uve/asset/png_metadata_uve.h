// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace UVE::Asset {

struct PngMetadataUVE final {
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    std::uint8_t bitDepth = 0U;
    std::uint8_t colorType = 0U;
    std::uint8_t compressionMethod = 0U;
    std::uint8_t filterMethod = 0U;
    std::uint8_t interlaceMethod = 0U;
    bool hasAlpha = false;
};

/// Parses bounded PNG signature/IHDR metadata from copied caller-owned bytes. The parser validates
/// the first IHDR chunk and its supported color/depth/method combinations; it never decodes pixels,
/// verifies or owns a filesystem path, allocates GPU resources, or selects a texture backend.
[[nodiscard]] std::optional<PngMetadataUVE> ParsePngMetadataUVE(const std::vector<std::byte>& bytes);

} // namespace UVE::Asset
