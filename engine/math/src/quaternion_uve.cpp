//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/math/quaternion_uve.h"

namespace UVE::Math {

namespace {

/// Cross product, kept file-local: Vector3UVE deliberately doesn't expose this publicly (out of
/// scope for the minimal math surface this increment needs), but rotating a vector by a
/// quaternion requires it internally.
[[nodiscard]] Vector3UVE CrossUVE(const Vector3UVE& lhs, const Vector3UVE& rhs) noexcept {
    return Vector3UVE{lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z,
                       lhs.x * rhs.y - lhs.y * rhs.x};
}

[[nodiscard]] Vector3UVE ScaleUVE(const Vector3UVE& vector, float scalar) noexcept {
    return Vector3UVE{vector.x * scalar, vector.y * scalar, vector.z * scalar};
}

} // namespace

QuaternionUVE MultiplyUVE(const QuaternionUVE& lhs, const QuaternionUVE& rhs) noexcept {
    return QuaternionUVE{
        lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.w * rhs.y - lhs.x * rhs.z + lhs.y * rhs.w + lhs.z * rhs.x,
        lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x + lhs.z * rhs.w,
        lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z,
    };
}

Vector3UVE RotateVectorUVE(const QuaternionUVE& rotation, const Vector3UVE& vector) noexcept {
    const Vector3UVE axis{rotation.x, rotation.y, rotation.z};
    const Vector3UVE twiceCross = ScaleUVE(CrossUVE(axis, vector), 2.0F);
    return vector + ScaleUVE(twiceCross, rotation.w) + CrossUVE(axis, twiceCross);
}

std::string ToStringUVE(const QuaternionUVE& rotation) {
    return "(" + std::to_string(rotation.x) + ", " + std::to_string(rotation.y) + ", " +
           std::to_string(rotation.z) + ", " + std::to_string(rotation.w) + ")";
}

} // namespace UVE::Math
