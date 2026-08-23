// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/audio/pcm_gain_effect_uve.h"
#include <algorithm>
#include <cmath>
#include <utility>
namespace UVE::Audio {
bool PcmGainEffectScheduleUVE::ScheduleWindowUVE(const PcmGainEffectWindowUVE& window) noexcept {
    if (m_count >= kMaximumPcmGainScheduledWindowsUVE || window.sampleCount == 0U ||
        !std::isfinite(window.gain) || window.gain < 0.0F) {
        return false;
    }
    m_windows[m_count] = window;
    ++m_count;
    return true;
}

bool PcmGainEffectScheduleUVE::ApplyPendingUVE(const std::vector<float>& inputSamples,
                                                std::vector<float>& outputSamples) noexcept {
    try {
        std::vector<PcmGainEffectWindowUVE> windows;
        windows.reserve(m_count);
        for (std::size_t index = 0U; index < m_count; ++index) {
            windows.push_back(m_windows[index]);
        }
        std::vector<float> candidateOutput;
        if (!ApplyScheduledPcmGainEffectsUVE(inputSamples, windows, candidateOutput)) {
            return false;
        }
        outputSamples = std::move(candidateOutput);
        ResetUVE();
        return true;
    } catch (...) {
        return false;
    }
}

void PcmGainEffectScheduleUVE::ResetUVE() noexcept {
    m_windows.fill(PcmGainEffectWindowUVE{});
    m_count = 0U;
}

bool ApplyScheduledPcmGainEffectsUVE(const std::vector<float>& inputSamples,
                                       const std::vector<PcmGainEffectWindowUVE>& windows,
                                       std::vector<float>& outputSamples) noexcept {
    if (inputSamples.size() > kMaximumPcmGainSamplesUVE || windows.size() > kMaximumPcmGainChainEffectsUVE) {
        return false;
    }
    for (const float sample : inputSamples) {
        if (!std::isfinite(sample)) return false;
    }
    for (const PcmGainEffectWindowUVE& window : windows) {
        if (window.sampleCount == 0U || !std::isfinite(window.gain) || window.gain < 0.0F ||
            window.startSample > inputSamples.size() || window.sampleCount > inputSamples.size() - window.startSample) {
            return false;
        }
    }
    try {
        std::vector<float> working = inputSamples;
        for (const PcmGainEffectWindowUVE& window : windows) {
            for (std::size_t index = window.startSample; index < window.startSample + window.sampleCount; ++index) {
                working[index] = std::clamp(working[index] * window.gain, -1.0F, 1.0F);
            }
        }
        outputSamples = std::move(working);
        return true;
    } catch (...) {
        return false;
    }
}

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
