// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace UVE::Asset {

inline constexpr std::size_t kMaximumTgaDecodedPixelBytesUVE = 64U * 1024U * 1024U;

struct TgaRgba8ImageUVE final {
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    std::vector<std::byte> pixels;
};

/// Decodes bounded TGA images into canonical top-down RGBA8 pixels.
/// Supports image type 2 (uncompressed) and type 10 (run-length encoded) with 16-bit BGR5551 or BGR565,
/// 24-bit BGR, or 32-bit BGRA source pixels with descriptor-declared alpha preservation, 8-bit grayscale image types 3 and 11,
/// 16-bit grayscale+alpha image
/// types 3 and 11, and 8-bit or 16-bit indexed pixel data in image types 1 and 9 with 16-bit BGR5551 or BGR565, 24-bit BGR,
/// or 32-bit BGRA palette entries, with 16-bit BGR5551 and 32-bit BGRA indexed alpha conversion,
/// honors the origin bits,
/// and leaves the caller's output unchanged on every failure.
[[nodiscard]] bool DecodeTgaRgba8ImageUVE(const std::vector<std::byte>& bytes,
                                          TgaRgba8ImageUVE& outImage) noexcept;

} // namespace UVE::Asset
