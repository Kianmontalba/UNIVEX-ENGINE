// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/audio/wav_pcm16_decoder_uve.h"
#include "uve/audio/wav_metadata_uve.h"
#include "uve/audio/pcm_gain_effect_uve.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <utility>
namespace UVE::Audio {
namespace {
[[nodiscard]] std::uint16_t ReadU16LE(const std::vector<std::byte>& bytes, const std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes[offset]) |
                                      (std::to_integer<std::uint8_t>(bytes[offset + 1U]) << 8U));
}
[[nodiscard]] std::uint32_t ReadU32LE(const std::vector<std::byte>& bytes, const std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[offset]) |
                                      (std::to_integer<std::uint8_t>(bytes[offset + 1U]) << 8U) |
                                      (std::to_integer<std::uint8_t>(bytes[offset + 2U]) << 16U) |
                                      (std::to_integer<std::uint8_t>(bytes[offset + 3U]) << 24U));
}
[[nodiscard]] bool HasTag(const std::vector<std::byte>& bytes, std::size_t offset, const char* tag) noexcept {
    return offset + 4U <= bytes.size() && std::memcmp(bytes.data() + offset, tag, 4U) == 0;
}
} // namespace

bool ApplyPcmGainEffectChainUVE(const std::vector<float>& inputSamples,
                                  const std::vector<float>& gains,
                                  std::vector<float>& outputSamples) noexcept {
    if (inputSamples.size() > kMaximumPcmGainSamplesUVE ||
        gains.size() > kMaximumPcmGainChainEffectsUVE) {
        return false;
    }
    for (const float sample : inputSamples) {
        if (!std::isfinite(sample)) {
            return false;
        }
    }
    if (gains.empty()) {
        outputSamples = inputSamples;
        return true;
    }
    std::vector<float> working = inputSamples;
    std::vector<float> next;
    for (const float gain : gains) {
        if (!ApplyPcmGainEffectUVE(working, gain, next)) {
            return false;
        }
        working.swap(next);
        next.clear();
    }
    outputSamples = std::move(working);
    return true;
}

bool ValidateWavPcm16SampleWindowUVE(const std::size_t totalSamples,
                                     const std::size_t startSample,
                                     const std::size_t requestedSamples,
                                     const std::size_t maximumSamples) noexcept {
    if (totalSamples == 0U || requestedSamples == 0U || maximumSamples == 0U ||
        requestedSamples > maximumSamples || startSample > totalSamples ||
        requestedSamples > totalSamples - startSample) {
        return false;
    }
    return true;
}

bool DecodeWavPcm16SamplesUVE(const std::vector<std::byte>& wavBytes,
                              std::vector<float>& outSamples) noexcept {
    const auto metadata = ParseWavMetadataUVE(wavBytes);
    if (!metadata || metadata->audioFormat != 1U || metadata->bitsPerSample != 16U ||
        metadata->blockAlign == 0U || metadata->dataBytes % metadata->blockAlign != 0U) {
        return false;
    }
    const std::size_t sampleCount = metadata->dataBytes / 2U;
    if (sampleCount > kMaximumWavPcm16SamplesUVE) {
        return false;
    }
    std::size_t dataOffset = 12U;
    while (dataOffset + 8U <= wavBytes.size()) {
        const std::uint32_t chunkSize = ReadU32LE(wavBytes, dataOffset + 4U);
        const std::size_t payloadOffset = dataOffset + 8U;
        if (payloadOffset > wavBytes.size() || chunkSize > wavBytes.size() - payloadOffset) {
            return false;
        }
        if (HasTag(wavBytes, dataOffset, "data")) {
            if (chunkSize != metadata->dataBytes) return false;
            std::vector<float> samples;
            samples.reserve(sampleCount);
            for (std::size_t offset = payloadOffset; offset < payloadOffset + chunkSize; offset += 2U) {
                const std::int16_t value = static_cast<std::int16_t>(ReadU16LE(wavBytes, offset));
                samples.push_back(std::clamp(static_cast<float>(value) / 32768.0F, -1.0F, 1.0F));
            }
            outSamples = std::move(samples);
            return true;
        }
        dataOffset = payloadOffset + chunkSize + (chunkSize & 1U);
    }
    return false;
}
} // namespace UVE::Audio
