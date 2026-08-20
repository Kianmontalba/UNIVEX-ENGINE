// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "uve/input/i_mobile_input_system_uve.h"

namespace UVE::Input {

enum class MobileGestureTypeUVE : std::uint8_t {
    Tap,
    Swipe,
};

enum class MobileSwipeDirectionUVE : std::uint8_t {
    PositiveX,
    NegativeX,
    PositiveY,
    NegativeY,
};

enum class TouchChordLifecycleTransitionUVE : std::uint8_t {
    None = 0,
    Began,
    Moved,
    Ended,
    Replaced,
};

/// Classifies an exact-count multi-touch chord across copied previous/current snapshots.
/// The policy owns no gesture lifecycle, platform framework, touch polling, or ECS state.
[[nodiscard]] bool EvaluateTouchChordLifecycleTransitionUVE(
    const MobileInputSnapshotUVE& previous, const MobileInputSnapshotUVE& current,
    std::size_t requiredTouchCount, TouchChordLifecycleTransitionUVE& outTransition) noexcept;

/// Evaluates an exact-count multi-touch chord over one copied snapshot and publishes its centroid.
/// The policy owns no gesture lifecycle, platform framework, touch polling, or ECS state.
[[nodiscard]] bool EvaluateTouchChordUVE(const MobileInputSnapshotUVE& snapshot,
                                          std::size_t requiredTouchCount,
                                          Math::Vector2UVE& outCentroid) noexcept;

struct MobileGestureEventUVE final {
    MobileGestureTypeUVE type = MobileGestureTypeUVE::Tap;
    MobileSwipeDirectionUVE direction = MobileSwipeDirectionUVE::PositiveX;
    std::uint64_t touchIdentifier = 0U;
    Math::Vector2UVE startPosition{};
    Math::Vector2UVE endPosition{};
    Math::Vector2UVE delta{};
    float durationSeconds = 0.0F;

    [[nodiscard]] bool operator==(const MobileGestureEventUVE&) const noexcept = default;
};

struct MobileGestureReportUVE final {
    std::array<MobileGestureEventUVE, kMaximumTouchCountUVE> events{};
    std::size_t count = 0U;
    bool truncated = false;

    [[nodiscard]] bool operator==(const MobileGestureReportUVE&) const noexcept = default;
};

struct MobileGestureRecognizerConfigUVE final {
    float maximumTapDurationSeconds = 0.25F;
    float maximumTapDistance = 24.0F;
    float minimumSwipeDistance = 48.0F;
    float maximumSwipeDurationSeconds = 0.75F;
};

/// Bounded frame-driven tap/swipe classification over copied mobile snapshots. The caller owns
/// platform coordinate mapping and supplies one finite frame delta per new snapshot. The recognizer
/// retains only capped per-touch tracking state and emits gestures when a tracked touch is released
/// or its identifier changes; it does not own device polling, application lifecycle, or ECS state.
class MobileGestureRecognizerUVE final {
public:
    explicit MobileGestureRecognizerUVE(MobileGestureRecognizerConfigUVE config = {}) noexcept;

    [[nodiscard]] MobileGestureReportUVE ConsumeSnapshotUVE(const MobileInputSnapshotUVE& snapshot,
                                                             float frameDeltaSeconds) noexcept;
    void ResetUVE() noexcept;

private:
    struct TouchTrackUVE final {
        bool active = false;
        std::uint64_t identifier = 0U;
        Math::Vector2UVE startPosition{};
        Math::Vector2UVE lastPosition{};
        float durationSeconds = 0.0F;
    };

    [[nodiscard]] static MobileGestureRecognizerConfigUVE SanitizeConfigUVE(
        MobileGestureRecognizerConfigUVE config) noexcept;
    [[nodiscard]] static bool IsFiniteVectorUVE(Math::Vector2UVE value) noexcept;
    [[nodiscard]] static MobileSwipeDirectionUVE GetSwipeDirectionUVE(Math::Vector2UVE delta) noexcept;
    [[nodiscard]] static MobileGestureEventUVE BuildGestureEventUVE(
        const TouchTrackUVE& track, MobileGestureTypeUVE type,
        MobileSwipeDirectionUVE direction) noexcept;
    void AppendReleaseGestureUVE(const TouchTrackUVE& track, MobileGestureReportUVE& report) const noexcept;

    MobileGestureRecognizerConfigUVE m_config;
    std::array<TouchTrackUVE, kMaximumTouchCountUVE> m_tracks{};
    std::uint64_t m_lastFrameNumber = 0U;
};

} // namespace UVE::Input
