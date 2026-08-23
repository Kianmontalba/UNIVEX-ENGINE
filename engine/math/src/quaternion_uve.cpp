

#include "uve/math/quaternion_uve.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace UVE::Math {

namespace {

constexpr float kMinimumQuaternionLengthSquaredUVE = std::numeric_limits<float>::epsilon();

[[nodiscard]] bool IsFiniteVectorUVE(const Vector3UVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] float LargestAbsoluteComponentUVE(const QuaternionUVE& value) noexcept {
    return std::max(std::fabs(value.x), std::max(std::fabs(value.y), std::max(std::fabs(value.z), std::fabs(value.w))));
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
    if (std::isfinite(lengthSquared)) {
        if (lengthSquared <= kMinimumQuaternionLengthSquaredUVE) {
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

    const float scale = LargestAbsoluteComponentUVE(value);
    if (!std::isfinite(scale) || scale <= 0.0F) {
        return false;
    }
    const double scaledX = static_cast<double>(value.x) / static_cast<double>(scale);
    const double scaledY = static_cast<double>(value.y) / static_cast<double>(scale);
    const double scaledZ = static_cast<double>(value.z) / static_cast<double>(scale);
    const double scaledW = static_cast<double>(value.w) / static_cast<double>(scale);
    const double scaledLengthSquared = scaledX * scaledX + scaledY * scaledY + scaledZ * scaledZ + scaledW * scaledW;
    const double inverseScaledLength = 1.0 / std::sqrt(scaledLengthSquared);
    if (!std::isfinite(inverseScaledLength)) {
        return false;
    }
    const QuaternionUVE candidate{
        static_cast<float>(scaledX * inverseScaledLength),
        static_cast<float>(scaledY * inverseScaledLength),
        static_cast<float>(scaledZ * inverseScaledLength),
        static_cast<float>(scaledW * inverseScaledLength),
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
    if (std::isfinite(lengthSquared)) {
        if (lengthSquared <= kMinimumQuaternionLengthSquaredUVE) {
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

    const float scale = LargestAbsoluteComponentUVE(value);
    if (!std::isfinite(scale) || scale <= 0.0F) {
        return false;
    }
    const double scaledX = static_cast<double>(value.x) / static_cast<double>(scale);
    const double scaledY = static_cast<double>(value.y) / static_cast<double>(scale);
    const double scaledZ = static_cast<double>(value.z) / static_cast<double>(scale);
    const double scaledW = static_cast<double>(value.w) / static_cast<double>(scale);
    const double scaledLengthSquared = scaledX * scaledX + scaledY * scaledY + scaledZ * scaledZ + scaledW * scaledW;
    const double inverseLengthSquared = 1.0 / (static_cast<double>(scale) * static_cast<double>(scale) * scaledLengthSquared);
    if (!std::isfinite(inverseLengthSquared)) {
        return false;
    }
    const QuaternionUVE candidate{
        static_cast<float>(-static_cast<double>(value.x) * inverseLengthSquared),
        static_cast<float>(-static_cast<double>(value.y) * inverseLengthSquared),
        static_cast<float>(-static_cast<double>(value.z) * inverseLengthSquared),
        static_cast<float>(static_cast<double>(value.w) * inverseLengthSquared),
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

bool TryMakeEulerUVE(const Vector3UVE& radians, QuaternionUVE& outRotation) noexcept {
    if (!IsFiniteVectorUVE(radians)) {
        return false;
    }
    const Vector3UVE half = radians * 0.5F;
    const float sx = std::sin(half.x);
    const float cx = std::cos(half.x);
    const float sy = std::sin(half.y);
    const float cy = std::cos(half.y);
    const float sz = std::sin(half.z);
    const float cz = std::cos(half.z);
    if (!std::isfinite(sx) || !std::isfinite(cx) || !std::isfinite(sy) || !std::isfinite(cy) ||
        !std::isfinite(sz) || !std::isfinite(cz)) {
        return false;
    }
    return TryNormalizeUVE(QuaternionUVE{
        sx * cy * cz - cx * sy * sz,
        cx * sy * cz + sx * cy * sz,
        cx * cy * sz - sx * sy * cz,
        cx * cy * cz + sx * sy * sz,
    }, outRotation);
}

bool TryMakeLookAtUVE(const Vector3UVE& direction, const Vector3UVE& up,
                      QuaternionUVE& outRotation) noexcept {
    if (!IsFiniteVectorUVE(direction) || !IsFiniteVectorUVE(up) ||
        LengthSquaredUVE(direction) <= kMinimumQuaternionLengthSquaredUVE ||
        LengthSquaredUVE(up) <= kMinimumQuaternionLengthSquaredUVE) {
        return false;
    }
    const Vector3UVE forward = NormalizeUVE(direction);
    const Vector3UVE rightUnnormalized = CrossUVE(up, forward);
    if (!IsFiniteVectorUVE(rightUnnormalized) ||
        LengthSquaredUVE(rightUnnormalized) <= kMinimumQuaternionLengthSquaredUVE) {
        return false;
    }
    const Vector3UVE right = NormalizeUVE(rightUnnormalized);
    const Vector3UVE correctedUp = CrossUVE(forward, right);
    const float trace = right.x + correctedUp.y + forward.z;
    QuaternionUVE candidate{};
    if (trace > 0.0F) {
        const float scale = std::sqrt(trace + 1.0F) * 2.0F;
        if (!std::isfinite(scale) || scale <= std::numeric_limits<float>::epsilon()) return false;
        candidate = QuaternionUVE{(correctedUp.z - forward.y) / scale, (forward.x - right.z) / scale,
                                  (right.y - correctedUp.x) / scale, scale * 0.25F};
    } else if (right.x > correctedUp.y && right.x > forward.z) {
        const float scale = std::sqrt(1.0F + right.x - correctedUp.y - forward.z) * 2.0F;
        if (!std::isfinite(scale) || scale <= std::numeric_limits<float>::epsilon()) return false;
        candidate = QuaternionUVE{scale * 0.25F, (right.y + correctedUp.x) / scale,
                                  (right.z + forward.x) / scale, (correctedUp.z - forward.y) / scale};
    } else if (correctedUp.y > forward.z) {
        const float scale = std::sqrt(1.0F + correctedUp.y - right.x - forward.z) * 2.0F;
        if (!std::isfinite(scale) || scale <= std::numeric_limits<float>::epsilon()) return false;
        candidate = QuaternionUVE{(right.y + correctedUp.x) / scale, scale * 0.25F,
                                  (correctedUp.z + forward.y) / scale, (forward.x - right.z) / scale};
    } else {
        const float scale = std::sqrt(1.0F + forward.z - right.x - correctedUp.y) * 2.0F;
        if (!std::isfinite(scale) || scale <= std::numeric_limits<float>::epsilon()) return false;
        candidate = QuaternionUVE{(right.z + forward.x) / scale, (correctedUp.z + forward.y) / scale,
                                  scale * 0.25F, (right.y - correctedUp.x) / scale};
    }
    return TryNormalizeUVE(candidate, outRotation);
}

bool TrySlerpUVE(const QuaternionUVE& lhs, const QuaternionUVE& rhs, const float alpha,
                 QuaternionUVE& outRotation) noexcept {
    if (!IsFiniteUVE(lhs) || !IsFiniteUVE(rhs) || !std::isfinite(alpha)) return false;
    QuaternionUVE a{};
    QuaternionUVE b{};
    if (!TryNormalizeUVE(lhs, a) || !TryNormalizeUVE(rhs, b)) return false;
    float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    if (dot < 0.0F) {
        b = QuaternionUVE{-b.x, -b.y, -b.z, -b.w};
        dot = -dot;
    }
    dot = std::clamp(dot, -1.0F, 1.0F);
    QuaternionUVE candidate{};
    if (dot > 0.9995F) {
        candidate = QuaternionUVE{a.x + alpha * (b.x - a.x), a.y + alpha * (b.y - a.y),
                                  a.z + alpha * (b.z - a.z), a.w + alpha * (b.w - a.w)};
    } else {
        const float theta = std::acos(dot);
        const float sineTheta = std::sin(theta);
        if (!std::isfinite(theta) || !std::isfinite(sineTheta) || std::fabs(sineTheta) <= 1.0e-6F) return false;
        const float leftWeight = std::sin((1.0F - alpha) * theta) / sineTheta;
        const float rightWeight = std::sin(alpha * theta) / sineTheta;
        candidate = QuaternionUVE{leftWeight * a.x + rightWeight * b.x,
                                  leftWeight * a.y + rightWeight * b.y,
                                  leftWeight * a.z + rightWeight * b.z,
                                  leftWeight * a.w + rightWeight * b.w};
    }
    return TryNormalizeUVE(candidate, outRotation);
}

bool TryToAxisAngleUVE(const QuaternionUVE& rotation, Vector3UVE& outAxis,
                       float& outRadians) noexcept {
    QuaternionUVE normalized{};
    if (!TryNormalizeUVE(rotation, normalized)) return false;
    const float clampedW = std::clamp(normalized.w, -1.0F, 1.0F);
    const float radians = 2.0F * std::acos(clampedW);
    const float sine = std::sqrt(std::max(0.0F, 1.0F - clampedW * clampedW));
    if (!std::isfinite(radians) || !std::isfinite(sine)) return false;
    Vector3UVE axis{1.0F, 0.0F, 0.0F};
    if (sine > 1.0e-5F) {
        axis = Vector3UVE{normalized.x / sine, normalized.y / sine, normalized.z / sine};
    }
    if (!IsFiniteVectorUVE(axis)) return false;
    outAxis = axis;
    outRadians = radians;
    return true;
}

std::string ToStringUVE(const QuaternionUVE& rotation) {
    return "(" + std::to_string(rotation.x) + ", " + std::to_string(rotation.y) + ", " +
           std::to_string(rotation.z) + ", " + std::to_string(rotation.w) + ")";
}

} // namespace UVE::Math
