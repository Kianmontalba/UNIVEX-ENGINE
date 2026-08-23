// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/bmp_metadata_uve.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <utility>

#include "uve/debug/logging_macros_uve.h"

namespace UVE::Asset {
namespace {

[[nodiscard]] bool CanReadUVE(const std::vector<std::byte>& bytes, const std::size_t offset,
                              const std::size_t size) noexcept {
    return offset <= bytes.size() && size <= bytes.size() - offset;
}

[[nodiscard]] std::uint16_t ReadU16LittleEndianUVE(const std::vector<std::byte>& bytes,
                                                   const std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes[offset])) |
           static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes[offset + 1U]) << 8U);
}

[[nodiscard]] std::uint32_t ReadU32LittleEndianUVE(const std::vector<std::byte>& bytes,
                                                   const std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[offset])) |
           (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[offset + 1U])) << 8U) |
           (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[offset + 2U])) << 16U) |
           (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[offset + 3U])) << 24U);
}

} // namespace

bool DecodeBmpRgba8ImageUVE(const std::vector<std::byte>& bytes, BmpRgba8ImageUVE& outImage) noexcept {
    constexpr std::size_t kFileHeaderBytesUVE = 14U;
    constexpr std::size_t kInfoHeaderBytesUVE = 40U;
    if (bytes.size() > kMaximumBmpDecodedPixelBytesUVE || !CanReadUVE(bytes, 0U, kFileHeaderBytesUVE) ||
        bytes[0] != std::byte{'B'} || bytes[1] != std::byte{'M'}) {
        return false;
    }

    const std::uint32_t declaredFileSize = ReadU32LittleEndianUVE(bytes, 2U);
    const std::uint32_t pixelOffset = ReadU32LittleEndianUVE(bytes, 10U);
    if (declaredFileSize != 0U && declaredFileSize > bytes.size()) {
        return false;
    }
    if (pixelOffset < kFileHeaderBytesUVE || !CanReadUVE(bytes, kFileHeaderBytesUVE, kInfoHeaderBytesUVE)) {
        return false;
    }

    const std::uint32_t infoHeaderSize = ReadU32LittleEndianUVE(bytes, kFileHeaderBytesUVE);
    if (infoHeaderSize < kInfoHeaderBytesUVE ||
        !CanReadUVE(bytes, kFileHeaderBytesUVE, static_cast<std::size_t>(infoHeaderSize))) {
        return false;
    }
    const std::int32_t signedWidth = static_cast<std::int32_t>(
        ReadU32LittleEndianUVE(bytes, kFileHeaderBytesUVE + 4U));
    const std::int32_t signedHeight = static_cast<std::int32_t>(
        ReadU32LittleEndianUVE(bytes, kFileHeaderBytesUVE + 8U));
    const std::uint16_t planes = ReadU16LittleEndianUVE(bytes, kFileHeaderBytesUVE + 12U);
    const std::uint16_t bitsPerPixel = ReadU16LittleEndianUVE(bytes, kFileHeaderBytesUVE + 14U);
    const std::uint32_t compression = ReadU32LittleEndianUVE(bytes, kFileHeaderBytesUVE + 16U);
    if (signedWidth <= 0 || signedHeight == 0 || planes != 1U ||
        (bitsPerPixel != 24U && bitsPerPixel != 32U) || compression != 0U) {
        return false;
    }

    const std::uint64_t width = static_cast<std::uint32_t>(signedWidth);
    const std::uint64_t absoluteHeight = signedHeight < 0
                                              ? static_cast<std::uint64_t>(-(static_cast<std::int64_t>(signedHeight)))
                                              : static_cast<std::uint32_t>(signedHeight);
    const std::uint64_t bytesPerPixel = bitsPerPixel / 8U;
    const std::uint64_t rawRowBytes = width * bytesPerPixel;
    const std::uint64_t rowStride = (rawRowBytes + 3U) & ~3U;
    const std::uint64_t pixelBytes = rowStride * absoluteHeight;
    const std::uint64_t decodedBytes = width * absoluteHeight * 4U;
    if (width > std::numeric_limits<std::uint32_t>::max() ||
        absoluteHeight > std::numeric_limits<std::uint32_t>::max() ||
        pixelBytes > std::numeric_limits<std::size_t>::max() ||
        decodedBytes > kMaximumBmpDecodedPixelBytesUVE ||
        static_cast<std::uint64_t>(pixelOffset) > bytes.size() ||
        pixelBytes > static_cast<std::uint64_t>(bytes.size() - static_cast<std::size_t>(pixelOffset))) {
        return false;
    }
    const std::uint64_t fileEnd = static_cast<std::uint64_t>(pixelOffset) + pixelBytes;
    if (declaredFileSize != 0U && fileEnd > declaredFileSize) {
        return false;
    }

    try {
        BmpRgba8ImageUVE candidate;
        candidate.width = static_cast<std::uint32_t>(width);
        candidate.height = static_cast<std::uint32_t>(absoluteHeight);
        candidate.pixels.resize(static_cast<std::size_t>(decodedBytes));
        const bool topDown = signedHeight < 0;
        const std::size_t sourceStride = static_cast<std::size_t>(rowStride);
        const std::size_t sourceOffset = static_cast<std::size_t>(pixelOffset);
        const std::size_t outputStride = static_cast<std::size_t>(width) * 4U;
        for (std::size_t outputRow = 0U; outputRow < static_cast<std::size_t>(absoluteHeight); ++outputRow) {
            const std::size_t sourceRow = topDown ? outputRow : static_cast<std::size_t>(absoluteHeight) - outputRow - 1U;
            const std::byte* const source = bytes.data() + sourceOffset + sourceRow * sourceStride;
            std::byte* const destination = candidate.pixels.data() + outputRow * outputStride;
            for (std::size_t column = 0U; column < static_cast<std::size_t>(width); ++column) {
                const std::byte* const pixel = source + column * static_cast<std::size_t>(bytesPerPixel);
                std::byte* const rgba = destination + column * 4U;
                rgba[0] = pixel[2];
                rgba[1] = pixel[1];
                rgba[2] = pixel[0];
                rgba[3] = std::byte{0xFF};
            }
        }
        outImage = std::move(candidate);
        return true;
    } catch (const std::bad_alloc&) {
        UVE_ERROR("BmpMetadataUVE: allocation failed while decoding bounded BMP image");
        return false;
    }
}

} // namespace UVE::Asset
