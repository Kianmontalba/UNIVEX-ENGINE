// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "uve/math/quaternion_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Asset {

inline constexpr std::size_t kMaximumAnimationAssetPayloadBytesUVE = 8U * 1024U * 1024U;
inline constexpr std::size_t kMaximumAnimationAssetSamplesUVE = 4096U;
inline constexpr std::size_t kMaximumAnimationAssetEventsUVE = 1024U;
inline constexpr std::size_t kMaximumAnimationAssetIdentifierBytesUVE = 128U;

struct AnimationAssetPoseUVE final {
    Math::Vector3UVE position;
    Math::QuaternionUVE rotation;
    Math::Vector3UVE scale{1.0F, 1.0F, 1.0F};
    [[nodiscard]] bool operator==(const AnimationAssetPoseUVE&) const noexcept = default;
};

struct AnimationAssetSampleUVE final {
    double timeSeconds = 0.0;
    AnimationAssetPoseUVE pose;
    [[nodiscard]] bool operator==(const AnimationAssetSampleUVE&) const noexcept = default;
};

struct AnimationAssetEventUVE final {
    double timeSeconds = 0.0;
    std::string eventId;
    [[nodiscard]] bool operator==(const AnimationAssetEventUVE&) const noexcept = default;
};

struct AnimationClipAssetUVE final {
    std::string clipId;
    double durationSeconds = 0.0;
    std::vector<AnimationAssetSampleUVE> samples;
    std::vector<AnimationAssetEventUVE> events;
};

/// Validates the bounded serialized animation payload without performing runtime sampling.
[[nodiscard]] bool IsAnimationClipAssetValidUVE(const AnimationClipAssetUVE& clip) noexcept;

/// Loads a `.uveanim` envelope containing the stable `uve-animation-v1` JSON payload.
/// Output is published only after envelope, schema, bounds, and finite-pose validation succeed.
[[nodiscard]] bool LoadAnimationClipAssetUVE(const std::filesystem::path& path,
                                              AnimationClipAssetUVE& outClip);

/// Saves a validated animation clip as an AssetKindUVE::Animation envelope.
[[nodiscard]] bool SaveAnimationClipAssetUVE(const AnimationClipAssetUVE& clip,
                                              const std::filesystem::path& path);

} // namespace UVE::Asset
