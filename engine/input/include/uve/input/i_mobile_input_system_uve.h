// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <array>
#include <cstddef>
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
