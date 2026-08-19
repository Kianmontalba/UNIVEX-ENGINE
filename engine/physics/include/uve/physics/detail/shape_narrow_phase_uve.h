// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <optional>

#include "uve/math/aabb_uve.h"

namespace UVE::Physics::Detail {

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

} // namespace UVE::Physics::Detail

// EOF
