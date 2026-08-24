// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace UVE::Asset {

inline constexpr std::uint64_t kMaximumPngDecodedPixelBytesUVE = 64ULL * 1024ULL * 1024ULL;
inline constexpr std::size_t kMaximumPngRgba8ScanlineBytesUVE = 4ULL * 1024ULL * 1024ULL;

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
enum class PngFilterTypeUVE : std::uint8_t { None = 0U, Sub = 1U, Up = 2U, Average = 3U, Paeth = 4U };

/// Reconstructs one bounded RGBA8 PNG scanline from its filtered bytes and an optional previous row.
/// It performs no zlib/IDAT decoding, chunk parsing, image allocation, filesystem I/O, or texture conversion.
[[nodiscard]] bool UnfilterPngRgba8ScanlineUVE(
    PngFilterTypeUVE filter, const std::vector<std::byte>& filteredBytes,
    const std::vector<std::byte>& previousRow, std::vector<std::byte>& outRow);

/// Checks the supported grayscale/grayscale-alpha/RGB/indexed/RGBA decoded-pixel budget after normalization to RGBA8,
/// including the bounded non-interlaced 1/2/4-bit grayscale/indexed and 16-bit grayscale/grayscale-alpha/RGB/RGBA source forms, without allocating or decoding pixel data.
[[nodiscard]] bool ValidatePngRgba8PixelBudgetUVE(
    const PngMetadataUVE& metadata,
    std::uint64_t maximumBytes = kMaximumPngDecodedPixelBytesUVE) noexcept;

struct PngRgba8ImageUVE final {
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    std::vector<std::byte> pixels;
};

/// Decodes bounded non-interlaced 1/2/4-bit grayscale/indexed, 8-bit grayscale/grayscale-alpha/RGB/indexed/RGBA, and 16-bit grayscale/grayscale-alpha/RGB/RGBA PNG forms using zlib and the existing scanline
/// unfilter primitive, normalizing grayscale/RGB input to opaque RGBA8, indexed input through validated PLTE/tRNS
/// palette facts, 16-bit grayscale/grayscale-alpha/RGB/RGBA input through big-endian high-byte down-conversion, and Adam7-interlaced
/// 8-bit grayscale/grayscale-alpha/RGB/indexed/RGBA plus Adam7-interlaced 8-bit grayscale/grayscale-alpha/RGB/indexed/RGBA and 16-bit grayscale/grayscale-alpha/RGB/RGBA input through bounded pass reconstruction,
/// validated palette mapping, high-byte conversion, and opaque-alpha normalization. It rejects unsupported
/// color/depth/interlace modes and owns no filesystem or GPU state.
[[nodiscard]] bool DecodePngRgba8ImageUVE(
    const std::vector<std::byte>& bytes, PngRgba8ImageUVE& outImage) noexcept;

} // namespace UVE::Asset
