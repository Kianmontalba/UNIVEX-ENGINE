//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/math/aabb_uve.h"

#include <algorithm>
#include <array>

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

std::optional<RayHitUVE> IntersectRayUVE(const RayUVE& ray, const AabbUVE& aabb, float maxDistance) noexcept {
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

        float tNear = (boxMin[axis] - origin[axis]) / direction[axis];
        float tFar = (boxMax[axis] - origin[axis]) / direction[axis];
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

    return RayHitUVE{tmin, normal};
}

} // namespace UVE::Math
