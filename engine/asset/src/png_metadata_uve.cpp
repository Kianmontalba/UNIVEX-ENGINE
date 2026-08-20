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
    if (filteredBytes.empty() || filteredBytes.size() > kMaximumPngRgba8ScanlineBytesUVE ||
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
            const unsigned int left = index >= 4U ? byteAt(reconstructed, index - 4U) : 0U;
            const unsigned int up = previousRow.empty() ? 0U : byteAt(previousRow, index);
            const unsigned int upperLeft = previousRow.empty() || index < 4U ? 0U : byteAt(previousRow, index - 4U);
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

bool ValidatePngRgba8PixelBudgetUVE(const PngMetadataUVE& metadata,
                                    const std::uint64_t maximumBytes) noexcept {
    if (metadata.width == 0U || metadata.height == 0U || metadata.bitDepth != 8U ||
        metadata.colorType != 6U || maximumBytes == 0U) {
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
        if (!metadata.has_value() || metadata->bitDepth != 8U || metadata->colorType != 6U ||
            metadata->interlaceMethod != 0U || !ValidatePngRgba8PixelBudgetUVE(*metadata)) {
            return false;
        }
        constexpr std::size_t kSignatureBytes = 8U;
        constexpr std::size_t kChunkOverheadBytes = 12U;
        if (bytes.size() < kSignatureBytes) {
            return false;
        }
        std::vector<std::byte> compressed;
        bool foundIdat = false;
        bool foundIend = false;
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
            if (HasBytes(bytes, typeOffset, {'I', 'D', 'A', 'T'})) {
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
        if (!foundIdat || !foundIend || compressed.empty()) {
            return false;
        }
        const std::uint64_t rowBytes64 = static_cast<std::uint64_t>(metadata->width) * 4ULL;
        const std::uint64_t inflatedBytes64 = (rowBytes64 + 1ULL) * static_cast<std::uint64_t>(metadata->height);
        if (rowBytes64 > std::numeric_limits<std::size_t>::max() ||
            inflatedBytes64 > std::numeric_limits<std::size_t>::max()) {
            return false;
        }
        const std::size_t rowBytes = static_cast<std::size_t>(rowBytes64);
        const std::size_t inflatedBytes = static_cast<std::size_t>(inflatedBytes64);
        std::vector<std::byte> inflated(inflatedBytes);
        uLongf destinationLength = static_cast<uLongf>(inflated.size());
        if (uncompress(reinterpret_cast<Bytef*>(inflated.data()), &destinationLength,
                       reinterpret_cast<const Bytef*>(compressed.data()), static_cast<uLong>(compressed.size())) != Z_OK ||
            destinationLength != inflated.size()) {
            return false;
        }
        std::vector<std::byte> pixels(rowBytes * static_cast<std::size_t>(metadata->height));
        std::vector<std::byte> previousRow;
        for (std::size_t row = 0U; row < metadata->height; ++row) {
            const std::size_t rowOffset = row * (rowBytes + 1U);
            const auto filter = static_cast<PngFilterTypeUVE>(std::to_integer<std::uint8_t>(inflated[rowOffset]));
            const std::vector<std::byte> filtered(inflated.begin() + static_cast<std::ptrdiff_t>(rowOffset + 1U),
                                                   inflated.begin() + static_cast<std::ptrdiff_t>(rowOffset + 1U + rowBytes));
            std::vector<std::byte> decodedRow;
            if (!UnfilterPngRgba8ScanlineUVE(filter, filtered, previousRow, decodedRow)) {
                return false;
            }
            std::memcpy(pixels.data() + row * rowBytes, decodedRow.data(), rowBytes);
            previousRow = std::move(decodedRow);
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
