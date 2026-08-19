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

} // namespace UVE::Physics::Detail

// EOF
