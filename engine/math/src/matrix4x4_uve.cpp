// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/math/matrix4x4_uve.h"

#include <cmath>
#include <utility>

namespace UVE::Math {

namespace {

[[nodiscard]] Vector3UVE ScaleVectorUVE(const Vector3UVE& vector, float scalar) noexcept {
    return Vector3UVE{vector.x * scalar, vector.y * scalar, vector.z * scalar};
}

[[nodiscard]] float DotWideUVE(const Vector3UVE& lhs, const Vector3UVE& rhs) noexcept {
    const double sum = static_cast<double>(lhs.x) * static_cast<double>(rhs.x) +
                       static_cast<double>(lhs.y) * static_cast<double>(rhs.y) +
                       static_cast<double>(lhs.z) * static_cast<double>(rhs.z);
    return static_cast<float>(sum);
}

} // namespace

Matrix4x4UVE Matrix4x4UVE::ComposeTrsUVE(Vector3UVE translation, QuaternionUVE rotation, Vector3UVE scale) noexcept {
    const Vector3UVE basisX = ScaleVectorUVE(RotateVectorUVE(rotation, Vector3UVE{1.0F, 0.0F, 0.0F}), scale.x);
    const Vector3UVE basisY = ScaleVectorUVE(RotateVectorUVE(rotation, Vector3UVE{0.0F, 1.0F, 0.0F}), scale.y);
    const Vector3UVE basisZ = ScaleVectorUVE(RotateVectorUVE(rotation, Vector3UVE{0.0F, 0.0F, 1.0F}), scale.z);

    Matrix4x4UVE result = IdentityUVE();
    result.m[0][0] = basisX.x;
    result.m[1][0] = basisX.y;
    result.m[2][0] = basisX.z;
    result.m[0][1] = basisY.x;
    result.m[1][1] = basisY.y;
    result.m[2][1] = basisY.z;
    result.m[0][2] = basisZ.x;
    result.m[1][2] = basisZ.y;
    result.m[2][2] = basisZ.z;
    result.m[0][3] = translation.x;
    result.m[1][3] = translation.y;
    result.m[2][3] = translation.z;
    return result;
}

Matrix4x4UVE Matrix4x4UVE::PerspectiveUVE(float fovYRadians, float aspectRatio, float nearPlane,
                                           float farPlane) noexcept {
    const float focalLength = 1.0F / std::tan(fovYRadians * 0.5F);

    Matrix4x4UVE result{};
    result.m[0][0] = focalLength / aspectRatio;
    result.m[0][1] = 0.0F;
    result.m[0][2] = 0.0F;
    result.m[0][3] = 0.0F;
    result.m[1][0] = 0.0F;
    result.m[1][1] = focalLength;
    result.m[1][2] = 0.0F;
    result.m[1][3] = 0.0F;
    result.m[2][0] = 0.0F;
    result.m[2][1] = 0.0F;
    result.m[2][2] = farPlane / (nearPlane - farPlane);
    result.m[2][3] = static_cast<float>(
        (static_cast<double>(farPlane) * static_cast<double>(nearPlane)) /
        (static_cast<double>(nearPlane) - static_cast<double>(farPlane)));
    result.m[3][0] = 0.0F;
    result.m[3][1] = 0.0F;
    result.m[3][2] = -1.0F;
    result.m[3][3] = 0.0F;
    return result;
}

Matrix4x4UVE Matrix4x4UVE::OrthographicUVE(float left, float right, float bottom, float top, float nearPlane,
                                            float farPlane) noexcept {
    Matrix4x4UVE result{};
    result.m[0][0] = static_cast<float>(
        2.0 / (static_cast<double>(right) - static_cast<double>(left)));
    result.m[0][1] = 0.0F;
    result.m[0][2] = 0.0F;
    result.m[0][3] = static_cast<float>(
        -(static_cast<double>(right) + static_cast<double>(left)) /
        (static_cast<double>(right) - static_cast<double>(left)));
    result.m[1][0] = 0.0F;
    result.m[1][1] = static_cast<float>(
        2.0 / (static_cast<double>(top) - static_cast<double>(bottom)));
    result.m[1][2] = 0.0F;
    result.m[1][3] = static_cast<float>(
        -(static_cast<double>(top) + static_cast<double>(bottom)) /
        (static_cast<double>(top) - static_cast<double>(bottom)));
    result.m[2][0] = 0.0F;
    result.m[2][1] = 0.0F;
    result.m[2][2] = -1.0F / (farPlane - nearPlane);
    result.m[2][3] = -nearPlane / (farPlane - nearPlane);
    result.m[3][0] = 0.0F;
    result.m[3][1] = 0.0F;
    result.m[3][2] = 0.0F;
    result.m[3][3] = 1.0F;
    return result;
}

Matrix4x4UVE Matrix4x4UVE::ViewFromPositionAndRotationUVE(Vector3UVE position, QuaternionUVE rotation) noexcept {
    const Vector3UVE right = RotateVectorUVE(rotation, Vector3UVE{1.0F, 0.0F, 0.0F});
    const Vector3UVE up = RotateVectorUVE(rotation, Vector3UVE{0.0F, 1.0F, 0.0F});
    const Vector3UVE forward = RotateVectorUVE(rotation, Vector3UVE{0.0F, 0.0F, -1.0F});

    Matrix4x4UVE result = IdentityUVE();
    result.m[0][0] = right.x;
    result.m[0][1] = right.y;
    result.m[0][2] = right.z;
    result.m[0][3] = -DotWideUVE(right, position);
    result.m[1][0] = up.x;
    result.m[1][1] = up.y;
    result.m[1][2] = up.z;
    result.m[1][3] = -DotWideUVE(up, position);
    result.m[2][0] = -forward.x;
    result.m[2][1] = -forward.y;
    result.m[2][2] = -forward.z;
    result.m[2][3] = DotWideUVE(forward, position);
    return result;
}

Matrix4x4UVE operator*(const Matrix4x4UVE& lhs, const Matrix4x4UVE& rhs) noexcept {
    Matrix4x4UVE result{};
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            double sum = 0.0;
            for (int k = 0; k < 4; ++k) {
                sum += static_cast<double>(lhs.m[row][k]) * static_cast<double>(rhs.m[k][col]);
            }
            result.m[row][col] = static_cast<float>(sum);
        }
    }
    return result;
}

