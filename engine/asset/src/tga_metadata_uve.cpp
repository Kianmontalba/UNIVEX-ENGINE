// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/tga_metadata_uve.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <utility>

namespace UVE::Asset {
namespace {

constexpr std::size_t kTgaHeaderBytesUVE = 18U;
constexpr std::uint8_t kTgaTrueColorImageTypeUVE = 2U;
constexpr std::uint8_t kTgaRleTrueColorImageTypeUVE = 10U;
constexpr std::uint8_t kTgaTrueColor16BitDepthUVE = 16U;
constexpr std::uint8_t kTgaGrayscaleImageTypeUVE = 3U;
constexpr std::uint8_t kTgaColorMappedImageTypeUVE = 1U;
constexpr std::uint8_t kTgaRleColorMappedImageTypeUVE = 9U;
constexpr std::uint8_t kTgaRleGrayscaleImageTypeUVE = 11U;
constexpr std::uint8_t kTgaTopOriginBitUVE = 0x20U;
constexpr std::uint8_t kTgaRightOriginBitUVE = 0x10U;
constexpr std::uint8_t kTgaUnsupportedInterleaveBitsUVE = 0xC0U;

[[nodiscard]] std::uint16_t ReadU16LittleEndianUVE(const std::vector<std::byte>& bytes,
                                                   const std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes[offset]) |
                                      (static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes[offset + 1U]))
                                       << 8U));
}

} // namespace

