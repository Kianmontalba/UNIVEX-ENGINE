// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/math/vector3_uve.h"

#include <algorithm>
#include <cmath>

namespace UVE::Math {

float LengthUVE(const Vector3UVE& v) noexcept {
    return std::hypot(v.x, v.y, v.z);
}

Vector3UVE NormalizeUVE(const Vector3UVE& v) noexcept {
    const float scale = std::max(std::fabs(v.x), std::max(std::fabs(v.y), std::fabs(v.z)));
    if (scale == 0.0F || !std::isfinite(scale)) {
        return v * (1.0F / LengthUVE(v));
    }
    const Vector3UVE scaled{v.x / scale, v.y / scale, v.z / scale};
    return scaled * (1.0F / LengthUVE(scaled));
}

std::string ToStringUVE(const Vector3UVE& vector) {
    return "(" + std::to_string(vector.x) + ", " + std::to_string(vector.y) + ", " +
           std::to_string(vector.z) + ")";
}

} // namespace UVE::Math