Vector3UVE TransformPointUVE(const Matrix4x4UVE& matrix, Vector3UVE point) noexcept {
    const double pointX = static_cast<double>(point.x);
    const double pointY = static_cast<double>(point.y);
    const double pointZ = static_cast<double>(point.z);
    const double transformedX = static_cast<double>(matrix.m[0][0]) * pointX +
                                static_cast<double>(matrix.m[0][1]) * pointY +
                                static_cast<double>(matrix.m[0][2]) * pointZ +
                                static_cast<double>(matrix.m[0][3]);
    const double transformedY = static_cast<double>(matrix.m[1][0]) * pointX +
                                static_cast<double>(matrix.m[1][1]) * pointY +
                                static_cast<double>(matrix.m[1][2]) * pointZ +
                                static_cast<double>(matrix.m[1][3]);
    const double transformedZ = static_cast<double>(matrix.m[2][0]) * pointX +
                                static_cast<double>(matrix.m[2][1]) * pointY +
                                static_cast<double>(matrix.m[2][2]) * pointZ +
                                static_cast<double>(matrix.m[2][3]);
    return Vector3UVE{
        static_cast<float>(transformedX),
        static_cast<float>(transformedY),
        static_cast<float>(transformedZ),
    };
}

Matrix4x4UVE TransposeUVE(const Matrix4x4UVE& matrix) noexcept {
    Matrix4x4UVE result{};
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            result.m[row][col] = matrix.m[col][row];
        }
    }
    return result;
}

bool TryInverseUVE(const Matrix4x4UVE& matrix, Matrix4x4UVE& outInverse) noexcept {
    // Gauss-Jordan elimination with partial pivoting on an augmented [matrix | identity] in
    // double precision, following this module's established convention (see operator* and
    // TransformPointUVE above) of accumulating in double even though the public type is float.
    double augmented[4][8];
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            const double value = static_cast<double>(matrix.m[row][col]);
            if (!std::isfinite(value)) {
                return false;
            }
            augmented[row][col] = value;
            augmented[row][col + 4] = (row == col) ? 1.0 : 0.0;
        }
    }

    constexpr double kMinimumPivotUVE = 1e-9;
    for (int pivotIndex = 0; pivotIndex < 4; ++pivotIndex) {
        int pivotRow = pivotIndex;
        double pivotMagnitude = std::abs(augmented[pivotIndex][pivotIndex]);
        for (int row = pivotIndex + 1; row < 4; ++row) {
            const double magnitude = std::abs(augmented[row][pivotIndex]);
            if (magnitude > pivotMagnitude) {
                pivotMagnitude = magnitude;
                pivotRow = row;
            }
        }
        if (pivotMagnitude < kMinimumPivotUVE) {
            return false; // Singular (or numerically indistinguishable from singular).
        }
        if (pivotRow != pivotIndex) {
            for (int col = 0; col < 8; ++col) {
                std::swap(augmented[pivotIndex][col], augmented[pivotRow][col]);
            }
        }

        const double pivot = augmented[pivotIndex][pivotIndex];
        for (int col = 0; col < 8; ++col) {
            augmented[pivotIndex][col] /= pivot;
        }
        for (int row = 0; row < 4; ++row) {
            if (row == pivotIndex) {
                continue;
            }
            const double factor = augmented[row][pivotIndex];
            if (factor == 0.0) {
                continue;
            }
            for (int col = 0; col < 8; ++col) {
                augmented[row][col] -= factor * augmented[pivotIndex][col];
            }
        }
    }

    Matrix4x4UVE result{};
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            const double value = augmented[row][col + 4];
            if (!std::isfinite(value)) {
                return false;
            }
            result.m[row][col] = static_cast<float>(value);
        }
    }
    outInverse = result;
    return true;
}

std::string ToStringUVE(const Matrix4x4UVE& matrix) {
    std::string result = "(";
    for (int row = 0; row < 4; ++row) {
        result += "(";
        for (int col = 0; col < 4; ++col) {
            result += std::to_string(matrix.m[row][col]);
            if (col < 3) {
                result += ", ";
            }
        }
        result += ")";
        if (row < 3) {
            result += "; ";
        }
    }
    result += ")";
    return result;
}

} // namespace UVE::Math
