//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/math/vector3_uve.h"

#include <cmath>

namespace UVE::Math {

float LengthUVE(const Vector3UVE& v) noexcept {
    return std::sqrt(LengthSquaredUVE(v));
}

Vector3UVE NormalizeUVE(const Vector3UVE& v) noexcept {
    return v * (1.0F / LengthUVE(v));
}

std::string ToStringUVE(const Vector3UVE& vector) {
    return "(" + std::to_string(vector.x) + ", " + std::to_string(vector.y) + ", " +
           std::to_string(vector.z) + ")";
}

} // namespace UVE::Math
