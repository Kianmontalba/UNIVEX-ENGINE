// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once
#include <cstddef>
#include <vector>
namespace UVE::Audio {
inline constexpr std::size_t kMaximumPcmGainSamplesUVE = 1U << 20U;
/// Applies a bounded finite gain to copied normalized PCM samples with hard [-1, 1] clamping.
/// The helper owns no device, voice, mixer, stream, or sample-buffer lifetime.
[[nodiscard]] bool ApplyPcmGainEffectUVE(const std::vector<float>& inputSamples, float gain,
                                         std::vector<float>& outputSamples) noexcept;
} // namespace UVE::Audio
