// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/png_metadata_uve.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <new>
#include <utility>

#include <zlib.h>

namespace UVE::Asset {
namespace {

[[nodiscard]] std::uint32_t ReadU32BE(const std::vector<std::byte>& bytes, const std::size_t offset) noexcept {
    return (static_cast<std::uint32_t>(std::to_integer<unsigned int>(bytes[offset])) << 24U) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned int>(bytes[offset + 1U])) << 16U) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned int>(bytes[offset + 2U])) << 8U) |
           static_cast<std::uint32_t>(std::to_integer<unsigned int>(bytes[offset + 3U]));
}

[[nodiscard]] bool HasBytes(const std::vector<std::byte>& bytes, const std::size_t offset,
                            const std::initializer_list<unsigned int> expected) noexcept {
    if (offset > bytes.size() || expected.size() > bytes.size() - offset) {
        return false;
    }
    std::size_t index = 0U;
    for (const unsigned int value : expected) {
        if (std::to_integer<unsigned int>(bytes[offset + index]) != value) {
            return false;
        }
        ++index;
    }
    return true;
}

[[nodiscard]] bool IsValidDepthForColor(const std::uint8_t bitDepth, const std::uint8_t colorType) noexcept {
    switch (colorType) {
        case 0U:
            return bitDepth == 1U || bitDepth == 2U || bitDepth == 4U || bitDepth == 8U || bitDepth == 16U;
        case 2U:
        case 4U:
        case 6U:
            return bitDepth == 8U || bitDepth == 16U;
        case 3U:
            return bitDepth == 1U || bitDepth == 2U || bitDepth == 4U || bitDepth == 8U;
        default:
            return false;
    }
}

[[nodiscard]] bool UnfilterPngScanlineUVE(const PngFilterTypeUVE filter,
                                           const std::vector<std::byte>& filteredBytes,
                                           const std::vector<std::byte>& previousRow,
                                           const std::size_t bytesPerPixel,
                                           std::vector<std::byte>& outRow) {
    if (bytesPerPixel == 0U || filteredBytes.empty() || filteredBytes.size() > kMaximumPngRgba8ScanlineBytesUVE ||
        (!previousRow.empty() && previousRow.size() != filteredBytes.size()) ||
        ((filter == PngFilterTypeUVE::Up || filter == PngFilterTypeUVE::Average || filter == PngFilterTypeUVE::Paeth) &&
         previousRow.empty())) {
        return false;
    }
    try {
        std::vector<std::byte> reconstructed(filteredBytes.size());
        const auto byteAt = [](const std::vector<std::byte>& bytes, const std::size_t index) noexcept {
            return std::to_integer<unsigned int>(bytes[index]);
        };
        const auto paeth = [](const unsigned int left, const unsigned int up,
                              const unsigned int upperLeft) noexcept {
            const int prediction = static_cast<int>(left) + static_cast<int>(up) - static_cast<int>(upperLeft);
            const int leftDistance = std::abs(prediction - static_cast<int>(left));
            const int upDistance = std::abs(prediction - static_cast<int>(up));
            const int upperLeftDistance = std::abs(prediction - static_cast<int>(upperLeft));
            if (leftDistance <= upDistance && leftDistance <= upperLeftDistance) return left;
            if (upDistance <= upperLeftDistance) return up;
            return upperLeft;
        };
        for (std::size_t index = 0U; index < filteredBytes.size(); ++index) {
            const unsigned int left = index >= bytesPerPixel ? byteAt(reconstructed, index - bytesPerPixel) : 0U;
            const unsigned int up = previousRow.empty() ? 0U : byteAt(previousRow, index);
            const unsigned int upperLeft = previousRow.empty() || index < bytesPerPixel ?
                0U : byteAt(previousRow, index - bytesPerPixel);
            unsigned int predictor = 0U;
            switch (filter) {
                case PngFilterTypeUVE::None:
                    predictor = 0U;
                    break;
                case PngFilterTypeUVE::Sub:
                    predictor = left;
                    break;
                case PngFilterTypeUVE::Up:
                    predictor = up;
                    break;
                case PngFilterTypeUVE::Average:
                    predictor = (left + up) / 2U;
                    break;
                case PngFilterTypeUVE::Paeth:
                    predictor = paeth(left, up, upperLeft);
                    break;
                default:
                    return false;
            }
            reconstructed[index] = std::byte{static_cast<unsigned char>(
                (byteAt(filteredBytes, index) + predictor) & 0xFFU)};
        }
        outRow = std::move(reconstructed);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    }
}

} // namespace

