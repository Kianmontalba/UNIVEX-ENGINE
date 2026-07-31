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

} // namespace UVE::Math
