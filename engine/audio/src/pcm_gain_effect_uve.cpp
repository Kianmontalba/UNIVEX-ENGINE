// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/audio/pcm_gain_effect_uve.h"
#include <algorithm>
#include <cmath>
#include <utility>
namespace UVE::Audio {
bool ApplyPcmGainEffectUVE(const std::vector<float>& inputSamples, const float gain,
                          std::vector<float>& outputSamples) noexcept {
    if (inputSamples.size() > kMaximumPcmGainSamplesUVE || !std::isfinite(gain) || gain < 0.0F) {
        return false;
    }
    try {
        std::vector<float> output;
        output.reserve(inputSamples.size());
        for (const float sample : inputSamples) {
            if (!std::isfinite(sample)) {
                return false;
            }
            output.push_back(std::clamp(sample * gain, -1.0F, 1.0F));
        }
        outputSamples = std::move(output);
        return true;
    } catch (...) {
        return false;
    }
}
} // namespace UVE::Audio
