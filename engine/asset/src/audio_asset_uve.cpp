// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/audio_asset_uve.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include "uve/asset/uve_file_envelope_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Asset {
namespace {

constexpr std::uint32_t kAudioAssetVersionUVE = 1U;

void AppendBytesUVE(std::vector<std::byte>& buffer, const void* data, const std::size_t size) {
    const auto* const bytes = static_cast<const std::byte*>(data);
    buffer.insert(buffer.end(), bytes, bytes + size);
}

void AppendU16LittleEndianUVE(std::vector<std::byte>& buffer, const std::uint16_t value) {
    buffer.push_back(std::byte{static_cast<unsigned char>(value & 0xFFU)});
    buffer.push_back(std::byte{static_cast<unsigned char>((value >> 8U) & 0xFFU)});
}

void AppendU32LittleEndianUVE(std::vector<std::byte>& buffer, const std::uint32_t value) {
    for (unsigned int shift = 0U; shift < 32U; shift += 8U) {
        buffer.push_back(std::byte{static_cast<unsigned char>((value >> shift) & 0xFFU)});
    }
}

void AppendU64LittleEndianUVE(std::vector<std::byte>& buffer, const std::uint64_t value) {
    for (unsigned int shift = 0U; shift < 64U; shift += 8U) {
        buffer.push_back(std::byte{static_cast<unsigned char>((value >> shift) & 0xFFU)});
    }
}

[[nodiscard]] bool ReadBytesUVE(const std::vector<std::byte>& buffer, std::size_t& offset,
                                void* destination, const std::size_t size) {
    if (size > buffer.size() - std::min(offset, buffer.size())) return false;
    std::memcpy(destination, buffer.data() + offset, size);
    offset += size;
    return true;
}

[[nodiscard]] bool ReadU16LittleEndianUVE(const std::vector<std::byte>& buffer, std::size_t& offset,
                                          std::uint16_t& outValue) {
    if (offset > buffer.size() || buffer.size() - offset < 2U) return false;
    outValue = static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(buffer[offset])) |
               static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(buffer[offset + 1U]) << 8U);
    offset += 2U;
    return true;
}

[[nodiscard]] bool ReadU32LittleEndianUVE(const std::vector<std::byte>& buffer, std::size_t& offset,
                                          std::uint32_t& outValue) {
    if (offset > buffer.size() || buffer.size() - offset < 4U) return false;
    outValue = static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(buffer[offset])) |
               (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(buffer[offset + 1U])) << 8U) |
               (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(buffer[offset + 2U])) << 16U) |
               (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(buffer[offset + 3U])) << 24U);
    offset += 4U;
    return true;
}

[[nodiscard]] bool ReadU64LittleEndianUVE(const std::vector<std::byte>& buffer, std::size_t& offset,
                                          std::uint64_t& outValue) {
    if (offset > buffer.size() || buffer.size() - offset < 8U) return false;
    outValue = 0U;
    for (unsigned int shift = 0U; shift < 64U; shift += 8U) {
        outValue |= static_cast<std::uint64_t>(std::to_integer<std::uint8_t>(buffer[offset++])) << shift;
    }
    return true;
}

[[nodiscard]] bool IsValidAudioUVE(const std::uint16_t channels, const std::uint32_t sampleRate,
                                   const std::vector<float>& samples) noexcept {
    if (channels == 0U || sampleRate == 0U || samples.empty() || samples.size() > kMaximumAudioAssetSamplesUVE ||
        samples.size() % channels != 0U) {
        return false;
    }
    for (const float sample : samples) {
        if (!std::isfinite(sample) || sample < -1.0F || sample > 1.0F) return false;
    }
    return true;
}

} // namespace

bool LoadAudioAssetUVE(const std::filesystem::path& path, AudioAssetUVE& outAudio) {
    const auto file = ReadUveFileUVE(path);
    if (!file.has_value() || file->first.assetType != AssetKindUVE::Audio) {
        if (file.has_value()) {
            UVE_ERROR("AudioAssetUVE: \"{}\" is not an audio file", path.string());
        }
        return false;
    }
    const auto& payload = file->second;
    std::size_t offset = 0U;
    std::uint32_t version = 0U;
    std::uint16_t channels = 0U;
    std::uint32_t sampleRate = 0U;
    std::uint64_t sampleCount = 0U;
    if (!ReadU32LittleEndianUVE(payload, offset, version) || !ReadU16LittleEndianUVE(payload, offset, channels) ||
        !ReadU32LittleEndianUVE(payload, offset, sampleRate) || !ReadU64LittleEndianUVE(payload, offset, sampleCount) ||
        version != kAudioAssetVersionUVE || sampleCount == 0U || sampleCount > kMaximumAudioAssetSamplesUVE ||
        sampleCount > std::numeric_limits<std::size_t>::max() ||
        sampleCount > (payload.size() - std::min(offset, payload.size())) / sizeof(float)) {
        UVE_ERROR("AudioAssetUVE: \"{}\" has an invalid or truncated payload header", path.string());
        return false;
    }
    std::vector<float> samples(static_cast<std::size_t>(sampleCount));
    if (!ReadBytesUVE(payload, offset, samples.data(), samples.size() * sizeof(float)) ||
        offset != payload.size() || !IsValidAudioUVE(channels, sampleRate, samples)) {
        UVE_ERROR("AudioAssetUVE: \"{}\" has invalid sample payload data", path.string());
        return false;
    }
    AudioAssetUVE candidate;
    candidate.channels = channels;
    candidate.sampleRate = sampleRate;
    candidate.samples = std::move(samples);
    outAudio = std::move(candidate);
    return true;
}

bool SaveAudioAssetUVE(const AudioAssetUVE& audio, const std::filesystem::path& path) {
    if (!IsValidAudioUVE(audio.channels, audio.sampleRate, audio.samples)) {
        UVE_ERROR("AudioAssetUVE: refusing to save invalid audio asset \"{}\"", path.string());
        return false;
    }
    std::vector<std::byte> payload;
    AppendU32LittleEndianUVE(payload, kAudioAssetVersionUVE);
    AppendU16LittleEndianUVE(payload, audio.channels);
    AppendU32LittleEndianUVE(payload, audio.sampleRate);
    AppendU64LittleEndianUVE(payload, static_cast<std::uint64_t>(audio.samples.size()));
    AppendBytesUVE(payload, audio.samples.data(), audio.samples.size() * sizeof(float));
    return WriteUveFileUVE(path, AssetKindUVE::Audio, payload);
}

} // namespace UVE::Asset
