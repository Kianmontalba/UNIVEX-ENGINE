// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/core/time_pose_contract_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace UVE::Core {

struct AnimationEventUVE final {
    double timeSeconds = 0.0;
    std::string eventId;

    [[nodiscard]] bool operator==(const AnimationEventUVE&) const = default;
};

struct AnimationClipUVE final {
    static constexpr std::size_t kMaximumSamplesUVE = 4096U;
    static constexpr std::size_t kMaximumEventsUVE = 1024U;
    static constexpr std::size_t kMaximumCollectedEventsUVE = 1024U;
    static constexpr std::size_t kMaximumIdentifierBytesUVE = 128U;

    std::string clipId;
    double durationSeconds = 0.0;
    std::vector<PoseSampleUVE> samples;
    std::vector<AnimationEventUVE> events;
};

enum class AnimationClipValidationCodeUVE : std::uint8_t {
    Valid = 0,
    InvalidIdentifier,
    InvalidDuration,
    CapacityExceeded,
    InvalidSampleTime,
    UnsortedSamples,
    InvalidPose,
    InvalidEventTime,
    UnsortedEvents,
    InvalidEventIdentifier,
};

struct AnimationClipValidationResultUVE final {
    AnimationClipValidationCodeUVE code = AnimationClipValidationCodeUVE::InvalidIdentifier;
    std::size_t index = 0U;
    std::string message;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return code == AnimationClipValidationCodeUVE::Valid;
    }
};

[[nodiscard]] AnimationClipValidationResultUVE ValidateAnimationClipUVE(
    const AnimationClipUVE& clip) noexcept;

[[nodiscard]] bool TrySampleAnimationClipUVE(const AnimationClipUVE& clip, double timeSeconds,
                                             bool looping, TransformPoseUVE& outPose) noexcept;

/// Collects events in chronological loop order and caps copied output at
/// `AnimationClipUVE::kMaximumCollectedEventsUVE`; excessively large loop indices/ranges fail closed.
[[nodiscard]] std::vector<AnimationEventUVE> CollectAnimationEventsUVE(
    const AnimationClipUVE& clip, double startSeconds, double endSeconds, bool looping);

} // namespace UVE::Core
