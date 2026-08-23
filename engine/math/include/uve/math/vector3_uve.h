// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <limits>
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

/// Component-wise subtraction. Added for Physics (Part 7.5, Increment 15) — see the class doc
/// comment; not needed by anything before it.
[[nodiscard]] constexpr Vector3UVE operator-(const Vector3UVE& lhs, const Vector3UVE& rhs) noexcept {
    return Vector3UVE{lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

/// Unary negation.
[[nodiscard]] constexpr Vector3UVE operator-(const Vector3UVE& v) noexcept {
    return Vector3UVE{-v.x, -v.y, -v.z};
}

/// Component-wise multiplication (used for scale composition, not a dot/cross product).
[[nodiscard]] constexpr Vector3UVE operator*(const Vector3UVE& lhs, const Vector3UVE& rhs) noexcept {
    return Vector3UVE{lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
}

/// Scalar multiplication.
[[nodiscard]] constexpr Vector3UVE operator*(const Vector3UVE& v, float scalar) noexcept {
    return Vector3UVE{v.x * scalar, v.y * scalar, v.z * scalar};
}

constexpr Vector3UVE& operator+=(Vector3UVE& lhs, const Vector3UVE& rhs) noexcept {
    lhs = lhs + rhs;
    return lhs;
}

constexpr Vector3UVE& operator-=(Vector3UVE& lhs, const Vector3UVE& rhs) noexcept {
    lhs = lhs - rhs;
    return lhs;
}

constexpr Vector3UVE& operator*=(Vector3UVE& v, float scalar) noexcept {
    v = v * scalar;
    return v;
}

[[nodiscard]] constexpr bool operator==(const Vector3UVE& lhs, const Vector3UVE& rhs) noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

[[nodiscard]] constexpr bool operator!=(const Vector3UVE& lhs, const Vector3UVE& rhs) noexcept {
    return !(lhs == rhs);
}

/// Dot product.
[[nodiscard]] constexpr float DotUVE(const Vector3UVE& lhs, const Vector3UVE& rhs) noexcept {
    const float floatResult = lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
    const auto IsFiniteFloatUVE = [](const float value) constexpr {
        return value == value && value <= std::numeric_limits<float>::max() &&
               value >= -std::numeric_limits<float>::max();
    };
    if (IsFiniteFloatUVE(floatResult)) {
        return floatResult;
    }
    return static_cast<float>(static_cast<double>(lhs.x) * static_cast<double>(rhs.x) +
                              static_cast<double>(lhs.y) * static_cast<double>(rhs.y) +
                              static_cast<double>(lhs.z) * static_cast<double>(rhs.z));
}

/// Cross product.
[[nodiscard]] constexpr Vector3UVE CrossUVE(const Vector3UVE& lhs, const Vector3UVE& rhs) noexcept {
    return Vector3UVE{
        lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x,
    };
}

/// Squared length — cheaper than LengthUVE() when only comparing magnitudes (no sqrt).
[[nodiscard]] constexpr float LengthSquaredUVE(const Vector3UVE& v) noexcept {
    return DotUVE(v, v);
}

/// Euclidean length. Non-constexpr: uses std::sqrt.
[[nodiscard]] float LengthUVE(const Vector3UVE& v) noexcept;

/// Returns `v` scaled to unit length. Contract: `v` must not be the zero vector (or within
/// floating-point epsilon of it) — callers that cannot guarantee this must check
/// `LengthSquaredUVE(v)` first; NormalizeUVE() does not itself guard against zero and scales
/// finite inputs before measuring length so large nonzero vectors do not overflow that intermediate
/// (matching this codebase's "don't validate what the caller must already ensure" convention for
/// value-type math, e.g. AabbUVE::TransformUVE assumes an affine matrix without checking).
[[nodiscard]] Vector3UVE NormalizeUVE(const Vector3UVE& v) noexcept;

/// Formats `vector` as `"(x, y, z)"`, for logging/debugging.
[[nodiscard]] std::string ToStringUVE(const Vector3UVE& vector);

} // namespace UVE::Math
