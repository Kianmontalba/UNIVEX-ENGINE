// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/physics/detail/shape_narrow_phase_uve.h"

#include <algorithm>
#include <cmath>

namespace UVE::Physics::Detail {
namespace {

[[nodiscard]] bool IsFiniteAabbUVE(const Math::AabbUVE& box) noexcept {
    return std::isfinite(box.min.x) && std::isfinite(box.min.y) && std::isfinite(box.min.z) &&
           std::isfinite(box.max.x) && std::isfinite(box.max.y) && std::isfinite(box.max.z) &&
           box.min.x < box.max.x && box.min.y < box.max.y && box.min.z < box.max.z;
}

} // namespace

std::optional<Math::PenetrationUVE> ComputeSphereAabbPenetrationUVE(
    const Math::AabbUVE& box, const Math::Vector3UVE sphereCenter, const float sphereRadius) noexcept {
    if (!IsFiniteAabbUVE(box) || !std::isfinite(sphereCenter.x) || !std::isfinite(sphereCenter.y) ||
        !std::isfinite(sphereCenter.z) || !std::isfinite(sphereRadius) || sphereRadius <= 0.0F) {
        return std::nullopt;
    }

    const Math::Vector3UVE closestPoint{
        std::clamp(sphereCenter.x, box.min.x, box.max.x),
        std::clamp(sphereCenter.y, box.min.y, box.max.y),
        std::clamp(sphereCenter.z, box.min.z, box.max.z),
    };
    const Math::Vector3UVE delta = closestPoint - sphereCenter;
    const float distanceSquared = Math::LengthSquaredUVE(delta);
    const float radiusSquared = sphereRadius * sphereRadius;
    if (distanceSquared > 0.0F) {
        if (distanceSquared >= radiusSquared) {
            return std::nullopt;
        }
        const float distance = std::sqrt(distanceSquared);
        return Math::PenetrationUVE{delta * (1.0F / distance), sphereRadius - distance};
    }

    const float distanceToMinX = sphereCenter.x - box.min.x;
    const float distanceToMaxX = box.max.x - sphereCenter.x;
    const float distanceToMinY = sphereCenter.y - box.min.y;
    const float distanceToMaxY = box.max.y - sphereCenter.y;
    const float distanceToMinZ = sphereCenter.z - box.min.z;
    const float distanceToMaxZ = box.max.z - sphereCenter.z;

    float nearestDistance = distanceToMinX;
    Math::Vector3UVE axis{1.0F, 0.0F, 0.0F};
    if (distanceToMaxX < nearestDistance) {
        nearestDistance = distanceToMaxX;
        axis = {-1.0F, 0.0F, 0.0F};
    }
    if (distanceToMinY < nearestDistance) {
        nearestDistance = distanceToMinY;
        axis = {0.0F, 1.0F, 0.0F};
    }
    if (distanceToMaxY < nearestDistance) {
        nearestDistance = distanceToMaxY;
        axis = {0.0F, -1.0F, 0.0F};
    }
    if (distanceToMinZ < nearestDistance) {
        nearestDistance = distanceToMinZ;
        axis = {0.0F, 0.0F, 1.0F};
    }
    if (distanceToMaxZ < nearestDistance) {
        nearestDistance = distanceToMaxZ;
        axis = {0.0F, 0.0F, -1.0F};
    }
    return Math::PenetrationUVE{axis, sphereRadius + nearestDistance};
}

} // namespace UVE::Physics::Detail

// EOF
