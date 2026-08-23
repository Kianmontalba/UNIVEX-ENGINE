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

/// Decodes bounded Windows BMP BITMAPINFOHEADER images with 24-bit BGR or 32-bit BGRA pixels,
/// BI_RGB compression, and bottom-up or top-down rows into copied top-down opaque RGBA8 pixels.
/// Unsupported headers, malformed bounds, oversized dimensions, and allocation failures return false
/// without publishing partial output.
[[nodiscard]] bool DecodeBmpRgba8ImageUVE(
    const std::vector<std::byte>& bytes, BmpRgba8ImageUVE& outImage) noexcept;

} // namespace UVE::Asset