bool DecodeTgaRgba8ImageUVE(const std::vector<std::byte>& bytes,
                            TgaRgba8ImageUVE& outImage) noexcept {
    if (bytes.size() > kMaximumTgaDecodedPixelBytesUVE || bytes.size() < kTgaHeaderBytesUVE) {
        return false;
    }

    const std::uint8_t colorMapType = std::to_integer<std::uint8_t>(bytes[1]);
    const std::uint8_t imageType = std::to_integer<std::uint8_t>(bytes[2]);
    const std::uint16_t width = ReadU16LittleEndianUVE(bytes, 12U);
    const std::uint16_t height = ReadU16LittleEndianUVE(bytes, 14U);
    const std::uint8_t pixelDepth = std::to_integer<std::uint8_t>(bytes[16]);
    const std::uint8_t imageDescriptor = std::to_integer<std::uint8_t>(bytes[17]);
    const std::uint16_t colorMapFirstIndex = ReadU16LittleEndianUVE(bytes, 3U);
    const std::uint16_t colorMapLength = ReadU16LittleEndianUVE(bytes, 5U);
    const std::uint8_t colorMapEntryDepth = std::to_integer<std::uint8_t>(bytes[7]);
    const bool grayscaleImage = imageType == kTgaGrayscaleImageTypeUVE || imageType == kTgaRleGrayscaleImageTypeUVE;
    const bool paletteImage = imageType == kTgaColorMappedImageTypeUVE || imageType == kTgaRleColorMappedImageTypeUVE;
    const bool trueColor15Image = !grayscaleImage && !paletteImage &&
                                  (imageType == kTgaTrueColorImageTypeUVE || imageType == kTgaRleTrueColorImageTypeUVE) &&
                                  pixelDepth == 15U;
    const bool trueColor16Image = !grayscaleImage && !paletteImage &&
                                  (imageType == kTgaTrueColorImageTypeUVE || imageType == kTgaRleTrueColorImageTypeUVE) &&
                                  pixelDepth == kTgaTrueColor16BitDepthUVE;
    const bool palette16Image = paletteImage && colorMapEntryDepth == kTgaTrueColor16BitDepthUVE;
    const bool palette32AlphaImage = paletteImage && colorMapEntryDepth == 32U && (imageDescriptor & 0x0FU) == 8U;
    const bool trueColor32AlphaImage = !grayscaleImage && !paletteImage &&
                                       (imageType == kTgaTrueColorImageTypeUVE || imageType == kTgaRleTrueColorImageTypeUVE) &&
                                       pixelDepth == 32U && (imageDescriptor & 0x0FU) == 8U;
    const bool bgraAlphaImage = palette32AlphaImage || trueColor32AlphaImage;
    const bool packed16Image = trueColor15Image || trueColor16Image || palette16Image;
    const bool bgr555Image = trueColor15Image;
    const bool bgr5551Image = !trueColor15Image && packed16Image && (imageDescriptor & 0x0FU) == 1U;
    const bool rleImage = imageType == kTgaRleTrueColorImageTypeUVE || imageType == kTgaRleGrayscaleImageTypeUVE ||
                          imageType == kTgaRleColorMappedImageTypeUVE;
    const bool supportedImageType = imageType == kTgaTrueColorImageTypeUVE ||
                                    imageType == kTgaRleTrueColorImageTypeUVE || grayscaleImage || paletteImage;
    const bool supportedPixelDepth = paletteImage ? (pixelDepth == 8U || pixelDepth == 16U)
                                                   : grayscaleImage ? (pixelDepth == 8U || pixelDepth == 16U)
                                                                     : (pixelDepth == 15U || pixelDepth == 16U || pixelDepth == 24U || pixelDepth == 32U);
    const bool supportedPacked16Descriptor = !packed16Image ||
                                             (trueColor15Image ? (imageDescriptor & 0x0FU) == 0U
                                                               : (imageDescriptor & 0x0FU) <= 1U);
    const bool supportedColorMap = paletteImage ? colorMapType == 1U && colorMapLength > 0U &&
                                                     (colorMapEntryDepth == 16U || colorMapEntryDepth == 24U ||
                                                      colorMapEntryDepth == 32U)
                                               : colorMapType == 0U;
    if (!supportedImageType || width == 0U || height == 0U || !supportedPixelDepth || !supportedColorMap ||
        !supportedPacked16Descriptor || (imageDescriptor & kTgaUnsupportedInterleaveBitsUVE) != 0U) {
        return false;
    }

    const std::size_t bytesPerPixel = pixelDepth == 15U ? 2U : pixelDepth / 8U;
    constexpr std::size_t kMaximumSizeT = std::numeric_limits<std::size_t>::max();
    if (static_cast<std::size_t>(width) > kMaximumSizeT / bytesPerPixel) {
        return false;
    }
    const std::size_t rowBytes = static_cast<std::size_t>(width) * bytesPerPixel;
    if (static_cast<std::size_t>(height) > kMaximumSizeT / rowBytes) {
        return false;
    }
    const std::size_t sourcePixelBytes = rowBytes * static_cast<std::size_t>(height);
    const std::size_t idLength = std::to_integer<std::uint8_t>(bytes[0]);
    if (idLength > kMaximumSizeT - kTgaHeaderBytesUVE) {
        return false;
    }
    const std::size_t colorMapOffset = kTgaHeaderBytesUVE + idLength;
    if (colorMapOffset > bytes.size()) {
        return false;
    }
    std::size_t colorMapBytes = 0U;
    std::size_t colorMapEntryBytes = 0U;
    if (paletteImage) {
        colorMapEntryBytes = colorMapEntryDepth / 8U;
        if (static_cast<std::size_t>(colorMapLength) > kMaximumSizeT / colorMapEntryBytes) {
            return false;
        }
        colorMapBytes = static_cast<std::size_t>(colorMapLength) * colorMapEntryBytes;
        if (colorMapBytes > bytes.size() - colorMapOffset) {
            return false;
        }
    }
    const std::size_t pixelOffset = colorMapOffset + colorMapBytes;
    if (!rleImage &&
        (sourcePixelBytes > kMaximumSizeT - pixelOffset ||
         pixelOffset + sourcePixelBytes > bytes.size())) {
        return false;
    }
    if (static_cast<std::size_t>(width) > kMaximumSizeT / static_cast<std::size_t>(height)) {
        return false;
    }
    const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (pixelCount > kMaximumSizeT / 4U) {
        return false;
    }
    const std::size_t outputBytes = pixelCount * 4U;
    if (outputBytes > kMaximumTgaDecodedPixelBytesUVE) {
        return false;
    }

    std::vector<std::byte> candidatePixels;
    try {
        candidatePixels.resize(outputBytes);
    } catch (const std::bad_alloc&) {
        return false;
    }

    const bool topOrigin = (imageDescriptor & kTgaTopOriginBitUVE) != 0U;
    const bool rightOrigin = (imageDescriptor & kTgaRightOriginBitUVE) != 0U;
    const bool grayscaleAlphaImage = grayscaleImage && pixelDepth == 16U;
    const auto writePixel = [&](const std::size_t decodedIndex, const std::size_t sourceOffset) noexcept -> bool {
        const std::size_t sourceY = decodedIndex / static_cast<std::size_t>(width);
        const std::size_t sourceX = decodedIndex % static_cast<std::size_t>(width);
        const std::size_t outputY = topOrigin ? sourceY : static_cast<std::size_t>(height) - 1U - sourceY;
        const std::size_t outputX = rightOrigin ? static_cast<std::size_t>(width) - 1U - sourceX : sourceX;
        const std::size_t outputOffset = (outputY * static_cast<std::size_t>(width) + outputX) * 4U;
        std::size_t colorOffset = sourceOffset;
        if (paletteImage) {
            const std::uint16_t paletteIndex = pixelDepth == 8U
                                                    ? static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes[sourceOffset]))
                                                    : ReadU16LittleEndianUVE(bytes, sourceOffset);
            if (paletteIndex < colorMapFirstIndex ||
                static_cast<std::size_t>(paletteIndex - colorMapFirstIndex) >= static_cast<std::size_t>(colorMapLength)) {
                return false;
            }
            colorOffset = colorMapOffset + static_cast<std::size_t>(paletteIndex - colorMapFirstIndex) * colorMapEntryBytes;
        }
        if (bgraAlphaImage) {
            candidatePixels[outputOffset] = bytes[colorOffset + 2U];
            candidatePixels[outputOffset + 1U] = bytes[colorOffset + 1U];
            candidatePixels[outputOffset + 2U] = bytes[colorOffset];
            candidatePixels[outputOffset + 3U] = bytes[colorOffset + 3U];
        } else if (packed16Image) {
            const std::uint16_t packed = static_cast<std::uint16_t>(
                std::to_integer<std::uint8_t>(bytes[colorOffset]) |
                (static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes[colorOffset + 1U])) << 8U));
            const auto expandFiveBit = [](const std::uint16_t value) noexcept {
                return std::byte{static_cast<unsigned char>((value * 255U + 15U) / 31U)};
            };
            if (bgr5551Image || bgr555Image) {
                candidatePixels[outputOffset] = expandFiveBit((packed >> 10U) & 0x1FU);
                candidatePixels[outputOffset + 1U] = expandFiveBit((packed >> 5U) & 0x1FU);
                candidatePixels[outputOffset + 2U] = expandFiveBit(packed & 0x1FU);
                candidatePixels[outputOffset + 3U] = bgr5551Image && (packed & 0x8000U) == 0U ? std::byte{0x00}
                                                                                               : std::byte{0xFF};
            } else {
                const auto expandSixBit = [](const std::uint16_t value) noexcept {
                    return std::byte{static_cast<unsigned char>((value * 255U + 31U) / 63U)};
                };
                candidatePixels[outputOffset] = expandFiveBit((packed >> 11U) & 0x1FU);
                candidatePixels[outputOffset + 1U] = expandSixBit((packed >> 5U) & 0x3FU);
                candidatePixels[outputOffset + 2U] = expandFiveBit(packed & 0x1FU);
                candidatePixels[outputOffset + 3U] = std::byte{0xFF};
            }
        } else if (grayscaleImage) {
            candidatePixels[outputOffset] = bytes[colorOffset];
            candidatePixels[outputOffset + 1U] = bytes[colorOffset];
            candidatePixels[outputOffset + 2U] = bytes[colorOffset];
            candidatePixels[outputOffset + 3U] = grayscaleAlphaImage ? bytes[colorOffset + 1U] : std::byte{0xFF};
        } else {
            candidatePixels[outputOffset] = bytes[colorOffset + 2U];
            candidatePixels[outputOffset + 1U] = bytes[colorOffset + 1U];
            candidatePixels[outputOffset + 2U] = bytes[colorOffset];
            candidatePixels[outputOffset + 3U] = std::byte{0xFF};
        }
        return true;
    };

    std::size_t encodedOffset = pixelOffset;
    std::size_t decodedPixelCount = 0U;
    while (decodedPixelCount < pixelCount) {
        if (!rleImage) {
            if (bytes.size() - encodedOffset < bytesPerPixel) {
                return false;
            }
            if (!writePixel(decodedPixelCount, encodedOffset)) {
                return false;
            }
            encodedOffset += bytesPerPixel;
            ++decodedPixelCount;
            continue;
        }

        if (encodedOffset >= bytes.size()) {
            return false;
        }
        const std::uint8_t packetHeader = std::to_integer<std::uint8_t>(bytes[encodedOffset]);
        ++encodedOffset;
        const std::size_t packetPixelCount = static_cast<std::size_t>(packetHeader & 0x7FU) + 1U;
        if (packetPixelCount > pixelCount - decodedPixelCount) {
            return false;
        }
        if ((packetHeader & 0x80U) == 0U) {
            if (packetPixelCount > (bytes.size() - encodedOffset) / bytesPerPixel) {
                return false;
            }
            for (std::size_t packetIndex = 0U; packetIndex < packetPixelCount; ++packetIndex) {
                if (!writePixel(decodedPixelCount + packetIndex, encodedOffset + packetIndex * bytesPerPixel)) {
                    return false;
                }
            }
            encodedOffset += packetPixelCount * bytesPerPixel;
        } else {
            if (bytes.size() - encodedOffset < bytesPerPixel) {
                return false;
            }
            for (std::size_t packetIndex = 0U; packetIndex < packetPixelCount; ++packetIndex) {
                if (!writePixel(decodedPixelCount + packetIndex, encodedOffset)) {
                    return false;
                }
            }
            encodedOffset += bytesPerPixel;
        }
        decodedPixelCount += packetPixelCount;
    }

    outImage.width = width;
    outImage.height = height;
    outImage.pixels = std::move(candidatePixels);
    return true;
}

} // namespace UVE::Asset
