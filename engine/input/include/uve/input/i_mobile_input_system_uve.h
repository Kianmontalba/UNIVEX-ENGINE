// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <array>
#include <cstddef>
#include <cmath>
#include <cstdint>

#include "uve/math/vector2_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Input {

inline constexpr std::size_t kMaximumTouchCountUVE = 10U;

/// One copied touch-slot value committed by MobileInputSystemUVE.
/// `identifier` remains stable for the lifetime of a platform touch contact; `delta` is computed
/// only when the same identifier occupies the same slot in adjacent committed frames.
struct TouchPointStateUVE final {
    bool active = false;
    std::uint64_t identifier = 0U;
    Math::Vector2UVE position{};
    Math::Vector2UVE delta{};
    float pressure = 0.0F;

    [[nodiscard]] bool operator==(const TouchPointStateUVE&) const noexcept = default;
};

/// Bounded, backend-neutral mobile input snapshot. Gyroscope values are rotation rates in radians
/// per second, normalized only for finite input; platform adapters own coordinate-system mapping.
enum class TouchLifecycleTransitionUVE : std::uint8_t {
    None = 0,
    Began,
    Moved,
    Ended,
    Replaced,
};

enum class MobileLifecycleStateUVE : std::uint8_t {
    Inactive = 0,
    Active,
    Suspended,
    Terminated,
    Count,
};

enum class MobileLifecycleTransitionUVE : std::uint8_t {
    None = 0,
    Activated,
    Resumed,
    Suspended,
    Deactivated,
    Terminated,
    Reinitialized,
};

/// Classifies a copied application-lifecycle edge for Android/iOS adapters without owning native
/// lifecycle callbacks, process state, or platform shutdown policy.
[[nodiscard]] inline bool EvaluateMobileLifecycleTransitionUVE(
    const MobileLifecycleStateUVE previous, const MobileLifecycleStateUVE current,
    MobileLifecycleTransitionUVE& outTransition) noexcept {
    const auto isValid = [](const MobileLifecycleStateUVE state) noexcept {
        return static_cast<std::uint8_t>(state) < static_cast<std::uint8_t>(MobileLifecycleStateUVE::Count);
    };
    if (!isValid(previous) || !isValid(current)) {
        return false;
    }
    MobileLifecycleTransitionUVE transition = MobileLifecycleTransitionUVE::None;
    if (previous != current) {
        switch (current) {
        case MobileLifecycleStateUVE::Active:
            transition = previous == MobileLifecycleStateUVE::Suspended
                             ? MobileLifecycleTransitionUVE::Resumed
                             : (previous == MobileLifecycleStateUVE::Terminated
                                    ? MobileLifecycleTransitionUVE::Reinitialized
                                    : MobileLifecycleTransitionUVE::Activated);
            break;
        case MobileLifecycleStateUVE::Suspended:
            transition = MobileLifecycleTransitionUVE::Suspended;
            break;
        case MobileLifecycleStateUVE::Inactive:
            transition = MobileLifecycleTransitionUVE::Deactivated;
            break;
        case MobileLifecycleStateUVE::Terminated:
            transition = MobileLifecycleTransitionUVE::Terminated;
            break;
        case MobileLifecycleStateUVE::Count:
            return false;
        }
    }
    outTransition = transition;
    return true;
}

/// Classifies one copied touch-slot transition. Replaced means two active frames have different
/// identifiers in the same slot; the helper does not poll hardware, own lifecycle state, or mutate snapshots.
[[nodiscard]] inline bool EvaluateTouchLifecycleTransitionUVE(
    const TouchPointStateUVE& previous, const TouchPointStateUVE& current,
    TouchLifecycleTransitionUVE& outTransition) noexcept {
    const auto isValid = [](const TouchPointStateUVE& touch) noexcept {
        return !touch.active || (touch.identifier != 0U && std::isfinite(touch.position.x) &&
                                 std::isfinite(touch.position.y) && std::isfinite(touch.delta.x) &&
                                 std::isfinite(touch.delta.y) && std::isfinite(touch.pressure) &&
                                 touch.pressure >= 0.0F && touch.pressure <= 1.0F);
    };
    if (!isValid(previous) || !isValid(current)) return false;
    TouchLifecycleTransitionUVE transition = TouchLifecycleTransitionUVE::None;
    if (!previous.active && current.active) {
        transition = TouchLifecycleTransitionUVE::Began;
    } else if (previous.active && !current.active) {
        transition = TouchLifecycleTransitionUVE::Ended;
    } else if (previous.active && current.active) {
        transition = previous.identifier == current.identifier
                         ? ((previous.position == current.position && previous.pressure == current.pressure)
                                ? TouchLifecycleTransitionUVE::None
                                : TouchLifecycleTransitionUVE::Moved)
                         : TouchLifecycleTransitionUVE::Replaced;
    }
    outTransition = transition;
    return true;
}

struct MobileInputSnapshotUVE final {
    std::array<TouchPointStateUVE, kMaximumTouchCountUVE> touches{};
    Math::Vector3UVE gyroscopeRotationRate{};
    std::uint64_t frameNumber = 0U;

    [[nodiscard]] bool operator==(const MobileInputSnapshotUVE&) const noexcept = default;
};

/// Injectable mobile input value service. It owns only live/current/previous copied snapshots and
/// does not poll Android/iOS APIs, own an application lifecycle, or select a platform backend.
/// Set*StateUVE() methods are safe from any thread; UpdateUVE() and snapshot queries are main-thread-only.
class IMobileInputSystemUVE {
public:
    virtual ~IMobileInputSystemUVE() = default;

    virtual void SetTouchStateUVE(std::size_t touchSlot, bool active, std::uint64_t identifier,
                                  Math::Vector2UVE position, float pressure) = 0;
    virtual void SetGyroscopeRotationRateUVE(Math::Vector3UVE rotationRate) = 0;
    virtual void UpdateUVE() = 0;

    [[nodiscard]] virtual MobileInputSnapshotUVE GetSnapshotUVE() const = 0;
    [[nodiscard]] virtual MobileInputSnapshotUVE GetPreviousSnapshotUVE() const = 0;
};

} // namespace UVE::Input
