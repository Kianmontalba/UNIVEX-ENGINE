//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <string>

#include "uve/math/matrix4x4_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Math {

/// An axis-aligned bounding box, used for mesh bounds and frustum culling (Part 7.2,
/// Increments 11-14). Deliberately minimal, matching Vector3UVE/QuaternionUVE's precedent: no
/// ray intersection, no sphere bounds, no oriented bounding box — not needed by anything built
/// so far.
/// Thread-safety: value type; safe to copy/pass freely, no shared state.
struct AabbUVE {
    Vector3UVE min{0.0F, 0.0F, 0.0F};
    Vector3UVE max{0.0F, 0.0F, 0.0F};

    /// Builds an AabbUVE from a center point and per-axis half-extents.
    [[nodiscard]] static constexpr AabbUVE FromCenterExtentsUVE(Vector3UVE center, Vector3UVE extents) noexcept {
        return AabbUVE{
            Vector3UVE{center.x - extents.x, center.y - extents.y, center.z - extents.z},
            Vector3UVE{center.x + extents.x, center.y + extents.y, center.z + extents.z},
        };
    }

    [[nodiscard]] constexpr Vector3UVE GetCenterUVE() const noexcept {
        return Vector3UVE{(min.x + max.x) * 0.5F, (min.y + max.y) * 0.5F, (min.z + max.z) * 0.5F};
    }

    [[nodiscard]] constexpr Vector3UVE GetExtentsUVE() const noexcept {
        return Vector3UVE{(max.x - min.x) * 0.5F, (max.y - min.y) * 0.5F, (max.z - min.z) * 0.5F};
    }

    /// Returns the smallest AabbUVE containing both `*this` and `other`.
    [[nodiscard]] constexpr AabbUVE UnionUVE(const AabbUVE& other) const noexcept {
        return AabbUVE{
            Vector3UVE{min.x < other.min.x ? min.x : other.min.x, min.y < other.min.y ? min.y : other.min.y,
                       min.z < other.min.z ? min.z : other.min.z},
            Vector3UVE{max.x > other.max.x ? max.x : other.max.x, max.y > other.max.y ? max.y : other.max.y,
                       max.z > other.max.z ? max.z : other.max.z},
        };
    }

    /// Returns the smallest axis-aligned box containing `*this` transformed by `matrix`.
    /// Implemented by transforming all 8 corners and taking the componentwise min/max —
    /// deliberately the simplest obviously-correct approach (not Arvo's faster analytical
    /// method); revisit only if profiling ever shows it matters.
    [[nodiscard]] AabbUVE TransformUVE(const Matrix4x4UVE& matrix) const noexcept;
};

[[nodiscard]] constexpr bool operator==(const AabbUVE& lhs, const AabbUVE& rhs) noexcept {
    return lhs.min == rhs.min && lhs.max == rhs.max;
}

[[nodiscard]] constexpr bool operator!=(const AabbUVE& lhs, const AabbUVE& rhs) noexcept {
    return !(lhs == rhs);
}

/// Formats `box` as `"[min .. max]"`, for logging/debugging.
[[nodiscard]] std::string ToStringUVE(const AabbUVE& box);

} // namespace UVE::Math
