// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/input/mobile_gesture_recognizer_uve.h"

#include <algorithm>
#include <cmath>

namespace UVE::Input {
namespace {

constexpr MobileGestureRecognizerConfigUVE kDefaultGestureConfig{};

} // namespace

bool EvaluateTouchChordLifecycleTransitionUVE(
    const MobileInputSnapshotUVE& previous, const MobileInputSnapshotUVE& current,
    const std::size_t requiredTouchCount,
    TouchChordLifecycleTransitionUVE& outTransition) noexcept {
    if (requiredTouchCount < 2U || requiredTouchCount > kMaximumTouchCountUVE) {
        return false;
    }

    const auto validateSnapshot = [](const MobileInputSnapshotUVE& snapshot,
                                     std::size_t& outActiveCount) noexcept {
        std::size_t activeCount = 0U;
        for (std::size_t slot = 0U; slot < kMaximumTouchCountUVE; ++slot) {
            const TouchPointStateUVE& touch = snapshot.touches[slot];
            if (!touch.active) {
                continue;
            }
            if (touch.identifier == 0U || !std::isfinite(touch.position.x) ||
                !std::isfinite(touch.position.y) || !std::isfinite(touch.delta.x) ||
                !std::isfinite(touch.delta.y) || !std::isfinite(touch.pressure) ||
                touch.pressure < 0.0F || touch.pressure > 1.0F) {
                return false;
            }
            for (std::size_t previousSlot = 0U; previousSlot < slot; ++previousSlot) {
                if (snapshot.touches[previousSlot].active &&
                    snapshot.touches[previousSlot].identifier == touch.identifier) {
                    return false;
                }
            }
            ++activeCount;
        }
        outActiveCount = activeCount;
        return true;
    };

    std::size_t previousCount = 0U;
    std::size_t currentCount = 0U;
    if (!validateSnapshot(previous, previousCount) ||
        !validateSnapshot(current, currentCount)) {
        return false;
    }

    const bool previousMatches = previousCount == requiredTouchCount;
    const bool currentMatches = currentCount == requiredTouchCount;
    TouchChordLifecycleTransitionUVE transition = TouchChordLifecycleTransitionUVE::None;
    if (!previousMatches && currentMatches) {
        transition = TouchChordLifecycleTransitionUVE::Began;
    } else if (previousMatches && !currentMatches) {
        transition = TouchChordLifecycleTransitionUVE::Ended;
    } else if (previousMatches && currentMatches) {
        bool sameIdentifiers = true;
        bool moved = false;
        for (std::size_t currentSlot = 0U; currentSlot < kMaximumTouchCountUVE; ++currentSlot) {
            const TouchPointStateUVE& currentTouch = current.touches[currentSlot];
            if (!currentTouch.active) {
                continue;
            }
            const TouchPointStateUVE* previousTouch = nullptr;
            for (std::size_t previousSlot = 0U; previousSlot < kMaximumTouchCountUVE; ++previousSlot) {
                const TouchPointStateUVE& candidate = previous.touches[previousSlot];
                if (candidate.active && candidate.identifier == currentTouch.identifier) {
                    previousTouch = &candidate;
                    break;
                }
            }
            if (previousTouch == nullptr) {
                sameIdentifiers = false;
                continue;
            }
            moved = moved || previousTouch->position != currentTouch.position ||
                    previousTouch->pressure != currentTouch.pressure;
        }
        transition = !sameIdentifiers
                         ? TouchChordLifecycleTransitionUVE::Replaced
                         : (moved ? TouchChordLifecycleTransitionUVE::Moved
                                  : TouchChordLifecycleTransitionUVE::None);
    }
    outTransition = transition;
    return true;
}

bool EvaluateTouchChordUVE(const MobileInputSnapshotUVE& snapshot,
                           const std::size_t requiredTouchCount,
                           Math::Vector2UVE& outCentroid) noexcept {
    if (requiredTouchCount < 2U || requiredTouchCount > kMaximumTouchCountUVE) {
        return false;
    }
    Math::Vector2UVE sum{};
    std::size_t activeCount = 0U;
    for (std::size_t slot = 0U; slot < kMaximumTouchCountUVE; ++slot) {
        const TouchPointStateUVE& touch = snapshot.touches[slot];
        if (!touch.active) {
            continue;
        }
        if (touch.identifier == 0U || !std::isfinite(touch.position.x) ||
            !std::isfinite(touch.position.y)) {
            return false;
        }
        for (std::size_t previous = 0U; previous < slot; ++previous) {
            if (snapshot.touches[previous].active &&
                snapshot.touches[previous].identifier == touch.identifier) {
                return false;
            }
        }
        sum.x += touch.position.x;
        sum.y += touch.position.y;
        ++activeCount;
    }
    if (activeCount != requiredTouchCount) {
        return false;
    }
    const float inverseCount = 1.0F / static_cast<float>(activeCount);
    const Math::Vector2UVE centroid{sum.x * inverseCount, sum.y * inverseCount};
    if (!std::isfinite(centroid.x) || !std::isfinite(centroid.y)) {
        return false;
    }
    outCentroid = centroid;
    return true;
}

MobileGestureRecognizerConfigUVE MobileGestureRecognizerUVE::SanitizeConfigUVE(
    MobileGestureRecognizerConfigUVE config) noexcept {
    if (!std::isfinite(config.maximumTapDurationSeconds) || config.maximumTapDurationSeconds <= 0.0F) {
        config.maximumTapDurationSeconds = kDefaultGestureConfig.maximumTapDurationSeconds;
    }
    if (!std::isfinite(config.maximumTapDistance) || config.maximumTapDistance <= 0.0F) {
        config.maximumTapDistance = kDefaultGestureConfig.maximumTapDistance;
    }
    if (!std::isfinite(config.minimumSwipeDistance) || config.minimumSwipeDistance <= 0.0F) {
        config.minimumSwipeDistance = kDefaultGestureConfig.minimumSwipeDistance;
    }
    if (!std::isfinite(config.maximumSwipeDurationSeconds) || config.maximumSwipeDurationSeconds <= 0.0F) {
        config.maximumSwipeDurationSeconds = kDefaultGestureConfig.maximumSwipeDurationSeconds;
    }
    if (config.minimumSwipeDistance < config.maximumTapDistance) {
        config.minimumSwipeDistance = config.maximumTapDistance;
    }
    return config;
}

MobileGestureRecognizerUVE::MobileGestureRecognizerUVE(
    const MobileGestureRecognizerConfigUVE config) noexcept
    : m_config(SanitizeConfigUVE(config)) {}

bool MobileGestureRecognizerUVE::IsFiniteVectorUVE(const Math::Vector2UVE value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y);
}

