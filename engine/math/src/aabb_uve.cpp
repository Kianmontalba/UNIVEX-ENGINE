// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/math/aabb_uve.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace UVE::Math {

AabbUVE AabbUVE::TransformUVE(const Matrix4x4UVE& matrix) const noexcept {
    const std::array<Vector3UVE, 8> corners = {
        Vector3UVE{min.x, min.y, min.z}, Vector3UVE{max.x, min.y, min.z}, Vector3UVE{min.x, max.y, min.z},
        Vector3UVE{max.x, max.y, min.z}, Vector3UVE{min.x, min.y, max.z}, Vector3UVE{max.x, min.y, max.z},
        Vector3UVE{min.x, max.y, max.z}, Vector3UVE{max.x, max.y, max.z},
    };

    Vector3UVE newMin = TransformPointUVE(matrix, corners[0]);
    Vector3UVE newMax = newMin;
    for (std::size_t index = 1; index < corners.size(); ++index) {
        const Vector3UVE transformed = TransformPointUVE(matrix, corners[index]);
        newMin.x = std::min(newMin.x, transformed.x);
        newMin.y = std::min(newMin.y, transformed.y);
        newMin.z = std::min(newMin.z, transformed.z);
        newMax.x = std::max(newMax.x, transformed.x);
        newMax.y = std::max(newMax.y, transformed.y);
        newMax.z = std::max(newMax.z, transformed.z);
    }
    return AabbUVE{newMin, newMax};
}

std::string ToStringUVE(const AabbUVE& box) {
    return "[" + ToStringUVE(box.min) + " .. " + ToStringUVE(box.max) + "]";
}

std::optional<PenetrationUVE> ComputePenetrationUVE(const AabbUVE& a, const AabbUVE& b) noexcept {
    if (!a.IntersectsUVE(b)) {
        return std::nullopt;
    }

    const float overlapX = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
    const float overlapY = std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y);
    const float overlapZ = std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z);
    const Vector3UVE centerA = a.GetCenterUVE();
    const Vector3UVE centerB = b.GetCenterUVE();

    if (overlapX <= overlapY && overlapX <= overlapZ) {
        return PenetrationUVE{Vector3UVE{centerB.x >= centerA.x ? 1.0F : -1.0F, 0.0F, 0.0F}, overlapX};
    }
    if (overlapY <= overlapZ) {
        return PenetrationUVE{Vector3UVE{0.0F, centerB.y >= centerA.y ? 1.0F : -1.0F, 0.0F}, overlapY};
    }
    return PenetrationUVE{Vector3UVE{0.0F, 0.0F, centerB.z >= centerA.z ? 1.0F : -1.0F}, overlapZ};
}

std::optional<SweptAabbHitUVE> SweepAabbUVE(const AabbUVE& moving, const Vector3UVE& displacement,
                                                   const AabbUVE& target) noexcept {
    const auto IsFiniteVectorUVE = [](const Vector3UVE& value) noexcept {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    };
    if (!IsFiniteVectorUVE(moving.min) || !IsFiniteVectorUVE(moving.max) ||
        !IsFiniteVectorUVE(displacement) || !IsFiniteVectorUVE(target.min) ||
        !IsFiniteVectorUVE(target.max) || moving.IntersectsUVE(target)) {
        return std::nullopt;
    }

    const Vector3UVE movingExtents = moving.GetExtentsUVE();
    if (!IsFiniteVectorUVE(movingExtents) || movingExtents.x < 0.0F || movingExtents.y < 0.0F ||
        movingExtents.z < 0.0F) {
        return std::nullopt;
    }
    const AabbUVE expandedTarget{
        Vector3UVE{target.min.x - movingExtents.x, target.min.y - movingExtents.y,
                   target.min.z - movingExtents.z},
        Vector3UVE{target.max.x + movingExtents.x, target.max.y + movingExtents.y,
                   target.max.z + movingExtents.z}};
    const Vector3UVE origin = moving.GetCenterUVE();
    const std::array<float, 3> origins{origin.x, origin.y, origin.z};
    const std::array<float, 3> directions{displacement.x, displacement.y, displacement.z};
    const std::array<float, 3> boxMin{expandedTarget.min.x, expandedTarget.min.y, expandedTarget.min.z};
    const std::array<float, 3> boxMax{expandedTarget.max.x, expandedTarget.max.y, expandedTarget.max.z};

    float entryTime = 0.0F;
    float exitTime = 1.0F;
    Vector3UVE normal{};
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
        if (directions[axis] == 0.0F) {
            if (origins[axis] < boxMin[axis] || origins[axis] > boxMax[axis]) {
                return std::nullopt;
            }
            continue;
        }
        float nearTime = (boxMin[axis] - origins[axis]) / directions[axis];
        float farTime = (boxMax[axis] - origins[axis]) / directions[axis];
        if (nearTime > farTime) {
            std::swap(nearTime, farTime);
        }
        if (nearTime > entryTime || (nearTime == entryTime && normal == Vector3UVE{})) {
            entryTime = nearTime;
            normal = Vector3UVE{};
            const float directionSign = directions[axis] > 0.0F ? 1.0F : -1.0F;
            if (axis == 0U) {
                normal.x = directionSign;
            } else if (axis == 1U) {
                normal.y = directionSign;
            } else {
                normal.z = directionSign;
            }
        }
        exitTime = std::min(exitTime, farTime);
        if (entryTime > exitTime) {
            return std::nullopt;
        }
    }
    if (entryTime < 0.0F || entryTime > 1.0F || normal == Vector3UVE{}) {
        return std::nullopt;
    }
    return SweptAabbHitUVE{entryTime, normal};
}

