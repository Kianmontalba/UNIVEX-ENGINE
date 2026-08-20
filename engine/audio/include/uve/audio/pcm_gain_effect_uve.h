// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once
#include <cstddef>
#include <vector>
namespace UVE::Audio {
inline constexpr std::size_t kMaximumPcmGainSamplesUVE = 1U << 20U;
inline constexpr std::size_t kMaximumPcmGainChainEffectsUVE = 8U;
/// Applies an ordered bounded chain of finite nonnegative PCM gain stages to copied samples.
/// The chain owns no mixer, device, stream, voice, or sample-buffer lifetime.
[[nodiscard]] bool ApplyPcmGainEffectChainUVE(const std::vector<float>& inputSamples,
                                               const std::vector<float>& gains,
                                               std::vector<float>& outputSamples) noexcept;

/// Applies a bounded finite gain to copied normalized PCM samples with hard [-1, 1] clamping.
/// The helper owns no device, voice, mixer, stream, or sample-buffer lifetime.
[[nodiscard]] bool ApplyPcmGainEffectUVE(const std::vector<float>& inputSamples, float gain,
                                         std::vector<float>& outputSamples) noexcept;
} // namespace UVE::Audio
