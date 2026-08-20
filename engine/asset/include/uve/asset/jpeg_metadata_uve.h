// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once
#include <cstdint>
#include <optional>
#include <vector>
namespace UVE::Asset {

inline constexpr std::uint64_t kMaximumJpegDecodedPixelBytesUVE = 64ULL * 1024ULL * 1024ULL;
struct JpegMetadataUVE final {
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    std::uint8_t precision = 0U;
    std::uint8_t componentCount = 0U;
    bool progressive = false;
};
/// Validates the normalized RGBA8 decoded-pixel budget without entropy decoding or allocation.
[[nodiscard]] bool ValidateJpegRgba8PixelBudgetUVE(
    const JpegMetadataUVE& metadata,
    std::uint64_t maximumBytes = kMaximumJpegDecodedPixelBytesUVE) noexcept;

/// Parses bounded JPEG SOI/segment/SOF metadata from caller-owned bytes. It validates segment
/// lengths and frame dimensions but never decodes entropy data, owns files, allocates pixels, or
/// selects a texture/renderer backend.
[[nodiscard]] std::optional<JpegMetadataUVE> ParseJpegMetadataUVE(const std::vector<std::byte>& bytes);
} // namespace UVE::Asset
