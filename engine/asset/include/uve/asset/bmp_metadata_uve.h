// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace UVE::Asset {

inline constexpr std::size_t kMaximumBmpDecodedPixelBytesUVE = 64U * 1024U * 1024U;

struct BmpRgba8ImageUVE final {
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    std::vector<std::byte> pixels;
};

/// Decodes bounded Windows BMP BITMAPINFOHEADER/V4/V5 images with 1/4/8-bit indexed BGRA palette pixels, 4-bit BI_RLE4 or 8-bit BI_RLE8 indexed packets, 16-bit BI_RGB BGR555, exact-mask 16-bit BI_BITFIELDS BGR555/BGR565, exact-mask 32-bit BI_BITFIELDS BGRX or BI_ALPHABITFIELDS BGRA, 24-bit BGR, or 32-bit BGRA pixels,
/// BI_RGB, BI_RLE4, BI_RLE8, the supported BI_BITFIELDS forms, or BI_ALPHABITFIELDS, and bottom-up or top-down rows into copied top-down RGBA8 pixels with source alpha only for validated exact-mask BGRA bitfields.
/// Unsupported headers, malformed bounds, oversized dimensions, and allocation failures return false
/// without publishing partial output.
[[nodiscard]] bool DecodeBmpRgba8ImageUVE(
    const std::vector<std::byte>& bytes, BmpRgba8ImageUVE& outImage) noexcept;

} // namespace UVE::Asset
