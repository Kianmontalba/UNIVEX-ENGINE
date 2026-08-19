// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/asset/animation_clip_metadata_uve.h"
#include <cmath>
namespace UVE::Asset {
std::optional<AnimationClipMetadataUVE> ValidateAnimationClipMetadataUVE(const AnimationClipMetadataInputUVE& input) noexcept {
    constexpr float kMaximumSampleRateHz = 120.0F;
    constexpr std::uint32_t kMaximumTrackCount = 65'536U;
    if (!std::isfinite(input.durationSeconds) || !std::isfinite(input.sampleRateHz) ||
        input.durationSeconds <= 0.0F || input.sampleRateHz <= 0.0F || input.sampleRateHz > kMaximumSampleRateHz ||
        input.trackCount == 0U || input.trackCount > kMaximumTrackCount || input.skeletonIdentity == 0U) {
        return std::nullopt;
    }
    return AnimationClipMetadataUVE{input.durationSeconds, input.sampleRateHz, input.trackCount, input.skeletonIdentity};
}
} // namespace UVE::Asset
