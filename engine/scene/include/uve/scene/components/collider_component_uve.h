// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "uve/math/vector3_uve.h"

namespace UVE::Scene {

/// Supported authored collider descriptors. Box remains the legacy default; sphere and capsule
/// are represented here for validation and conservative world-AABB broad-phase bounds. Exact
/// shape narrow phases remain a separate future contract.
enum class ColliderShapeTypeUVE : std::uint8_t {
    Box = 0,
    Sphere = 1,
    Capsule = 2,
};

/// One of the master spec's named built-in components. The original halfExtents/layer/material
/// fields remain in their established order so existing aggregate initializers stay source
/// compatible; shape fields are trailing defaults. Capsule height is the total end-to-end height
/// along the local Y axis, including both hemispherical caps.
struct ColliderComponentUVE final {
    Math::Vector3UVE halfExtents{0.5F, 0.5F, 0.5F};
    std::uint32_t collisionLayer = 1;
    std::uint32_t collisionMask = 0xFFFFFFFFU;
    float friction = 0.0F;
    float restitution = 0.0F;
    float density = 1.0F;
    ColliderShapeTypeUVE shapeType = ColliderShapeTypeUVE::Box;
    float radius = 0.5F;
    float height = 1.0F;
};

/// Returns conservative local half-extents for the supported descriptors. Collision and raycast
/// systems intentionally consume these as broad-phase AABBs in v1; callers that need exact sphere
/// or capsule contacts must not infer them from this helper.
[[nodiscard]] inline Math::Vector3UVE GetColliderLocalHalfExtentsUVE(
    const ColliderComponentUVE& collider) noexcept {
    switch (collider.shapeType) {
    case ColliderShapeTypeUVE::Sphere:
        return {collider.radius, collider.radius, collider.radius};
    case ColliderShapeTypeUVE::Capsule:
        return {collider.radius, collider.height * 0.5F, collider.radius};
    case ColliderShapeTypeUVE::Box:
    default:
        return collider.halfExtents;
    }
}

/// Validates the value-only collider contract before scene persistence and explicit validation
/// consumers. Collision masks remain opaque bit fields; a zero layer is rejected because it would
/// make the collider unreachable by the layer-mask raycast contract. Runtime material extraction
/// retains its established defensive friction/restitution clamp for legacy hand-authored values.
[[nodiscard]] inline bool IsColliderComponentValidUVE(const ColliderComponentUVE& collider) noexcept {
    const bool validCommon = std::isfinite(collider.halfExtents.x) && std::isfinite(collider.halfExtents.y) &&
                              std::isfinite(collider.halfExtents.z) && collider.halfExtents.x > 0.0F &&
                              collider.halfExtents.y > 0.0F && collider.halfExtents.z > 0.0F &&
                              collider.collisionLayer != 0U && std::isfinite(collider.friction) &&
                              collider.friction >= 0.0F && collider.friction <= 1.0F &&
                              std::isfinite(collider.restitution) && collider.restitution >= 0.0F &&
                              collider.restitution <= 1.0F && std::isfinite(collider.density) &&
                              collider.density > 0.0F;
    if (!validCommon) {
        return false;
    }
    switch (collider.shapeType) {
    case ColliderShapeTypeUVE::Box:
        return true;
    case ColliderShapeTypeUVE::Sphere:
        return std::isfinite(collider.radius) && collider.radius > 0.0F;
    case ColliderShapeTypeUVE::Capsule:
        return std::isfinite(collider.radius) && collider.radius > 0.0F &&
               std::isfinite(collider.height) && collider.height >= 2.0F * collider.radius;
    default:
        return false;
    }
}

} // namespace UVE::Scene