std::optional<RayHitUVE> IntersectRayUVE(const RayUVE& ray, const AabbUVE& aabb, float maxDistance) noexcept {
    const auto IsFiniteVectorUVE = [](const Vector3UVE& value) noexcept {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    };
    if (!IsFiniteVectorUVE(ray.origin) || !IsFiniteVectorUVE(ray.direction) ||
        !IsFiniteVectorUVE(aabb.min) || !IsFiniteVectorUVE(aabb.max) || !std::isfinite(maxDistance) ||
        maxDistance < 0.0F || aabb.min.x > aabb.max.x || aabb.min.y > aabb.max.y || aabb.min.z > aabb.max.z) {
        return std::nullopt;
    }

    const std::array<float, 3> origin{ray.origin.x, ray.origin.y, ray.origin.z};
    const std::array<float, 3> direction{ray.direction.x, ray.direction.y, ray.direction.z};
    const std::array<float, 3> boxMin{aabb.min.x, aabb.min.y, aabb.min.z};
    const std::array<float, 3> boxMax{aabb.max.x, aabb.max.y, aabb.max.z};

    float tmin = 0.0F;
    float tmax = maxDistance;
    Vector3UVE normal{0.0F, 0.0F, 0.0F};

    for (std::size_t axis = 0; axis < 3; ++axis) {
        if (direction[axis] == 0.0F) {
            if (origin[axis] < boxMin[axis] || origin[axis] > boxMax[axis]) {
                return std::nullopt;
            }
            continue;
        }

        const float nearNumerator = boxMin[axis] - origin[axis];
        const float farNumerator = boxMax[axis] - origin[axis];
        if (!std::isfinite(nearNumerator) || !std::isfinite(farNumerator)) {
            return std::nullopt;
        }
        float tNear = nearNumerator / direction[axis];
        float tFar = farNumerator / direction[axis];
        if (!std::isfinite(tNear) || !std::isfinite(tFar)) {
            return std::nullopt;
        }
        float nearSign = -1.0F;
        if (tNear > tFar) {
            std::swap(tNear, tFar);
            nearSign = 1.0F;
        }

        if (tNear > tmin) {
            tmin = tNear;
            normal = Vector3UVE{0.0F, 0.0F, 0.0F};
            if (axis == 0) {
                normal.x = nearSign;
            } else if (axis == 1) {
                normal.y = nearSign;
            } else {
                normal.z = nearSign;
            }
        }
        tmax = std::min(tmax, tFar);
        if (tmin > tmax) {
            return std::nullopt;
        }
    }

    if (!std::isfinite(tmin) || !IsFiniteVectorUVE(normal)) {
        return std::nullopt;
    }
    return RayHitUVE{tmin, normal};
}

} // namespace UVE::Math
