// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace UVE::Audio {

struct WavMetadataUVE final {
    std::uint16_t audioFormat = 0U;
    std::uint16_t channels = 0U;
    std::uint32_t sampleRate = 0U;
    std::uint32_t byteRate = 0U;
    std::uint16_t blockAlign = 0U;
    std::uint16_t bitsPerSample = 0U;
    std::uint32_t dataBytes = 0U;
    float durationSeconds = 0.0F;
};

/// Parses bounded PCM WAV RIFF metadata from copied caller-owned bytes. The parser requires a
/// RIFF/WAVE header, one valid PCM fmt chunk, and one valid data chunk; it never exposes samples,
/// allocates a decoder, or transfers ownership to an audio backend.
[[nodiscard]] std::optional<WavMetadataUVE> ParseWavMetadataUVE(const std::vector<std::byte>& bytes);

} // namespace UVE::Audio
