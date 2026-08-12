

#include "uve/math/quaternion_uve.h"

#include <cmath>
#include <limits>

namespace UVE::Math {

namespace {

constexpr float kMinimumQuaternionLengthSquaredUVE = std::numeric_limits<float>::epsilon();

[[nodiscard]] bool IsFiniteVectorUVE(const Vector3UVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
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

bool IsFiniteUVE(const QuaternionUVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) && std::isfinite(value.w);
}

float LengthSquaredUVE(const QuaternionUVE& value) noexcept {
    return value.x * value.x + value.y * value.y + value.z * value.z + value.w * value.w;
}

bool TryNormalizeUVE(const QuaternionUVE& value, QuaternionUVE& outNormalized) noexcept {
    if (!IsFiniteUVE(value)) {
        return false;
    }

    const float lengthSquared = LengthSquaredUVE(value);
    if (!std::isfinite(lengthSquared) || lengthSquared <= kMinimumQuaternionLengthSquaredUVE) {
        return false;
    }

    const float inverseLength = 1.0F / std::sqrt(lengthSquared);
    if (!std::isfinite(inverseLength)) {
        return false;
    }

    const QuaternionUVE candidate{
        value.x * inverseLength,
        value.y * inverseLength,
        value.z * inverseLength,
        value.w * inverseLength,
    };
    if (!IsFiniteUVE(candidate)) {
        return false;
    }

    outNormalized = candidate;
    return true;
}

bool TryInverseUVE(const QuaternionUVE& value, QuaternionUVE& outInverse) noexcept {
    if (!IsFiniteUVE(value)) {
        return false;
    }

    const float lengthSquared = LengthSquaredUVE(value);
    if (!std::isfinite(lengthSquared) || lengthSquared <= kMinimumQuaternionLengthSquaredUVE) {
        return false;
    }

    const float inverseLengthSquared = 1.0F / lengthSquared;
    if (!std::isfinite(inverseLengthSquared)) {
        return false;
    }

    const QuaternionUVE candidate{
        -value.x * inverseLengthSquared,
        -value.y * inverseLengthSquared,
        -value.z * inverseLengthSquared,
        value.w * inverseLengthSquared,
    };
    if (!IsFiniteUVE(candidate)) {
        return false;
    }

    outInverse = candidate;
    return true;
}

bool TryMakeAxisAngleUVE(const Vector3UVE& axis, const float radians, QuaternionUVE& outRotation) noexcept {
    if (!IsFiniteVectorUVE(axis) || !std::isfinite(radians)) {
        return false;
    }

    const float axisLengthSquared = axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;
    if (!std::isfinite(axisLengthSquared) || axisLengthSquared <= kMinimumQuaternionLengthSquaredUVE) {
        return false;
    }

    const float inverseAxisLength = 1.0F / std::sqrt(axisLengthSquared);
    const float halfAngle = radians * 0.5F;
    if (!std::isfinite(inverseAxisLength) || !std::isfinite(halfAngle)) {
        return false;
    }

    const float sine = std::sin(halfAngle);
    const float cosine = std::cos(halfAngle);
    if (!std::isfinite(sine) || !std::isfinite(cosine)) {
        return false;
    }

    const QuaternionUVE candidate{
        axis.x * inverseAxisLength * sine,
        axis.y * inverseAxisLength * sine,
        axis.z * inverseAxisLength * sine,
        cosine,
    };
    return TryNormalizeUVE(candidate, outRotation);
}

Vector3UVE RotateVectorUVE(const QuaternionUVE& rotation, const Vector3UVE& vector) noexcept {
    const Vector3UVE axis{rotation.x, rotation.y, rotation.z};
    const Vector3UVE twiceCross = CrossUVE(axis, vector) * 2.0F;
    return vector + twiceCross * rotation.w + CrossUVE(axis, twiceCross);
}

std::string ToStringUVE(const QuaternionUVE& rotation) {
    return "(" + std::to_string(rotation.x) + ", " + std::to_string(rotation.y) + ", " +
           std::to_string(rotation.z) + ", " + std::to_string(rotation.w) + ")";
}

} // namespace UVE::Math