std::optional<PngMetadataUVE> ParsePngMetadataUVE(const std::vector<std::byte>& bytes) {
    constexpr std::size_t kSignatureBytes = 8U;
    constexpr std::size_t kChunkHeaderBytes = 8U;
    constexpr std::size_t kIhdrPayloadBytes = 13U;
    if (!HasBytes(bytes, 0U, {137U, 80U, 78U, 71U, 13U, 10U, 26U, 10U}) ||
        bytes.size() < kSignatureBytes + kChunkHeaderBytes + kIhdrPayloadBytes) {
        return std::nullopt;
    }
    const std::size_t chunkOffset = kSignatureBytes;
    if (ReadU32BE(bytes, chunkOffset) != kIhdrPayloadBytes ||
        !HasBytes(bytes, chunkOffset + 4U, {'I', 'H', 'D', 'R'})) {
        return std::nullopt;
    }
    const std::size_t payloadOffset = chunkOffset + kChunkHeaderBytes;
    PngMetadataUVE metadata;
    metadata.width = ReadU32BE(bytes, payloadOffset);
    metadata.height = ReadU32BE(bytes, payloadOffset + 4U);
    metadata.bitDepth = std::to_integer<std::uint8_t>(bytes[payloadOffset + 8U]);
    metadata.colorType = std::to_integer<std::uint8_t>(bytes[payloadOffset + 9U]);
    metadata.compressionMethod = std::to_integer<std::uint8_t>(bytes[payloadOffset + 10U]);
    metadata.filterMethod = std::to_integer<std::uint8_t>(bytes[payloadOffset + 11U]);
    metadata.interlaceMethod = std::to_integer<std::uint8_t>(bytes[payloadOffset + 12U]);
    if (metadata.width == 0U || metadata.height == 0U || !IsValidDepthForColor(metadata.bitDepth, metadata.colorType) ||
        metadata.compressionMethod != 0U || metadata.filterMethod != 0U || metadata.interlaceMethod > 1U) {
        return std::nullopt;
    }
    metadata.hasAlpha = metadata.colorType == 4U || metadata.colorType == 6U;
    return metadata;
}

bool UnfilterPngRgba8ScanlineUVE(const PngFilterTypeUVE filter,
                                    const std::vector<std::byte>& filteredBytes,
                                    const std::vector<std::byte>& previousRow,
                                    std::vector<std::byte>& outRow) {
    return UnfilterPngScanlineUVE(filter, filteredBytes, previousRow, 4U, outRow);
}

bool ValidatePngRgba8PixelBudgetUVE(const PngMetadataUVE& metadata,
                                    const std::uint64_t maximumBytes) noexcept {
    const bool supportedPackedGray = metadata.bitDepth >= 1U && metadata.bitDepth <= 4U && metadata.colorType == 0U;
    const bool supportedPackedIndexed = metadata.bitDepth >= 1U && metadata.bitDepth <= 4U && metadata.colorType == 3U;
    const bool supportedEightBit = metadata.bitDepth == 8U &&
                                   (metadata.colorType == 0U || metadata.colorType == 2U || metadata.colorType == 3U ||
                                    metadata.colorType == 4U || metadata.colorType == 6U);
    const bool supportedSixteenBit = metadata.bitDepth == 16U &&
                                     (metadata.colorType == 0U || metadata.colorType == 2U || metadata.colorType == 4U ||
                                      metadata.colorType == 6U);
    if (metadata.width == 0U || metadata.height == 0U ||
        !(supportedPackedGray || supportedPackedIndexed || supportedEightBit || supportedSixteenBit) ||
        maximumBytes == 0U) {
        return false;
    }
    constexpr std::uint64_t kBytesPerRgba8Pixel = 4ULL;
    const std::uint64_t pixelCount = static_cast<std::uint64_t>(metadata.width) *
                                      static_cast<std::uint64_t>(metadata.height);
    if (pixelCount > maximumBytes / kBytesPerRgba8Pixel) {
        return false;
    }
    return pixelCount * kBytesPerRgba8Pixel <= maximumBytes;
}

