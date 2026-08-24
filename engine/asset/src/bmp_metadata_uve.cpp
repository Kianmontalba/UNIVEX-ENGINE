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
    const std::uint32_t colorsUsed = ReadU32LittleEndianUVE(bytes, kFileHeaderBytesUVE + 32U);
    const bool packed1Image = bitsPerPixel == 1U;
    const bool packed4Image = bitsPerPixel == 4U;
    const bool indexedImage = packed1Image || packed4Image || bitsPerPixel == 8U;
    const bool rle4Image = bitsPerPixel == 4U && compression == 2U;
    const bool rle8Image = bitsPerPixel == 8U && compression == 1U;
    const bool rleImage = rle4Image || rle8Image;
    const bool packed16Image = bitsPerPixel == 16U;
    const bool bitfields32BgrxImage = bitsPerPixel == 32U && compression == 3U;
    const bool bitfields32BgraImage = bitsPerPixel == 32U && compression == 6U;
    const bool bitfields32Image = bitfields32BgrxImage || bitfields32BgraImage;
    const bool bitfieldsImage = (packed16Image && compression == 3U) || bitfields32Image;
    if (signedWidth <= 0 || signedHeight == 0 || planes != 1U ||
        (!indexedImage && !packed16Image && bitsPerPixel != 24U && bitsPerPixel != 32U) ||
        (!bitfieldsImage && !rleImage && compression != 0U)) {
        return false;
    }

    const std::uint64_t width = static_cast<std::uint32_t>(signedWidth);
    const std::size_t paletteOffset = kFileHeaderBytesUVE + static_cast<std::size_t>(infoHeaderSize);
    const std::uint32_t paletteEntryCount = indexedImage
                                                  ? (colorsUsed == 0U ? (packed1Image ? 2U : (packed4Image ? 16U : 256U))
                                                                      : colorsUsed)
                                                  : 0U;
    constexpr std::size_t kBmpPaletteEntryBytesUVE = 4U;
    const std::uint32_t maximumPaletteEntries = packed1Image ? 2U : (packed4Image ? 16U : 256U);
    if (indexedImage && paletteEntryCount > maximumPaletteEntries) {
        return false;
    }
    const std::size_t paletteBytes = static_cast<std::size_t>(paletteEntryCount) * kBmpPaletteEntryBytesUVE;
    if (indexedImage && (!CanReadUVE(bytes, paletteOffset, paletteBytes) ||
                         static_cast<std::uint64_t>(pixelOffset) < static_cast<std::uint64_t>(paletteOffset) + paletteBytes)) {
        return false;
    }
    constexpr std::size_t kBmpBitfieldsOffsetUVE = kFileHeaderBytesUVE + kInfoHeaderBytesUVE;
    constexpr std::size_t kBmpBitfieldsBytesUVE = 12U;
    bool bitfields555Image = false;
    bool bitfields565Image = false;
    if (bitfieldsImage) {
        const std::size_t maskBytes = bitfields32BgraImage ? 16U : kBmpBitfieldsBytesUVE;
        if (infoHeaderSize != kInfoHeaderBytesUVE ||
            !CanReadUVE(bytes, kBmpBitfieldsOffsetUVE, maskBytes) ||
            static_cast<std::uint64_t>(pixelOffset) <
                static_cast<std::uint64_t>(kBmpBitfieldsOffsetUVE + maskBytes)) {
            return false;
        }
        const std::uint32_t redMask = ReadU32LittleEndianUVE(bytes, kBmpBitfieldsOffsetUVE);
        const std::uint32_t greenMask = ReadU32LittleEndianUVE(bytes, kBmpBitfieldsOffsetUVE + 4U);
        const std::uint32_t blueMask = ReadU32LittleEndianUVE(bytes, kBmpBitfieldsOffsetUVE + 8U);
        const std::uint32_t alphaMask = bitfields32BgraImage
            ? ReadU32LittleEndianUVE(bytes, kBmpBitfieldsOffsetUVE + 12U) : 0U;
        bitfields555Image = redMask == 0x7C00U && greenMask == 0x03E0U && blueMask == 0x001FU;
        bitfields565Image = redMask == 0xF800U && greenMask == 0x07E0U && blueMask == 0x001FU;
        const bool validBgrxMasks = bitfields32BgrxImage && redMask == 0x00FF0000U &&
                                    greenMask == 0x0000FF00U && blueMask == 0x000000FFU;
        const bool validBgraMasks = bitfields32BgraImage && redMask == 0x00FF0000U &&
                                    greenMask == 0x0000FF00U && blueMask == 0x000000FFU &&
                                    alphaMask == 0xFF000000U;
        if (!bitfields555Image && !bitfields565Image && !validBgrxMasks && !validBgraMasks) {
            return false;
        }
    }
    const std::uint64_t absoluteHeight = signedHeight < 0
                                              ? static_cast<std::uint64_t>(-(static_cast<std::int64_t>(signedHeight)))
                                              : static_cast<std::uint32_t>(signedHeight);
    const std::uint64_t bytesPerPixel = bitsPerPixel / 8U;
    const std::uint64_t rawRowBytes = packed1Image ? (width + 7U) / 8U
                                                    : (packed4Image ? (width + 1U) / 2U : width * bytesPerPixel);
    const std::uint64_t rowStride = (rawRowBytes + 3U) & ~3U;
    const std::uint64_t pixelBytes = rowStride * absoluteHeight;
    const std::uint64_t decodedBytes = width * absoluteHeight * 4U;
    if (width > std::numeric_limits<std::uint32_t>::max() ||
        absoluteHeight > std::numeric_limits<std::uint32_t>::max() ||
        decodedBytes > kMaximumBmpDecodedPixelBytesUVE ||
        static_cast<std::uint64_t>(pixelOffset) > bytes.size()) {
        return false;
    }
    if (!rleImage &&
        (pixelBytes > std::numeric_limits<std::size_t>::max() ||
         pixelBytes > static_cast<std::uint64_t>(bytes.size() - static_cast<std::size_t>(pixelOffset)))) {
        return false;
    }
    const std::uint64_t fileEnd = static_cast<std::uint64_t>(pixelOffset) + pixelBytes;
    if (!rleImage && declaredFileSize != 0U && fileEnd > declaredFileSize) {
        return false;
    }
    const std::size_t streamEnd = rleImage
                                      ? (declaredFileSize == 0U ? bytes.size() : static_cast<std::size_t>(declaredFileSize))
                                      : static_cast<std::size_t>(fileEnd);
    if (rleImage && streamEnd < static_cast<std::size_t>(pixelOffset)) {
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
        if (rleImage) {
            const auto writePalettePixel = [&](const std::uint64_t x, const std::uint64_t y,
                                               const std::uint8_t paletteIndex) noexcept {
                if (x >= width || y >= absoluteHeight || paletteIndex >= paletteEntryCount) {
                    return false;
                }
                const std::size_t outputRow = topDown
                                                  ? static_cast<std::size_t>(y)
                                                  : static_cast<std::size_t>(absoluteHeight - y - 1U);
                std::byte* const rgba = candidate.pixels.data() + outputRow * outputStride +
                                        static_cast<std::size_t>(x) * 4U;
                const std::size_t palettePixelOffset = paletteOffset +
                                                       static_cast<std::size_t>(paletteIndex) * kBmpPaletteEntryBytesUVE;
                rgba[0] = bytes[palettePixelOffset + 2U];
                rgba[1] = bytes[palettePixelOffset + 1U];
                rgba[2] = bytes[palettePixelOffset];
                rgba[3] = std::byte{0xFF};
                return true;
            };
            const std::byte defaultRed = bytes[paletteOffset + 2U];
            const std::byte defaultGreen = bytes[paletteOffset + 1U];
            const std::byte defaultBlue = bytes[paletteOffset];
            for (std::size_t outputOffset = 0U; outputOffset < candidate.pixels.size(); outputOffset += 4U) {
                candidate.pixels[outputOffset] = defaultRed;
                candidate.pixels[outputOffset + 1U] = defaultGreen;
                candidate.pixels[outputOffset + 2U] = defaultBlue;
                candidate.pixels[outputOffset + 3U] = std::byte{0xFF};
            }
            const std::size_t streamOffset = static_cast<std::size_t>(pixelOffset);
            std::size_t cursor = streamOffset;
            std::uint64_t x = 0U;
            std::uint64_t y = 0U;
            bool sawEndOfBitmap = false;
            while (!sawEndOfBitmap) {
                if (cursor > streamEnd || streamEnd - cursor < 2U) {
                    return false;
                }
                const std::uint8_t count = std::to_integer<std::uint8_t>(bytes[cursor++]);
                const std::uint8_t value = std::to_integer<std::uint8_t>(bytes[cursor++]);
                if (count != 0U) {
                    if (y >= absoluteHeight || x + count > width) {
                        return false;
                    }
                    for (std::uint8_t index = 0U; index < count; ++index) {
                        const std::uint8_t paletteIndex = rle4Image
                                                               ? static_cast<std::uint8_t>((index & 1U) == 0U
                                                                                                 ? value >> 4U
                                                                                                 : value & 0x0FU)
                                                               : value;
                        if (!writePalettePixel(x + index, y, paletteIndex)) {
                            return false;
                        }
                    }
                    x += count;
                    continue;
                }
                if (value == 0U) {
                    if (y >= absoluteHeight) {
                        return false;
                    }
                    x = 0U;
                    ++y;
                } else if (value == 1U) {
                    sawEndOfBitmap = true;
                } else if (value == 2U) {
                    if (streamEnd - cursor < 2U) {
                        return false;
                    }
                    const std::uint8_t deltaX = std::to_integer<std::uint8_t>(bytes[cursor++]);
                    const std::uint8_t deltaY = std::to_integer<std::uint8_t>(bytes[cursor++]);
                    if (x + deltaX > width || y + deltaY > absoluteHeight) {
                        return false;
                    }
                    x += deltaX;
                    y += deltaY;
                } else {
                    const std::size_t absoluteCount = value;
                    const std::size_t packedBytes = rle4Image ? (absoluteCount + 1U) / 2U : absoluteCount;
                    if (y >= absoluteHeight || x + absoluteCount > width ||
                        streamEnd - cursor < packedBytes ||
                        ((packedBytes & 1U) != 0U && streamEnd - cursor < packedBytes + 1U)) {
                        return false;
                    }
                    for (std::size_t index = 0U; index < absoluteCount; ++index) {
                        const std::uint8_t packedIndices =
                            std::to_integer<std::uint8_t>(bytes[cursor + (rle4Image ? index / 2U : index)]);
                        const std::uint8_t paletteIndex = rle4Image
                                                               ? static_cast<std::uint8_t>((index & 1U) == 0U
                                                                                                 ? packedIndices >> 4U
                                                                                                 : packedIndices & 0x0FU)
                                                               : packedIndices;
                        if (!writePalettePixel(x + index, y, paletteIndex)) {
                            return false;
                        }
                    }
                    cursor += packedBytes;
                    if ((packedBytes & 1U) != 0U) {
                        ++cursor;
                    }
                    x += absoluteCount;
                }
            }
            if (!sawEndOfBitmap) {
                return false;
            }
        } else {
            for (std::size_t outputRow = 0U; outputRow < static_cast<std::size_t>(absoluteHeight); ++outputRow) {
                const std::size_t sourceRow = topDown ? outputRow : static_cast<std::size_t>(absoluteHeight) - outputRow - 1U;
                const std::byte* const source = bytes.data() + sourceOffset + sourceRow * sourceStride;
                std::byte* const destination = candidate.pixels.data() + outputRow * outputStride;
                for (std::size_t column = 0U; column < static_cast<std::size_t>(width); ++column) {
                    const std::byte* const pixel = source +
                                                     (packed1Image ? column / 8U
                                                                   : (packed4Image ? column / 2U
                                                                                   : column * static_cast<std::size_t>(bytesPerPixel)));
                    std::byte* const rgba = destination + column * 4U;
                    if (indexedImage) {
                        const std::uint8_t packedIndices = std::to_integer<std::uint8_t>(*pixel);
                        const std::uint8_t paletteIndex = packed1Image
                                                               ? static_cast<std::uint8_t>((packedIndices >> (7U - (column & 7U))) & 0x01U)
                                                               : (packed4Image
                                                                      ? static_cast<std::uint8_t>((packedIndices >>
                                                                                                   ((column & 1U) == 0U ? 4U : 0U)) &
                                                                                                  0x0FU)
                                                                      : packedIndices);
                        if (paletteIndex >= paletteEntryCount) {
                            return false;
                        }
                        const std::size_t palettePixelOffset = paletteOffset +
                                                               static_cast<std::size_t>(paletteIndex) * kBmpPaletteEntryBytesUVE;
                        rgba[0] = bytes[palettePixelOffset + 2U];
                        rgba[1] = bytes[palettePixelOffset + 1U];
                        rgba[2] = bytes[palettePixelOffset];
                    } else if (bitfields32Image) {
                        const std::uint32_t packed = static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(pixel[0])) |
                                                      (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(pixel[1])) << 8U) |
                                                      (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(pixel[2])) << 16U) |
                                                      (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(pixel[3])) << 24U);
                        rgba[0] = std::byte{static_cast<unsigned char>((packed >> 16U) & 0xFFU)};
                        rgba[1] = std::byte{static_cast<unsigned char>((packed >> 8U) & 0xFFU)};
                        rgba[2] = std::byte{static_cast<unsigned char>(packed & 0xFFU)};
                    } else if (packed16Image) {
                        const std::uint16_t packed = static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(pixel[0])) |
                                                      static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(pixel[1]) << 8U);
                        const auto expandFiveBit = [](const std::uint16_t value) noexcept {
                            return std::byte{static_cast<unsigned char>((value * 255U + 15U) / 31U)};
                        };
                        const auto expandSixBit = [](const std::uint16_t value) noexcept {
                            return std::byte{static_cast<unsigned char>((value * 255U + 31U) / 63U)};
                        };
                        if (bitfields565Image) {
                            rgba[0] = expandFiveBit((packed >> 11U) & 0x1FU);
                            rgba[1] = expandSixBit((packed >> 5U) & 0x3FU);
                            rgba[2] = expandFiveBit(packed & 0x1FU);
                        } else {
                            rgba[0] = expandFiveBit((packed >> 10U) & 0x1FU);
                            rgba[1] = expandFiveBit((packed >> 5U) & 0x1FU);
                            rgba[2] = expandFiveBit(packed & 0x1FU);
                        }
                    } else {
                        rgba[0] = pixel[2];
                        rgba[1] = pixel[1];
                        rgba[2] = pixel[0];
                    }
                    rgba[3] = bitfields32BgraImage ? pixel[3] : std::byte{0xFF};
                }
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
