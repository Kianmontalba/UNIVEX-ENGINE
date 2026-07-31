//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <string>

namespace UVE::Math {

/// A 3-component single-precision vector, used engine-wide for position, scale, and any other
/// three-float quantity. Deliberately minimal: only the operations Scene/ECS's transform
/// composition actually needs today (addition, component-wise multiplication for scale
/// composition, equality). A fuller math library (Vector2UVE/Vector4UVE, cross/dot products,
/// SIMD optimization, swizzles) is a real design problem for whichever future increment
/// (Rendering, Physics) first needs it — not invented here.
/// Thread-safety: value type; safe to copy/pass freely, no shared state.
struct Vector3UVE {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};

/// Component-wise addition.
[[nodiscard]] constexpr Vector3UVE operator+(const Vector3UVE& lhs, const Vector3UVE& rhs) noexcept {
    return Vector3UVE{lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

/// Component-wise multiplication (used for scale composition, not a dot/cross product).
[[nodiscard]] constexpr Vector3UVE operator*(const Vector3UVE& lhs, const Vector3UVE& rhs) noexcept {
    return Vector3UVE{lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
}

[[nodiscard]] constexpr bool operator==(const Vector3UVE& lhs, const Vector3UVE& rhs) noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

[[nodiscard]] constexpr bool operator!=(const Vector3UVE& lhs, const Vector3UVE& rhs) noexcept {
    return !(lhs == rhs);
}

/// Formats `vector` as `"(x, y, z)"`, for logging/debugging.
[[nodiscard]] std::string ToStringUVE(const Vector3UVE& vector);

} // namespace UVE::Math
