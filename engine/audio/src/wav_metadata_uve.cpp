// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/audio/wav_metadata_uve.h"

#include <cmath>
#include <cstring>
#include <limits>

namespace UVE::Audio {
namespace {

[[nodiscard]] std::uint16_t ReadU16LE(const std::vector<std::byte>& bytes, const std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(std::to_integer<unsigned int>(bytes[offset]) |
                                      (std::to_integer<unsigned int>(bytes[offset + 1U]) << 8U));
}

[[nodiscard]] std::uint32_t ReadU32LE(const std::vector<std::byte>& bytes, const std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(bytes[offset]) |
                                      (std::to_integer<std::uint32_t>(bytes[offset + 1U]) << 8U) |
                                      (std::to_integer<std::uint32_t>(bytes[offset + 2U]) << 16U) |
                                      (std::to_integer<std::uint32_t>(bytes[offset + 3U]) << 24U));
}

[[nodiscard]] bool HasTag(const std::vector<std::byte>& bytes, const std::size_t offset,
                          const char a, const char b, const char c, const char d) noexcept {
    return bytes[offset] == std::byte{static_cast<unsigned char>(a)} &&
           bytes[offset + 1U] == std::byte{static_cast<unsigned char>(b)} &&
           bytes[offset + 2U] == std::byte{static_cast<unsigned char>(c)} &&
           bytes[offset + 3U] == std::byte{static_cast<unsigned char>(d)};
}

} // namespace

std::optional<WavMetadataUVE> ParseWavMetadataUVE(const std::vector<std::byte>& bytes) {
    constexpr std::size_t kRiffHeaderBytes = 12U;
    constexpr std::size_t kChunkHeaderBytes = 8U;
    if (bytes.size() < kRiffHeaderBytes || !HasTag(bytes, 0U, 'R', 'I', 'F', 'F') ||
        !HasTag(bytes, 8U, 'W', 'A', 'V', 'E')) {
        return std::nullopt;
    }
    const std::uint32_t riffPayloadBytes = ReadU32LE(bytes, 4U);
    if (riffPayloadBytes < 4U || static_cast<std::uint64_t>(riffPayloadBytes) + 8U > bytes.size()) {
        return std::nullopt;
    }
    const std::size_t riffEnd = static_cast<std::size_t>(riffPayloadBytes) + 8U;

    std::optional<WavMetadataUVE> metadata;
    std::uint32_t dataBytes = 0U;
    std::size_t offset = kRiffHeaderBytes;
    while (offset + kChunkHeaderBytes <= riffEnd) {
        const std::uint32_t chunkBytes = ReadU32LE(bytes, offset + 4U);
        const std::size_t payloadOffset = offset + kChunkHeaderBytes;
        const std::uint64_t payloadEnd = static_cast<std::uint64_t>(payloadOffset) + chunkBytes;
        if (payloadEnd > riffEnd) {
            return std::nullopt;
        }
        if (HasTag(bytes, offset, 'f', 'm', 't', ' ') && chunkBytes >= 16U && !metadata.has_value()) {
            WavMetadataUVE parsed;
            parsed.audioFormat = ReadU16LE(bytes, payloadOffset);
            parsed.channels = ReadU16LE(bytes, payloadOffset + 2U);
            parsed.sampleRate = ReadU32LE(bytes, payloadOffset + 4U);
            parsed.byteRate = ReadU32LE(bytes, payloadOffset + 8U);
            parsed.blockAlign = ReadU16LE(bytes, payloadOffset + 12U);
            parsed.bitsPerSample = ReadU16LE(bytes, payloadOffset + 14U);
            const std::uint32_t expectedBlockAlign = static_cast<std::uint32_t>(parsed.channels) *
                                                       (static_cast<std::uint32_t>(parsed.bitsPerSample) / 8U);
            const std::uint64_t expectedByteRate = static_cast<std::uint64_t>(parsed.sampleRate) *
                                                    parsed.blockAlign;
            if (parsed.audioFormat != 1U || parsed.channels == 0U || parsed.sampleRate == 0U ||
                parsed.bitsPerSample == 0U || parsed.bitsPerSample % 8U != 0U || expectedBlockAlign == 0U ||
                parsed.blockAlign != expectedBlockAlign || expectedByteRate > std::numeric_limits<std::uint32_t>::max() ||
                parsed.byteRate != expectedByteRate) {
                return std::nullopt;
            }
            metadata = parsed;
        } else if (HasTag(bytes, offset, 'd', 'a', 't', 'a')) {
            if (chunkBytes == 0U || dataBytes != 0U) {
                return std::nullopt;
            }
            dataBytes = chunkBytes;
        }
        const std::size_t paddedBytes = static_cast<std::size_t>(chunkBytes) + (chunkBytes & 1U);
        if (offset > bytes.size() - kChunkHeaderBytes - paddedBytes) {
            return std::nullopt;
        }
        offset += kChunkHeaderBytes + paddedBytes;
    }
    if (offset != riffEnd || !metadata.has_value() || dataBytes == 0U ||
        dataBytes % metadata->blockAlign != 0U) {
        return std::nullopt;
    }
    metadata->dataBytes = dataBytes;
    metadata->durationSeconds = static_cast<float>(static_cast<double>(dataBytes) /
                                                   static_cast<double>(metadata->byteRate));
    if (!std::isfinite(metadata->durationSeconds) || metadata->durationSeconds <= 0.0F) {
        return std::nullopt;
    }
    return metadata;
}

} // namespace UVE::Audio
