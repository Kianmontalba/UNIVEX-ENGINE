// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/png_metadata_uve.h"

#include <cstddef>
#include <cstdlib>
#include <initializer_list>
#include <new>
#include <utility>

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

} // namespace UVE::Asset
