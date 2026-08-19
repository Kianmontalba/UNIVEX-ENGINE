// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once
#include <cstdint>
#include <optional>
#include <vector>
namespace UVE::Asset {
struct JpegMetadataUVE final {
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    std::uint8_t precision = 0U;
    std::uint8_t componentCount = 0U;
    bool progressive = false;
};
/// Parses bounded JPEG SOI/segment/SOF metadata from caller-owned bytes. It validates segment
/// lengths and frame dimensions but never decodes entropy data, owns files, allocates pixels, or
/// selects a texture/renderer backend.
[[nodiscard]] std::optional<JpegMetadataUVE> ParseJpegMetadataUVE(const std::vector<std::byte>& bytes);
} // namespace UVE::Asset
