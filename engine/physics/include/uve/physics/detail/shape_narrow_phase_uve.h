// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <optional>

#include "uve/math/aabb_uve.h"
#include "uve/math/quaternion_uve.h"

namespace UVE::Physics::Detail {

/// Computes the first bounded hit of a moving sphere against a stationary sphere along `ray`.
/// The combined radius is used for exact quadratic time-of-impact; an origin already inside or
/// touching the combined sphere reports distance zero with a zero normal. Invalid, non-finite,
/// overflowed, or out-of-range results fail closed. This value-only helper owns no ECS or backend state.
[[nodiscard]] std::optional<Math::RayHitUVE> IntersectMovingSphereSphereUVE(
    const Math::RayUVE& ray, Math::Vector3UVE targetCenter, float movingRadius, float targetRadius,
    float maxDistance) noexcept;

/// Intersects a point ray against a stationary sphere using the same bounded quadratic policy as
/// `IntersectMovingSphereSphereUVE`; initial containment reports distance zero and a zero normal.
[[nodiscard]] std::optional<Math::RayHitUVE> IntersectRaySphereUVE(
    const Math::RayUVE& ray, Math::Vector3UVE targetCenter, float targetRadius, float maxDistance) noexcept;

/// Intersects a point ray against a finite capsule represented by its world-space centerline segment.
/// Cylinder and hemispherical-cap candidates are evaluated in double precision; initial containment
/// reports distance zero and a zero normal. Invalid or tangent-only inputs fail closed.
[[nodiscard]] std::optional<Math::RayHitUVE> IntersectRayCapsuleUVE(
    const Math::RayUVE& ray, Math::Vector3UVE segmentStart, Math::Vector3UVE segmentEnd,
    float capsuleRadius, float maxDistance) noexcept;

/// Intersects a point ray against a stationary oriented box by transforming the ray to box-local
/// space and reusing bounded slab math. The returned normal is restored to world space.
[[nodiscard]] std::optional<Math::RayHitUVE> IntersectRayOrientedBoxUVE(
    const Math::RayUVE& ray, Math::Vector3UVE boxCenter, Math::Vector3UVE boxHalfExtents,
    Math::QuaternionUVE boxRotation, float maxDistance) noexcept;

/// Computes exact sphere-vs-axis-aligned-box penetration for the current Physics v1 geometry
/// contract. The returned axis points from the sphere center toward the box, matching
/// CollisionPairUVE's first-to-second separation convention when the sphere is the first shape.
/// Touching boundaries are not intersections. This helper is value-only and deliberately does not
/// model oriented boxes, capsule pairs, sphere pairs, transforms, ECS state, or backend resources.
[[nodiscard]] std::optional<Math::PenetrationUVE> ComputeSphereAabbPenetrationUVE(
    const Math::AabbUVE& box, Math::Vector3UVE sphereCenter, float sphereRadius) noexcept;

/// Computes exact overlap distance between a capsule's axis segment and an axis-aligned box, then
/// returns a deterministic generic penetration axis/depth. The segment is supplied explicitly and
/// the capsule radius is applied as a Minkowski expansion. When the center segment is already
/// inside the box, the returned depth is the radius floor needed by the generic resolver. This
/// value-only helper does not model rotated capsules, capsule pairs, sphere pairs, ECS state, or
/// backend resources.
[[nodiscard]] std::optional<Math::PenetrationUVE> ComputeCapsuleAabbPenetrationUVE(
    const Math::AabbUVE& box, Math::Vector3UVE segmentStart, Math::Vector3UVE segmentEnd,
    float capsuleRadius) noexcept;

/// Computes exact sphere-vs-sphere penetration with a deterministic first-to-second axis. Touching
/// spheres are not intersections; coincident centers use +X as the stable fallback axis. This is a
/// copied value-only helper with no ECS, transform, or backend ownership.
[[nodiscard]] std::optional<Math::PenetrationUVE> ComputeSphereSpherePenetrationUVE(
    Math::Vector3UVE firstCenter, float firstRadius, Math::Vector3UVE secondCenter,
    float secondRadius) noexcept;

/// Computes exact capsule-vs-sphere penetration from the capsule centerline segment and sphere
/// center. The returned axis points from the capsule toward the sphere. Touching pairs are not
/// intersections; coincident closest points use +X as the stable fallback axis. This copied
/// value-only helper does not model rotated capsules, capsule pairs, transforms, ECS state, or
/// backend resources.
[[nodiscard]] std::optional<Math::PenetrationUVE> ComputeCapsuleSpherePenetrationUVE(
    Math::Vector3UVE capsuleSegmentStart, Math::Vector3UVE capsuleSegmentEnd, float capsuleRadius,
    Math::Vector3UVE sphereCenter, float sphereRadius) noexcept;

/// Computes exact penetration between two capsules represented by explicit centerline segments.
/// The returned axis points from the first capsule toward the second. Touching pairs are not
/// intersections; coincident closest points use +X as the stable fallback axis. This copied
/// value-only helper does not model rotated collider ownership, transforms, ECS state, or backend
/// resources.
[[nodiscard]] std::optional<Math::PenetrationUVE> ComputeCapsuleCapsulePenetrationUVE(
    Math::Vector3UVE firstSegmentStart, Math::Vector3UVE firstSegmentEnd, float firstRadius,
    Math::Vector3UVE secondSegmentStart, Math::Vector3UVE secondSegmentEnd, float secondRadius) noexcept;

/// Computes exact sphere-vs-oriented-box penetration by transforming the sphere center into box
/// local space. The returned axis is in world space and points from the sphere toward the box.
/// Touching boundaries are not intersections; invalid rotations fail closed.
[[nodiscard]] std::optional<Math::PenetrationUVE> ComputeSphereOrientedBoxPenetrationUVE(
    Math::Vector3UVE boxCenter, Math::Vector3UVE boxHalfExtents, Math::QuaternionUVE boxRotation,
    Math::Vector3UVE sphereCenter, float sphereRadius) noexcept;

/// Computes exact capsule-vs-oriented-box penetration by transforming the capsule centerline into
/// box local space. The returned axis is in world space and points from the capsule toward the box.
/// Touching boundaries are not intersections; invalid rotations fail closed.
[[nodiscard]] std::optional<Math::PenetrationUVE> ComputeCapsuleOrientedBoxPenetrationUVE(
    Math::Vector3UVE boxCenter, Math::Vector3UVE boxHalfExtents, Math::QuaternionUVE boxRotation,
    Math::Vector3UVE capsuleSegmentStart, Math::Vector3UVE capsuleSegmentEnd, float capsuleRadius) noexcept;

/// Computes exact oriented-box-vs-oriented-box penetration using the 15-axis separating-axis
/// theorem. The returned axis points from the first box toward the second; touching and invalid
/// inputs are rejected, and coincident centers use deterministic axis ordering.
[[nodiscard]] std::optional<Math::PenetrationUVE> ComputeOrientedBoxOrientedBoxPenetrationUVE(
    Math::Vector3UVE firstCenter, Math::Vector3UVE firstHalfExtents, Math::QuaternionUVE firstRotation,
    Math::Vector3UVE secondCenter, Math::Vector3UVE secondHalfExtents,
    Math::QuaternionUVE secondRotation) noexcept;

} // namespace UVE::Physics::Detail

// EOF
