//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <optional>
#include <string>

#include "uve/math/matrix4x4_uve.h"
#include "uve/math/ray_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Math {

/// An axis-aligned bounding box, used for mesh bounds and frustum culling (Part 7.2,
/// Increments 11-14) and AABB broad/narrow-phase collision (Part 7.5, Increment 15).
/// Deliberately minimal, matching Vector3UVE/QuaternionUVE's precedent: no ray intersection, no
/// sphere bounds, no oriented bounding box — not needed by anything built so far.
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

    /// True iff `*this` and `other` overlap on all three axes (touching-but-not-overlapping
    /// edges count as not intersecting). The broad-phase (and, for an axis-aligned-box-only
    /// world, narrow-phase) overlap test CollisionSystemUVE (Part 7.5) is built on.
    [[nodiscard]] constexpr bool IntersectsUVE(const AabbUVE& other) const noexcept {
        return min.x < other.max.x && max.x > other.min.x && min.y < other.max.y && max.y > other.min.y &&
               min.z < other.max.z && max.z > other.min.z;
    }
};

/// The minimum-translation-vector (MTV) between two overlapping AABBs: the shortest
/// single-axis push that separates them. `axis` is a unit vector pointing from `a` toward `b`;
/// `depth` is the overlap distance along that axis. Pure AABB geometry (same category as
/// FrustumUVE's plane math), so it lives here rather than being duplicated inside
/// CollisionSystemUVE (Part 7.5).
struct PenetrationUVE {
    Vector3UVE axis;
    float depth = 0.0F;
};

/// Returns the MTV separating `a` and `b`, or std::nullopt if they don't overlap
/// (`a.IntersectsUVE(b)` is false). When multiple axes tie for the smallest overlap, the first
/// in x/y/z order is returned — deterministic, not an arbitrary implementation detail.
[[nodiscard]] std::optional<PenetrationUVE> ComputePenetrationUVE(const AabbUVE& a, const AabbUVE& b) noexcept;

/// The result of a ray hitting an AabbUVE: `distance` along `ray.direction` (i.e. the hit point
/// is `ray.origin + ray.direction * distance`), and the unit-length face `normal` that was hit —
/// except when the ray's origin is already inside the box, in which case `distance = 0` and
/// `normal` is the zero vector (no face was actually crossed to reach it).
struct RayHitUVE {
    float distance = 0.0F;
    Vector3UVE normal;
};

/// Returns the closest hit of `ray` against `aabb` within `[0, maxDistance]`, or std::nullopt if
/// the ray misses or the box is beyond `maxDistance`. Standard slab method — pure AABB geometry,
/// so it lives here next to ComputePenetrationUVE rather than inside RaycastSystemUVE (Part 7.5,
/// Increment 16). A ray origin already inside the box reports `distance = 0` (hits immediately,
/// rather than "behind" the ray), matching common engine convention.
[[nodiscard]] std::optional<RayHitUVE> IntersectRayUVE(const RayUVE& ray, const AabbUVE& aabb,
                                                         float maxDistance) noexcept;

[[nodiscard]] constexpr bool operator==(const AabbUVE& lhs, const AabbUVE& rhs) noexcept {
    return lhs.min == rhs.min && lhs.max == rhs.max;
}

[[nodiscard]] constexpr bool operator!=(const AabbUVE& lhs, const AabbUVE& rhs) noexcept {
    return !(lhs == rhs);
}

/// Formats `box` as `"[min .. max]"`, for logging/debugging.
[[nodiscard]] std::string ToStringUVE(const AabbUVE& box);

} // namespace UVE::Math
