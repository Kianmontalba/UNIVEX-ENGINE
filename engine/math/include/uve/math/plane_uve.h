// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/math/vector3_uve.h"

namespace UVE::Math {

/// A plane in `normal . point + distance = 0` form, where `normal` is expected to be unit
/// length. Used by FrustumUVE (Part 7.2 culling). Deliberately minimal: only the signed-distance
/// query FrustumUVE's intersection test needs.
/// Thread-safety: value type; safe to copy/pass freely, no shared state.
struct PlaneUVE {
    Vector3UVE normal{0.0F, 1.0F, 0.0F};
    float distance = 0.0F;

    /// Builds a plane with the given unit `normal` passing through `point`.
    [[nodiscard]] static constexpr PlaneUVE FromNormalAndPointUVE(Vector3UVE normal, Vector3UVE point) noexcept {
        return PlaneUVE{normal, -(normal.x * point.x + normal.y * point.y + normal.z * point.z)};
    }

    /// Positive when `point` is on the side `normal` points toward, negative on the other side,
    /// zero exactly on the plane.
    [[nodiscard]] constexpr float GetSignedDistanceUVE(Vector3UVE point) const noexcept {
        return normal.x * point.x + normal.y * point.y + normal.z * point.z + distance;
    }
};

} // namespace UVE::Math
