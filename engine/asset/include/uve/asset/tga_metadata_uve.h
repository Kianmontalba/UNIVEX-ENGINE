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

/// Decodes bounded TGA images into canonical top-down opaque RGBA8 pixels.
/// Supports image type 2 (uncompressed) and type 10 (run-length encoded) with 24-bit BGR or
/// 32-bit BGRA source pixels, plus 8-bit grayscale image types 3 and 11 and 8-bit indexed image
/// types 1 and 9 with 24-bit BGR or 32-bit BGRA palette entries, honors the origin bits,
/// and leaves the caller's output unchanged on every failure.
[[nodiscard]] bool DecodeTgaRgba8ImageUVE(const std::vector<std::byte>& bytes,
                                          TgaRgba8ImageUVE& outImage) noexcept;

} // namespace UVE::Asset
