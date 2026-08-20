// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once
#include <cstddef>
#include <vector>
namespace UVE::Audio {
inline constexpr std::size_t kMaximumWavPcm16SamplesUVE = 1U << 20U;
/// Validates one bounded PCM16 sample window for caller-owned streaming/chunk preparation.
[[nodiscard]] bool ValidateWavPcm16SampleWindowUVE(std::size_t totalSamples,
                                                    std::size_t startSample,
                                                    std::size_t requestedSamples,
                                                    std::size_t maximumSamples = kMaximumWavPcm16SamplesUVE) noexcept;
/// Decodes validated PCM16 WAV bytes into copied normalized float samples.
/// Supports only RIFF/WAVE PCM16; owns no decoder, stream, device, or voice lifetime.
[[nodiscard]] bool DecodeWavPcm16SamplesUVE(const std::vector<std::byte>& wavBytes,
                                            std::vector<float>& outSamples) noexcept;
} // namespace UVE::Audio
