// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once
#include <cstdint>
#include <optional>
namespace UVE::Asset {
struct AnimationClipMetadataInputUVE final {
    float durationSeconds = 0.0F;
    float sampleRateHz = 0.0F;
    std::uint32_t trackCount = 0U;
    std::uint64_t skeletonIdentity = 0U;
};
struct AnimationClipMetadataUVE final {
    float durationSeconds = 0.0F;
    float sampleRateHz = 0.0F;
    std::uint32_t trackCount = 0U;
    std::uint64_t skeletonIdentity = 0U;
};
/// Validates copied animation clip metadata for a decoder/importer handoff. It owns no playback,
/// keyframes, skeleton, retargeting, filesystem, or renderer state.
[[nodiscard]] std::optional<AnimationClipMetadataUVE> ValidateAnimationClipMetadataUVE(
    const AnimationClipMetadataInputUVE& input) noexcept;
} // namespace UVE::Asset
