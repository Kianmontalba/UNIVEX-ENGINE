// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <string>

#include "uve/math/vector3_uve.h"

namespace UVE::Math {

/// A unit quaternion used for 3D rotation. Default-constructs to the identity rotation
/// (x=y=z=0, w=1) — deliberately minimal, alongside Vector3UVE: no SLERP, no Euler-angle
/// conversion, no matrix conversion. Those are real design problems for whichever future
/// increment (Rendering, Physics, Animation) first needs them.
/// Thread-safety: value type; safe to copy/pass freely, no shared state.
struct QuaternionUVE {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
    float w = 1.0F;
};

[[nodiscard]] constexpr bool operator==(const QuaternionUVE& lhs, const QuaternionUVE& rhs) noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

[[nodiscard]] constexpr bool operator!=(const QuaternionUVE& lhs, const QuaternionUVE& rhs) noexcept {
    return !(lhs == rhs);
}

/// Hamilton product `lhs * rhs`: the rotation "rhs applied first, then lhs" — i.e.
/// `RotateVectorUVE(MultiplyUVE(lhs, rhs), v) == RotateVectorUVE(lhs, RotateVectorUVE(rhs, v))`.
[[nodiscard]] QuaternionUVE MultiplyUVE(const QuaternionUVE& lhs, const QuaternionUVE& rhs) noexcept;

/// Rotates `vector` by `rotation`.
[[nodiscard]] Vector3UVE RotateVectorUVE(const QuaternionUVE& rotation, const Vector3UVE& vector) noexcept;

/// Formats `rotation` as `"(x, y, z, w)"`, for logging/debugging.
[[nodiscard]] std::string ToStringUVE(const QuaternionUVE& rotation);

} // namespace UVE::Math
