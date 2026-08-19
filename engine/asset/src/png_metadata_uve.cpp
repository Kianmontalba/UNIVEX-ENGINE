// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/png_metadata_uve.h"

#include <cstddef>
#include <initializer_list>

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

} // namespace UVE::Asset
