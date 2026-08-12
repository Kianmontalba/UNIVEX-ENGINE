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

/// Returns whether every quaternion component is finite.
[[nodiscard]] bool IsFiniteUVE(const QuaternionUVE& value) noexcept;

/// Returns the squared quaternion magnitude. Callers requiring a unit rotation should use
/// TryNormalizeUVE() rather than assuming a non-zero result.
[[nodiscard]] float LengthSquaredUVE(const QuaternionUVE& value) noexcept;

/// Normalizes `value` into `outNormalized`. Returns false and does not modify `outNormalized`
/// when the input is non-finite or has an effectively zero magnitude.
[[nodiscard]] bool TryNormalizeUVE(const QuaternionUVE& value, QuaternionUVE& outNormalized) noexcept;

/// Computes the inverse into `outInverse`. Returns false and does not modify `outInverse` for a
/// non-finite or effectively zero-magnitude input.
[[nodiscard]] bool TryInverseUVE(const QuaternionUVE& value, QuaternionUVE& outInverse) noexcept;

/// Builds a normalized axis-angle rotation around `axis` by `radians`. Returns false without
/// modifying `outRotation` when the axis or angle is non-finite or the axis has zero magnitude.
[[nodiscard]] bool TryMakeAxisAngleUVE(const Vector3UVE& axis, float radians,
                                        QuaternionUVE& outRotation) noexcept;

/// Rotates `vector` by `rotation`.
[[nodiscard]] Vector3UVE RotateVectorUVE(const QuaternionUVE& rotation, const Vector3UVE& vector) noexcept;

/// Formats `rotation` as `"(x, y, z, w)"`, for logging/debugging.
[[nodiscard]] std::string ToStringUVE(const QuaternionUVE& rotation);

} // namespace UVE::Math