MobileSwipeDirectionUVE MobileGestureRecognizerUVE::GetSwipeDirectionUVE(
    const Math::Vector2UVE delta) noexcept {
    if (std::fabs(delta.x) >= std::fabs(delta.y)) {
        return delta.x >= 0.0F ? MobileSwipeDirectionUVE::PositiveX : MobileSwipeDirectionUVE::NegativeX;
    }
    return delta.y >= 0.0F ? MobileSwipeDirectionUVE::PositiveY : MobileSwipeDirectionUVE::NegativeY;
}

MobileGestureEventUVE MobileGestureRecognizerUVE::BuildGestureEventUVE(
    const TouchTrackUVE& track, const MobileGestureTypeUVE type,
    const MobileSwipeDirectionUVE direction) noexcept {
    return MobileGestureEventUVE{type,
                                 direction,
                                 track.identifier,
                                 track.startPosition,
                                 track.lastPosition,
                                 track.lastPosition - track.startPosition,
                                 track.durationSeconds};
}

void MobileGestureRecognizerUVE::AppendReleaseGestureUVE(const TouchTrackUVE& track,
                                                          MobileGestureReportUVE& report) const noexcept {
    if (report.count >= report.events.size()) {
        report.truncated = true;
        return;
    }
    const Math::Vector2UVE delta = track.lastPosition - track.startPosition;
    if (!IsFiniteVectorUVE(delta)) {
        return;
    }
    const double deltaX = static_cast<double>(delta.x);
    const double deltaY = static_cast<double>(delta.y);
    const double distanceSquared = deltaX * deltaX + deltaY * deltaY;
    const double tapDistanceSquared = static_cast<double>(m_config.maximumTapDistance) *
                                      static_cast<double>(m_config.maximumTapDistance);
    const double swipeDistanceSquared = static_cast<double>(m_config.minimumSwipeDistance) *
                                        static_cast<double>(m_config.minimumSwipeDistance);
    MobileGestureTypeUVE type = MobileGestureTypeUVE::Tap;
    MobileSwipeDirectionUVE direction = MobileSwipeDirectionUVE::PositiveX;
    if (track.durationSeconds <= m_config.maximumTapDurationSeconds && distanceSquared <= tapDistanceSquared) {
        type = MobileGestureTypeUVE::Tap;
    } else if (track.durationSeconds <= m_config.maximumSwipeDurationSeconds &&
               distanceSquared >= swipeDistanceSquared) {
        type = MobileGestureTypeUVE::Swipe;
        direction = GetSwipeDirectionUVE(delta);
    } else {
        return;
    }
    report.events[report.count++] = BuildGestureEventUVE(track, type, direction);
}

MobileGestureReportUVE MobileGestureRecognizerUVE::ConsumeSnapshotUVE(
    const MobileInputSnapshotUVE& snapshot, const float frameDeltaSeconds) noexcept {
    MobileGestureReportUVE report{};
    if (snapshot.frameNumber == 0U || snapshot.frameNumber <= m_lastFrameNumber ||
        !std::isfinite(frameDeltaSeconds) || frameDeltaSeconds < 0.0F) {
        return report;
    }
    m_lastFrameNumber = snapshot.frameNumber;
    const float deltaSeconds = std::clamp(frameDeltaSeconds, 0.0F, 10.0F);

    for (std::size_t touchSlot = 0U; touchSlot < kMaximumTouchCountUVE; ++touchSlot) {
        const TouchPointStateUVE& touch = snapshot.touches[touchSlot];
        TouchTrackUVE& track = m_tracks[touchSlot];
        if (touch.active && IsFiniteVectorUVE(touch.position)) {
            if (!track.active || track.identifier != touch.identifier) {
                if (track.active) {
                    AppendReleaseGestureUVE(track, report);
                }
                track = TouchTrackUVE{true, touch.identifier, touch.position, touch.position, 0.0F};
            } else {
                track.lastPosition = touch.position;
                track.durationSeconds += deltaSeconds;
            }
        } else if (track.active) {
            AppendReleaseGestureUVE(track, report);
            track = {};
        }
    }
    return report;
}

void MobileGestureRecognizerUVE::ResetUVE() noexcept {
    m_tracks.fill({});
    m_lastFrameNumber = 0U;
}

} // namespace UVE::Input
