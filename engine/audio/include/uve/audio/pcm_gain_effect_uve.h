// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once
#include <cstddef>
#include <vector>
namespace UVE::Audio {
inline constexpr std::size_t kMaximumPcmGainSamplesUVE = 1U << 20U;
inline constexpr std::size_t kMaximumPcmGainChainEffectsUVE = 8U;

struct PcmGainEffectWindowUVE final {
    std::size_t startSample = 0U;
    std::size_t sampleCount = 0U;
    float gain = 1.0F;
    [[nodiscard]] bool operator==(const PcmGainEffectWindowUVE&) const noexcept = default;
};

/// Applies ordered bounded gain windows to copied samples. Overlapping windows are applied in
/// caller order; the helper owns no mixer, device, stream, voice, or scheduler lifetime.
[[nodiscard]] bool ApplyScheduledPcmGainEffectsUVE(
    const std::vector<float>& inputSamples, const std::vector<PcmGainEffectWindowUVE>& windows,
    std::vector<float>& outputSamples) noexcept;
/// Applies an ordered bounded chain of finite nonnegative PCM gain stages to copied samples.
/// Allocation and validation failures return false without publishing partial output. The chain owns
/// no mixer, device, stream, voice, or sample-buffer lifetime.
[[nodiscard]] bool ApplyPcmGainEffectChainUVE(const std::vector<float>& inputSamples,
                                               const std::vector<float>& gains,
                                               std::vector<float>& outputSamples) noexcept;

/// Applies a bounded finite gain to copied normalized PCM samples with hard [-1, 1] clamping.
/// The helper owns no device, voice, mixer, stream, or sample-buffer lifetime.
[[nodiscard]] bool ApplyPcmGainEffectUVE(const std::vector<float>& inputSamples, float gain,
                                         std::vector<float>& outputSamples) noexcept;
} // namespace UVE::Audio