bool DecodePngRgba8ImageUVE(const std::vector<std::byte>& bytes, PngRgba8ImageUVE& outImage) noexcept {
    try {
        const auto metadata = ParsePngMetadataUVE(bytes);
        if (!metadata.has_value()) {
            return false;
        }
        const bool supportedPackedGray = metadata->bitDepth >= 1U && metadata->bitDepth <= 4U && metadata->colorType == 0U;
        const bool supportedPackedIndexed = metadata->bitDepth >= 1U && metadata->bitDepth <= 4U && metadata->colorType == 3U;
        const bool supportedEightBit = metadata->bitDepth == 8U &&
                                       (metadata->colorType == 0U || metadata->colorType == 2U || metadata->colorType == 3U ||
                                        metadata->colorType == 4U || metadata->colorType == 6U);
        const bool supportedSixteenBit = metadata->bitDepth == 16U &&
                                         (metadata->colorType == 0U || metadata->colorType == 2U ||
                                          metadata->colorType == 4U || metadata->colorType == 6U);
        const bool supportedAdam7 = metadata->interlaceMethod == 1U &&
                                    (((metadata->bitDepth >= 1U && metadata->bitDepth <= 4U) &&
                                      (metadata->colorType == 0U || metadata->colorType == 3U)) ||
                                     (metadata->bitDepth == 8U &&
                                      (metadata->colorType == 0U || metadata->colorType == 2U || metadata->colorType == 3U ||
                                       metadata->colorType == 4U || metadata->colorType == 6U)) ||
                                     (metadata->bitDepth == 16U &&
                                      (metadata->colorType == 0U || metadata->colorType == 2U ||
                                       metadata->colorType == 4U || metadata->colorType == 6U)));
        if (!(supportedPackedGray || supportedPackedIndexed || supportedEightBit || supportedSixteenBit) ||
            (metadata->interlaceMethod != 0U && !supportedAdam7) || !ValidatePngRgba8PixelBudgetUVE(*metadata)) {
            return false;
        }
        constexpr std::size_t kSignatureBytes = 8U;
        constexpr std::size_t kChunkOverheadBytes = 12U;
        if (bytes.size() < kSignatureBytes) {
            return false;
        }
        std::vector<std::byte> compressed;
        std::vector<std::byte> paletteRgb;
        std::vector<std::byte> paletteAlpha;
        bool foundIdat = false;
        bool foundIend = false;
        bool hasTransparentGray = false;
        bool hasTransparentRgb = false;
        std::uint16_t transparentGray = 0U;
        std::uint16_t transparentRed = 0U;
        std::uint16_t transparentGreen = 0U;
        std::uint16_t transparentBlue = 0U;
        const auto readU16BE = [](const std::vector<std::byte>& source, const std::size_t valueOffset) noexcept {
            return static_cast<std::uint16_t>((std::to_integer<std::uint16_t>(source[valueOffset]) << 8U) |
                                              std::to_integer<std::uint16_t>(source[valueOffset + 1U]));
        };
        std::size_t offset = kSignatureBytes;
        while (offset <= bytes.size() && bytes.size() - offset >= kChunkOverheadBytes) {
            const std::uint32_t chunkLength = ReadU32BE(bytes, offset);
            const std::size_t payloadLength = static_cast<std::size_t>(chunkLength);
            if (payloadLength > bytes.size() - offset - kChunkOverheadBytes) {
                return false;
            }
            const std::size_t typeOffset = offset + 4U;
            const std::size_t payloadOffset = offset + 8U;
            const std::size_t crcOffset = payloadOffset + payloadLength;
            uLong crc = crc32(0L, Z_NULL, 0U);
            crc = crc32(crc, reinterpret_cast<const Bytef*>(bytes.data() + typeOffset), 4U);
            if (payloadLength > 0U) {
                crc = crc32(crc, reinterpret_cast<const Bytef*>(bytes.data() + payloadOffset),
                           static_cast<uInt>(payloadLength));
            }
            if (crc != static_cast<uLong>(ReadU32BE(bytes, crcOffset))) {
                return false;
            }
            if (HasBytes(bytes, typeOffset, {'P', 'L', 'T', 'E'})) {
                if (metadata->colorType != 3U || foundIdat || !paletteRgb.empty() || payloadLength == 0U ||
                    payloadLength > 768U || payloadLength % 3U != 0U) {
                    return false;
                }
                paletteRgb.insert(paletteRgb.end(), bytes.begin() + static_cast<std::ptrdiff_t>(payloadOffset),
                                  bytes.begin() + static_cast<std::ptrdiff_t>(payloadOffset + payloadLength));
                paletteAlpha.assign(payloadLength / 3U, std::byte{0xFF});
            } else if (HasBytes(bytes, typeOffset, {'t', 'R', 'N', 'S'})) {
                if (foundIdat) return false;
                if (metadata->colorType == 3U) {
                    if (paletteAlpha.empty() || payloadLength > paletteAlpha.size()) return false;
                    std::copy(bytes.begin() + static_cast<std::ptrdiff_t>(payloadOffset),
                              bytes.begin() + static_cast<std::ptrdiff_t>(payloadOffset + payloadLength), paletteAlpha.begin());
                } else if (metadata->colorType == 0U) {
                    const bool supportedGrayTransparencyDepth = metadata->bitDepth == 1U || metadata->bitDepth == 2U ||
                        metadata->bitDepth == 4U || metadata->bitDepth == 8U || metadata->bitDepth == 16U;
                    if (!supportedGrayTransparencyDepth || payloadLength != 2U || hasTransparentGray) {
                        return false;
                    }
                    transparentGray = readU16BE(bytes, payloadOffset);
                    if (metadata->bitDepth < 16U &&
                        static_cast<std::uint32_t>(transparentGray) >= (1U << metadata->bitDepth)) {
                        return false;
                    }
                    hasTransparentGray = true;
                } else if (metadata->colorType == 2U) {
                    if ((metadata->bitDepth != 8U && metadata->bitDepth != 16U) || payloadLength != 6U || hasTransparentRgb) {
                        return false;
                    }
                    transparentRed = readU16BE(bytes, payloadOffset);
                    transparentGreen = readU16BE(bytes, payloadOffset + 2U);
                    transparentBlue = readU16BE(bytes, payloadOffset + 4U);
                    hasTransparentRgb = true;
                } else {
                    return false;
                }
            } else if (HasBytes(bytes, typeOffset, {'I', 'D', 'A', 'T'})) {
                if (compressed.size() > kMaximumPngDecodedPixelBytesUVE -
                                       std::min<std::size_t>(compressed.size(), kMaximumPngDecodedPixelBytesUVE) ||
                    payloadLength > kMaximumPngDecodedPixelBytesUVE - compressed.size()) {
                    return false;
                }
                compressed.insert(compressed.end(), bytes.begin() + static_cast<std::ptrdiff_t>(payloadOffset),
                                  bytes.begin() + static_cast<std::ptrdiff_t>(payloadOffset + payloadLength));
                foundIdat = true;
            } else if (HasBytes(bytes, typeOffset, {'I', 'E', 'N', 'D'})) {
                foundIend = true;
                break;
            }
            offset = crcOffset + 4U;
        }
        if (!foundIdat || !foundIend || compressed.empty() || (metadata->colorType == 3U && paletteRgb.empty())) {
            return false;
        }
        const std::size_t sourceBytesPerPixel = metadata->bitDepth < 8U ? 1U :
            (metadata->bitDepth == 16U ?
                 (metadata->colorType == 0U ? 2U : (metadata->colorType == 2U ? 6U :
                  (metadata->colorType == 4U ? 4U : 8U))) :
                 (metadata->colorType == 0U || metadata->colorType == 3U ? 1U :
                  (metadata->colorType == 2U ? 3U : (metadata->colorType == 4U ? 2U : 4U))));
        const std::uint64_t outputRowBytes64 = static_cast<std::uint64_t>(metadata->width) * 4ULL;
        if (outputRowBytes64 > std::numeric_limits<std::size_t>::max()) {
            return false;
        }
        const std::size_t outputRowBytes = static_cast<std::size_t>(outputRowBytes64);
        const std::size_t pixelBytes = outputRowBytes * static_cast<std::size_t>(metadata->height);
        std::vector<std::byte> inflated;
        std::vector<std::byte> pixels(pixelBytes);
        const auto checkedAdd = [](const std::uint64_t left, const std::uint64_t right,
                                   std::uint64_t& result) noexcept {
            if (right > std::numeric_limits<std::uint64_t>::max() - left) return false;
            result = left + right;
            return true;
        };
        const auto checkedMultiply = [](const std::uint64_t left, const std::uint64_t right,
                                        std::uint64_t& result) noexcept {
            if (left != 0U && right > std::numeric_limits<std::uint64_t>::max() / left) return false;
            result = left * right;
            return true;
        };
        const auto sourceRowBytesForWidth = [metadata, sourceBytesPerPixel](const std::size_t width) noexcept {
            if (metadata->bitDepth < 8U) {
                return (width * static_cast<std::size_t>(metadata->bitDepth) + 7U) / 8U;
            }
            return width * sourceBytesPerPixel;
        };
        const auto readPackedSample = [metadata](const std::vector<std::byte>& decodedRow,
                                                   const std::size_t x) noexcept {
            const std::size_t samplesPerByte = 8U / metadata->bitDepth;
            const std::size_t packedOffset = x / samplesPerByte;
            const std::size_t sampleShift = 8U - metadata->bitDepth -
                                            (x % samplesPerByte) * metadata->bitDepth;
            const std::uint8_t packed = std::to_integer<std::uint8_t>(decodedRow[packedOffset]);
            const std::uint8_t sampleMask = static_cast<std::uint8_t>((1U << metadata->bitDepth) - 1U);
            return static_cast<std::uint8_t>((packed >> sampleShift) & sampleMask);
        };
        const auto passWidth = [metadata](const std::size_t start, const std::size_t step) noexcept {
            if (metadata->width <= start) return std::size_t{0U};
            return (static_cast<std::size_t>(metadata->width) - start + step - 1U) / step;
        };
        const auto passHeight = [metadata](const std::size_t start, const std::size_t step) noexcept {
            if (metadata->height <= start) return std::size_t{0U};
            return (static_cast<std::size_t>(metadata->height) - start + step - 1U) / step;
        };
        constexpr std::size_t kAdam7StartX[7] = {0U, 4U, 0U, 2U, 0U, 1U, 0U};
        constexpr std::size_t kAdam7StartY[7] = {0U, 0U, 4U, 0U, 2U, 0U, 1U};
        constexpr std::size_t kAdam7StepX[7] = {8U, 8U, 4U, 4U, 2U, 2U, 1U};
        constexpr std::size_t kAdam7StepY[7] = {8U, 8U, 4U, 4U, 2U, 2U, 2U};
        std::uint64_t inflatedBytes64 = 0U;
        const std::size_t passCount = metadata->interlaceMethod == 1U ? 7U : 1U;
        for (std::size_t pass = 0U; pass < passCount; ++pass) {
            const std::size_t width = metadata->interlaceMethod == 1U ? passWidth(kAdam7StartX[pass], kAdam7StepX[pass]) : metadata->width;
            const std::size_t height = metadata->interlaceMethod == 1U ? passHeight(kAdam7StartY[pass], kAdam7StepY[pass]) : metadata->height;
            if (width == 0U || height == 0U) continue;
            std::uint64_t rowBytes = static_cast<std::uint64_t>(sourceRowBytesForWidth(width));
            std::uint64_t passBytes = 0U;
            if (rowBytes > kMaximumPngRgba8ScanlineBytesUVE ||
                !checkedAdd(rowBytes, 1U, rowBytes) ||
                !checkedMultiply(rowBytes, static_cast<std::uint64_t>(height), passBytes) ||
                !checkedAdd(inflatedBytes64, passBytes, inflatedBytes64) ||
                inflatedBytes64 > kMaximumPngDecodedPixelBytesUVE) {
                return false;
            }
        }
        if (inflatedBytes64 > std::numeric_limits<std::size_t>::max()) {
            return false;
        }
        inflated.resize(static_cast<std::size_t>(inflatedBytes64));
        uLongf destinationLength = static_cast<uLongf>(inflated.size());
        if (uncompress(reinterpret_cast<Bytef*>(inflated.data()), &destinationLength,
                       reinterpret_cast<const Bytef*>(compressed.data()), static_cast<uLong>(compressed.size())) != Z_OK ||
            destinationLength != inflated.size()) {
            return false;
        }
        std::size_t inflatedOffset = 0U;
        const auto decodePass = [&](const std::size_t passWidthValue, const std::size_t passHeightValue,
                                    const std::size_t startX, const std::size_t startY,
                                    const std::size_t stepX, const std::size_t stepY) noexcept {
            const std::size_t sourceRowBytes = sourceRowBytesForWidth(passWidthValue);
            std::vector<std::byte> previousRow;
            for (std::size_t row = 0U; row < passHeightValue; ++row) {
                const std::size_t rowOffset = inflatedOffset + row * (sourceRowBytes + 1U);
                const auto filter = static_cast<PngFilterTypeUVE>(std::to_integer<std::uint8_t>(inflated[rowOffset]));
                const std::vector<std::byte> filtered(inflated.begin() + static_cast<std::ptrdiff_t>(rowOffset + 1U),
                                                       inflated.begin() + static_cast<std::ptrdiff_t>(rowOffset + 1U + sourceRowBytes));
                std::vector<std::byte> decodedRow;
                if (!UnfilterPngScanlineUVE(filter, filtered, previousRow, sourceBytesPerPixel, decodedRow)) return false;
                for (std::size_t x = 0U; x < passWidthValue; ++x) {
                    const std::size_t sourceOffset = x * sourceBytesPerPixel;
                    const std::size_t outputX = startX + x * stepX;
                    const std::size_t outputY = startY + row * stepY;
                    const std::size_t outputOffset = outputY * outputRowBytes + outputX * 4U;
                    if (metadata->interlaceMethod == 1U && metadata->bitDepth == 16U && metadata->colorType == 0U) {
                        const std::byte gray = decodedRow[sourceOffset];
                        const std::uint16_t sample = readU16BE(decodedRow, sourceOffset);
                        pixels[outputOffset] = gray;
                        pixels[outputOffset + 1U] = gray;
                        pixels[outputOffset + 2U] = gray;
                        pixels[outputOffset + 3U] = hasTransparentGray && sample == transparentGray ? std::byte{0} : std::byte{0xFF};
                    } else if (metadata->interlaceMethod == 1U && metadata->bitDepth == 16U && metadata->colorType == 2U) {
                        const std::uint16_t red = readU16BE(decodedRow, sourceOffset);
                        const std::uint16_t green = readU16BE(decodedRow, sourceOffset + 2U);
                        const std::uint16_t blue = readU16BE(decodedRow, sourceOffset + 4U);
                        pixels[outputOffset] = decodedRow[sourceOffset];
                        pixels[outputOffset + 1U] = decodedRow[sourceOffset + 2U];
                        pixels[outputOffset + 2U] = decodedRow[sourceOffset + 4U];
                        pixels[outputOffset + 3U] = hasTransparentRgb && red == transparentRed && green == transparentGreen &&
                            blue == transparentBlue ? std::byte{0} : std::byte{0xFF};
                    } else if (metadata->interlaceMethod == 1U && metadata->bitDepth == 16U && metadata->colorType == 4U) {
                        const std::byte gray = decodedRow[sourceOffset];
                        pixels[outputOffset] = gray;
                        pixels[outputOffset + 1U] = gray;
                        pixels[outputOffset + 2U] = gray;
                        pixels[outputOffset + 3U] = decodedRow[sourceOffset + 2U];
                    } else if (metadata->interlaceMethod == 1U && metadata->bitDepth == 16U && metadata->colorType == 6U) {
                        pixels[outputOffset] = decodedRow[sourceOffset];
                        pixels[outputOffset + 1U] = decodedRow[sourceOffset + 2U];
                        pixels[outputOffset + 2U] = decodedRow[sourceOffset + 4U];
                        pixels[outputOffset + 3U] = decodedRow[sourceOffset + 6U];
                    } else if (metadata->interlaceMethod == 1U && metadata->bitDepth < 8U && metadata->colorType == 0U) {
                        const std::uint8_t sample = readPackedSample(decodedRow, x);
                        const std::uint8_t sampleMask = static_cast<std::uint8_t>((1U << metadata->bitDepth) - 1U);
                        const std::byte gray = std::byte{static_cast<unsigned char>(
                            (static_cast<unsigned int>(sample) * 255U) / sampleMask)};
                        pixels[outputOffset] = gray;
                        pixels[outputOffset + 1U] = gray;
                        pixels[outputOffset + 2U] = gray;
                        pixels[outputOffset + 3U] = hasTransparentGray && sample == transparentGray ? std::byte{0} : std::byte{0xFF};
                    } else if (metadata->interlaceMethod == 1U && metadata->bitDepth < 8U && metadata->colorType == 3U) {
                        const std::size_t paletteIndex = readPackedSample(decodedRow, x);
                        if (paletteIndex >= paletteAlpha.size()) return false;
                        const std::size_t paletteOffset = paletteIndex * 3U;
                        pixels[outputOffset] = paletteRgb[paletteOffset];
                        pixels[outputOffset + 1U] = paletteRgb[paletteOffset + 1U];
                        pixels[outputOffset + 2U] = paletteRgb[paletteOffset + 2U];
                        pixels[outputOffset + 3U] = paletteAlpha[paletteIndex];
                    } else if (metadata->interlaceMethod == 1U && metadata->colorType == 3U) {
                        const std::size_t paletteIndex = std::to_integer<std::uint8_t>(decodedRow[sourceOffset]);
                        if (paletteIndex >= paletteAlpha.size()) return false;
                        const std::size_t paletteOffset = paletteIndex * 3U;
                        pixels[outputOffset] = paletteRgb[paletteOffset];
                        pixels[outputOffset + 1U] = paletteRgb[paletteOffset + 1U];
                        pixels[outputOffset + 2U] = paletteRgb[paletteOffset + 2U];
                        pixels[outputOffset + 3U] = paletteAlpha[paletteIndex];
                    } else if (metadata->interlaceMethod == 1U && metadata->colorType == 0U) {
                        const std::byte gray = decodedRow[sourceOffset];
                        pixels[outputOffset] = gray;
                        pixels[outputOffset + 1U] = gray;
                        pixels[outputOffset + 2U] = gray;
                        pixels[outputOffset + 3U] = hasTransparentGray &&
                            std::to_integer<std::uint16_t>(gray) == transparentGray ? std::byte{0} : std::byte{0xFF};
                    } else if (metadata->interlaceMethod == 1U && metadata->colorType == 2U) {
                        const std::byte red = decodedRow[sourceOffset];
                        const std::byte green = decodedRow[sourceOffset + 1U];
                        const std::byte blue = decodedRow[sourceOffset + 2U];
                        pixels[outputOffset] = red;
                        pixels[outputOffset + 1U] = green;
                        pixels[outputOffset + 2U] = blue;
                        pixels[outputOffset + 3U] = hasTransparentRgb &&
                            std::to_integer<std::uint16_t>(red) == transparentRed &&
                            std::to_integer<std::uint16_t>(green) == transparentGreen &&
                            std::to_integer<std::uint16_t>(blue) == transparentBlue ? std::byte{0} : std::byte{0xFF};
                    } else if (metadata->interlaceMethod == 1U && metadata->colorType == 4U) {
                        const std::byte gray = decodedRow[sourceOffset];
                        pixels[outputOffset] = gray;
                        pixels[outputOffset + 1U] = gray;
                        pixels[outputOffset + 2U] = gray;
                        pixels[outputOffset + 3U] = decodedRow[sourceOffset + 1U];
                    } else if (metadata->interlaceMethod == 1U) {
                        pixels[outputOffset] = decodedRow[sourceOffset];
                        pixels[outputOffset + 1U] = decodedRow[sourceOffset + 1U];
                        pixels[outputOffset + 2U] = decodedRow[sourceOffset + 2U];
                        pixels[outputOffset + 3U] = decodedRow[sourceOffset + 3U];
                    } else if (metadata->bitDepth == 16U && metadata->colorType == 0U) {
                        const std::byte gray = decodedRow[sourceOffset];
                        const std::uint16_t sample = static_cast<std::uint16_t>(
                            (std::to_integer<std::uint16_t>(decodedRow[sourceOffset]) << 8U) |
                            std::to_integer<std::uint16_t>(decodedRow[sourceOffset + 1U]));
                        pixels[outputOffset] = gray;
                        pixels[outputOffset + 1U] = gray;
                        pixels[outputOffset + 2U] = gray;
                        pixels[outputOffset + 3U] = hasTransparentGray && sample == transparentGray ? std::byte{0} : std::byte{0xFF};
                    } else if (metadata->bitDepth == 16U && metadata->colorType == 2U) {
                        const std::uint16_t red = static_cast<std::uint16_t>(
                            (std::to_integer<std::uint16_t>(decodedRow[sourceOffset]) << 8U) |
                            std::to_integer<std::uint16_t>(decodedRow[sourceOffset + 1U]));
                        const std::uint16_t green = static_cast<std::uint16_t>(
                            (std::to_integer<std::uint16_t>(decodedRow[sourceOffset + 2U]) << 8U) |
                            std::to_integer<std::uint16_t>(decodedRow[sourceOffset + 3U]));
                        const std::uint16_t blue = static_cast<std::uint16_t>(
                            (std::to_integer<std::uint16_t>(decodedRow[sourceOffset + 4U]) << 8U) |
                            std::to_integer<std::uint16_t>(decodedRow[sourceOffset + 5U]));
                        pixels[outputOffset] = decodedRow[sourceOffset];
                        pixels[outputOffset + 1U] = decodedRow[sourceOffset + 2U];
                        pixels[outputOffset + 2U] = decodedRow[sourceOffset + 4U];
                        pixels[outputOffset + 3U] = hasTransparentRgb && red == transparentRed && green == transparentGreen &&
                            blue == transparentBlue ? std::byte{0} : std::byte{0xFF};
                    } else if (metadata->bitDepth == 16U && metadata->colorType == 4U) {
                        const std::byte gray = decodedRow[sourceOffset];
                        pixels[outputOffset] = gray;
                        pixels[outputOffset + 1U] = gray;
                        pixels[outputOffset + 2U] = gray;
                        pixels[outputOffset + 3U] = decodedRow[sourceOffset + 2U];
                    } else if (metadata->bitDepth == 16U) {
                        pixels[outputOffset] = decodedRow[sourceOffset];
                        pixels[outputOffset + 1U] = decodedRow[sourceOffset + 2U];
                        pixels[outputOffset + 2U] = decodedRow[sourceOffset + 4U];
                        pixels[outputOffset + 3U] = decodedRow[sourceOffset + 6U];
                    } else if (metadata->bitDepth < 8U && metadata->colorType == 0U) {
                        const std::uint8_t sample = readPackedSample(decodedRow, x);
                        const std::uint8_t sampleMask = static_cast<std::uint8_t>((1U << metadata->bitDepth) - 1U);
                        const std::byte gray = std::byte{static_cast<unsigned char>(
                            (static_cast<unsigned int>(sample) * 255U) / sampleMask)};
                        pixels[outputOffset] = gray;
                        pixels[outputOffset + 1U] = gray;
                        pixels[outputOffset + 2U] = gray;
                        pixels[outputOffset + 3U] = hasTransparentGray && sample == transparentGray ? std::byte{0} : std::byte{0xFF};
                    } else if (metadata->colorType == 3U) {
                        const std::size_t paletteIndex = metadata->bitDepth < 8U
                            ? readPackedSample(decodedRow, x)
                            : std::to_integer<std::uint8_t>(decodedRow[sourceOffset]);
                        if (paletteIndex >= paletteAlpha.size()) return false;
                        const std::size_t paletteOffset = paletteIndex * 3U;
                        pixels[outputOffset] = paletteRgb[paletteOffset];
                        pixels[outputOffset + 1U] = paletteRgb[paletteOffset + 1U];
                        pixels[outputOffset + 2U] = paletteRgb[paletteOffset + 2U];
                        pixels[outputOffset + 3U] = paletteAlpha[paletteIndex];
                    } else if (metadata->colorType == 4U) {
                        const std::byte gray = decodedRow[sourceOffset];
                        pixels[outputOffset] = gray;
                        pixels[outputOffset + 1U] = gray;
                        pixels[outputOffset + 2U] = gray;
                        pixels[outputOffset + 3U] = decodedRow[sourceOffset + 1U];
                    } else {
                        const std::byte red = decodedRow[sourceOffset];
                        const std::byte green = sourceBytesPerPixel == 1U ? red : decodedRow[sourceOffset + 1U];
                        const std::byte blue = sourceBytesPerPixel == 1U ? red : decodedRow[sourceOffset + 2U];
                        pixels[outputOffset] = red;
                        pixels[outputOffset + 1U] = green;
                        pixels[outputOffset + 2U] = blue;
                        pixels[outputOffset + 3U] = metadata->colorType == 6U ? decodedRow[sourceOffset + 3U] :
                            (metadata->colorType == 0U ?
                                (hasTransparentGray && std::to_integer<std::uint16_t>(red) == transparentGray ? std::byte{0} : std::byte{0xFF}) :
                                (hasTransparentRgb && std::to_integer<std::uint16_t>(red) == transparentRed &&
                                 std::to_integer<std::uint16_t>(green) == transparentGreen &&
                                 std::to_integer<std::uint16_t>(blue) == transparentBlue ? std::byte{0} : std::byte{0xFF}));
                    }
                }
                previousRow = std::move(decodedRow);
            }
            inflatedOffset += passHeightValue * (sourceRowBytes + 1U);
            return true;
        };
        for (std::size_t pass = 0U; pass < passCount; ++pass) {
            const std::size_t width = metadata->interlaceMethod == 1U ? passWidth(kAdam7StartX[pass], kAdam7StepX[pass]) : metadata->width;
            const std::size_t height = metadata->interlaceMethod == 1U ? passHeight(kAdam7StartY[pass], kAdam7StepY[pass]) : metadata->height;
            if (width == 0U || height == 0U) continue;
            const std::size_t startX = metadata->interlaceMethod == 1U ? kAdam7StartX[pass] : 0U;
            const std::size_t startY = metadata->interlaceMethod == 1U ? kAdam7StartY[pass] : 0U;
            const std::size_t stepX = metadata->interlaceMethod == 1U ? kAdam7StepX[pass] : 1U;
            const std::size_t stepY = metadata->interlaceMethod == 1U ? kAdam7StepY[pass] : 1U;
            if (!decodePass(width, height, startX, startY, stepX, stepY)) return false;
        }
        PngRgba8ImageUVE image;
        image.width = metadata->width;
        image.height = metadata->height;
        image.pixels = std::move(pixels);
        outImage = std::move(image);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    }
}

} // namespace UVE::Asset
