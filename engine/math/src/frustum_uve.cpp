// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/math/frustum_uve.h"

#include <cmath>

namespace UVE::Math {

namespace {

using RowUVE = std::array<float, 4>;

[[nodiscard]] RowUVE CombineRowsUVE(const RowUVE& a, const RowUVE& b, float sign) noexcept {
    return RowUVE{a[0] + sign * b[0], a[1] + sign * b[1], a[2] + sign * b[2], a[3] + sign * b[3]};
}

/// Normalizes `coefficients` (an unnormalized `A*x + B*y + C*z + D` plane equation extracted
/// from a matrix row combination) into a PlaneUVE with unit normal, matching PlaneUVE's contract.
[[nodiscard]] PlaneUVE MakePlaneUVE(const RowUVE& coefficients) noexcept {
    const Vector3UVE normal{coefficients[0], coefficients[1], coefficients[2]};
    const float length = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    const float inverseLength = length > 0.0F ? 1.0F / length : 0.0F;
    return PlaneUVE{
        Vector3UVE{normal.x * inverseLength, normal.y * inverseLength, normal.z * inverseLength},
        coefficients[3] * inverseLength,
    };
}

} // namespace

FrustumUVE FrustumUVE::FromViewProjectionUVE(const Matrix4x4UVE& viewProjection) noexcept {
    const RowUVE row0{viewProjection.m[0][0], viewProjection.m[0][1], viewProjection.m[0][2], viewProjection.m[0][3]};
    const RowUVE row1{viewProjection.m[1][0], viewProjection.m[1][1], viewProjection.m[1][2], viewProjection.m[1][3]};
    const RowUVE row2{viewProjection.m[2][0], viewProjection.m[2][1], viewProjection.m[2][2], viewProjection.m[2][3]};
    const RowUVE row3{viewProjection.m[3][0], viewProjection.m[3][1], viewProjection.m[3][2], viewProjection.m[3][3]};

    FrustumUVE frustum;
    frustum.planes[0] = MakePlaneUVE(CombineRowsUVE(row3, row0, 1.0F));  // left:   w + x >= 0
    frustum.planes[1] = MakePlaneUVE(CombineRowsUVE(row3, row0, -1.0F)); // right:  w - x >= 0
    frustum.planes[2] = MakePlaneUVE(CombineRowsUVE(row3, row1, 1.0F));  // bottom: w + y >= 0
    frustum.planes[3] = MakePlaneUVE(CombineRowsUVE(row3, row1, -1.0F)); // top:    w - y >= 0
    frustum.planes[4] = MakePlaneUVE(row2);                              // near:   z >= 0 ([0,1] depth range)
    frustum.planes[5] = MakePlaneUVE(CombineRowsUVE(row3, row2, -1.0F)); // far:    w - z >= 0
    return frustum;
}

bool FrustumUVE::IntersectsUVE(const AabbUVE& box) const noexcept {
    const Vector3UVE center = box.GetCenterUVE();
    const Vector3UVE extents = box.GetExtentsUVE();
    for (const PlaneUVE& plane : planes) {
        const float radius = extents.x * std::abs(plane.normal.x) + extents.y * std::abs(plane.normal.y) +
                              extents.z * std::abs(plane.normal.z);
        if (plane.GetSignedDistanceUVE(center) + radius < 0.0F) {
            return false;
        }
    }
    return true;
}

} // namespace UVE::Math
