// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/asset/jpeg_metadata_uve.h"
#include <cstddef>
namespace UVE::Asset {
namespace {
[[nodiscard]] bool IsSofMarkerUVE(const std::uint8_t marker) noexcept {
    return (marker >= 0xC0U && marker <= 0xC3U) || (marker >= 0xC5U && marker <= 0xC7U) ||
           (marker >= 0xC9U && marker <= 0xCBU) || (marker >= 0xCDU && marker <= 0xCFU);
}
[[nodiscard]] bool IsProgressiveMarkerUVE(const std::uint8_t marker) noexcept {
    return marker == 0xC2U || marker == 0xC6U || marker == 0xCAU || marker == 0xCEU;
}
[[nodiscard]] std::uint16_t ReadU16BEUVE(const std::vector<std::byte>& bytes, const std::size_t offset) noexcept {
    return static_cast<std::uint16_t>((std::to_integer<std::uint16_t>(bytes[offset]) << 8U) |
                                      std::to_integer<std::uint16_t>(bytes[offset + 1U]));
}
} // namespace

bool ValidateJpegRgba8PixelBudgetUVE(const JpegMetadataUVE& metadata,
                                     const std::uint64_t maximumBytes) noexcept {
    if (metadata.width == 0U || metadata.height == 0U || metadata.precision != 8U ||
        metadata.componentCount == 0U || metadata.componentCount > 4U || maximumBytes == 0U) {
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

std::optional<JpegMetadataUVE> ParseJpegMetadataUVE(const std::vector<std::byte>& bytes) {
    if (bytes.size() < 2U || bytes[0] != std::byte{0xFF} || bytes[1] != std::byte{0xD8}) return std::nullopt;
    std::size_t offset = 2U;
    while (offset < bytes.size()) {
        if (bytes[offset++] != std::byte{0xFF}) return std::nullopt;
        while (offset < bytes.size() && bytes[offset] == std::byte{0xFF}) ++offset;
        if (offset >= bytes.size()) return std::nullopt;
        const std::uint8_t marker = std::to_integer<std::uint8_t>(bytes[offset++]);
        if (marker == 0xD9U || marker == 0xDAU) return std::nullopt;
        if (marker >= 0xD0U && marker <= 0xD7U) continue;
        if (marker == 0x01U) continue;
        if (offset + 2U > bytes.size()) return std::nullopt;
        const std::uint16_t segmentLength = ReadU16BEUVE(bytes, offset);
        if (segmentLength < 2U || offset + segmentLength > bytes.size()) return std::nullopt;
        if (IsSofMarkerUVE(marker)) {
            if (segmentLength < 8U) return std::nullopt;
            const std::size_t payload = offset + 2U;
            const std::uint8_t precision = std::to_integer<std::uint8_t>(bytes[payload]);
            const std::uint16_t height = ReadU16BEUVE(bytes, payload + 1U);
            const std::uint16_t width = ReadU16BEUVE(bytes, payload + 3U);
            const std::uint8_t components = std::to_integer<std::uint8_t>(bytes[payload + 5U]);
            if (width == 0U || height == 0U || components == 0U || components > 4U ||
                segmentLength != static_cast<std::uint16_t>(8U + 3U * components)) return std::nullopt;
            return JpegMetadataUVE{width, height, precision, components, IsProgressiveMarkerUVE(marker)};
        }
        offset += segmentLength;
    }
    return std::nullopt;
}
} // namespace UVE::Asset
