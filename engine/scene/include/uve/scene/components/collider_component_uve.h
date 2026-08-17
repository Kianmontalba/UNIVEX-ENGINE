// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cmath>
#include <cstdint>

#include "uve/math/vector3_uve.h"

namespace UVE::Scene {

/// One of the master spec's named built-in components (Part 7.3). Increment 15
/// (CollisionSystemUVE, Part 7.5) keeps the shape itself minimal — an axis-aligned box
/// half-extent; a real shape-type enum (box/sphere/capsule/mesh) is future work, not invented
/// ahead of a second shape actually existing. `collisionLayer` is consulted by RaycastSystemUVE
/// (Increment 16) via a query's layer mask; `collisionMask` remains reserved — present so a
/// future collider-vs-collider filtering feature never has to make a breaking change to this
/// component's serialized shape, but CollisionSystemUVE::DetectCollisionsUVE() does not yet
/// consult it. `friction`/`restitution` (Increment 16) are consumed by PhysicsSystemUVE's
/// collision resolution; `density` remains reserved (captured for spec completeness, not yet
/// wired to mass).
struct ColliderComponentUVE final {
    Math::Vector3UVE halfExtents{0.5F, 0.5F, 0.5F};
    std::uint32_t collisionLayer = 1;
    std::uint32_t collisionMask = 0xFFFFFFFFU;
    float friction = 0.0F;
    float restitution = 0.0F;
    float density = 1.0F;
};

/// Validates the value-only collider contract before scene persistence and explicit validation
/// consumers. Collision masks remain opaque bit fields; a zero layer is rejected because it would
/// make the collider unreachable by the layer-mask raycast contract. Runtime material extraction
/// retains its established defensive friction/restitution clamp for legacy hand-authored values.
[[nodiscard]] inline bool IsColliderComponentValidUVE(const ColliderComponentUVE& collider) noexcept {
    return std::isfinite(collider.halfExtents.x) && std::isfinite(collider.halfExtents.y) &&
           std::isfinite(collider.halfExtents.z) && collider.halfExtents.x > 0.0F &&
           collider.halfExtents.y > 0.0F && collider.halfExtents.z > 0.0F && collider.collisionLayer != 0U &&
           std::isfinite(collider.friction) && collider.friction >= 0.0F && collider.friction <= 1.0F &&
           std::isfinite(collider.restitution) && collider.restitution >= 0.0F && collider.restitution <= 1.0F &&
           std::isfinite(collider.density) && collider.density > 0.0F;
}

} // namespace UVE::Scene
