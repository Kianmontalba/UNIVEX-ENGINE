//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/math/matrix4x4_uve.h"

#include <cmath>
#include <numbers>
#include <string>

#include <gtest/gtest.h>

namespace UVE::Math::Tests {
namespace {

constexpr float kEpsilon = 1e-4F;

struct Vec4UVE {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
    float w = 0.0F;
};

// Full 4-component multiply, used only for testing PerspectiveUVE's clip-space output (unlike
// TransformPointUVE, this deliberately keeps `w` instead of assuming an affine w=1 matrix).
[[nodiscard]] Vec4UVE MultiplyHomogeneousUVE(const Matrix4x4UVE& matrix, Vec4UVE vector) {
    Vec4UVE result{};
    result.x = matrix.m[0][0] * vector.x + matrix.m[0][1] * vector.y + matrix.m[0][2] * vector.z +
               matrix.m[0][3] * vector.w;
    result.y = matrix.m[1][0] * vector.x + matrix.m[1][1] * vector.y + matrix.m[1][2] * vector.z +
               matrix.m[1][3] * vector.w;
    result.z = matrix.m[2][0] * vector.x + matrix.m[2][1] * vector.y + matrix.m[2][2] * vector.z +
               matrix.m[2][3] * vector.w;
    result.w = matrix.m[3][0] * vector.x + matrix.m[3][1] * vector.y + matrix.m[3][2] * vector.z +
               matrix.m[3][3] * vector.w;
    return result;
}

TEST(Matrix4x4UVETest, IdentityUVE_IsIdentityMatrix) {
    constexpr Matrix4x4UVE identity = Matrix4x4UVE::IdentityUVE();
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            EXPECT_EQ(identity.m[row][col], row == col ? 1.0F : 0.0F);
        }
    }
}

TEST(Matrix4x4UVETest, ComposeTrsUVE_TranslationOnly_TransformsOriginToTranslation) {
    const Matrix4x4UVE matrix = Matrix4x4UVE::ComposeTrsUVE(Vector3UVE{5.0F, -2.0F, 3.0F}, QuaternionUVE{},
                                                              Vector3UVE{1.0F, 1.0F, 1.0F});

    const Vector3UVE transformed = TransformPointUVE(matrix, Vector3UVE{0.0F, 0.0F, 0.0F});

    EXPECT_NEAR(transformed.x, 5.0F, kEpsilon);
    EXPECT_NEAR(transformed.y, -2.0F, kEpsilon);
    EXPECT_NEAR(transformed.z, 3.0F, kEpsilon);
}

TEST(Matrix4x4UVETest, ComposeTrsUVE_ScaleOnly_ScalesPoint) {
    const Matrix4x4UVE matrix = Matrix4x4UVE::ComposeTrsUVE(Vector3UVE{}, QuaternionUVE{}, Vector3UVE{2.0F, 3.0F, 4.0F});

    const Vector3UVE transformed = TransformPointUVE(matrix, Vector3UVE{1.0F, 1.0F, 1.0F});

    EXPECT_NEAR(transformed.x, 2.0F, kEpsilon);
    EXPECT_NEAR(transformed.y, 3.0F, kEpsilon);
    EXPECT_NEAR(transformed.z, 4.0F, kEpsilon);
}

TEST(Matrix4x4UVETest, ComposeTrsUVE_NinetyDegreesAboutZ_RotatesXAxisToYAxis) {
    const float halfNinety = std::sqrt(2.0F) / 2.0F;
    const QuaternionUVE ninetyAboutZ{0.0F, 0.0F, halfNinety, halfNinety};
    const Matrix4x4UVE matrix = Matrix4x4UVE::ComposeTrsUVE(Vector3UVE{}, ninetyAboutZ, Vector3UVE{1.0F, 1.0F, 1.0F});

    const Vector3UVE transformed = TransformPointUVE(matrix, Vector3UVE{1.0F, 0.0F, 0.0F});

    EXPECT_NEAR(transformed.x, 0.0F, kEpsilon);
    EXPECT_NEAR(transformed.y, 1.0F, kEpsilon);
    EXPECT_NEAR(transformed.z, 0.0F, kEpsilon);
}

TEST(Matrix4x4UVETest, PerspectiveUVE_MapsNearPlaneToDepthZero) {
    const Matrix4x4UVE projection =
        Matrix4x4UVE::PerspectiveUVE(std::numbers::pi_v<float> / 2.0F, 1.0F, 1.0F, 100.0F);

    const Vec4UVE clip = MultiplyHomogeneousUVE(projection, Vec4UVE{0.0F, 0.0F, -1.0F, 1.0F});

    EXPECT_NEAR(clip.z / clip.w, 0.0F, kEpsilon);
}

TEST(Matrix4x4UVETest, PerspectiveUVE_MapsFarPlaneToDepthOne) {
    const Matrix4x4UVE projection =
        Matrix4x4UVE::PerspectiveUVE(std::numbers::pi_v<float> / 2.0F, 1.0F, 1.0F, 100.0F);

    const Vec4UVE clip = MultiplyHomogeneousUVE(projection, Vec4UVE{0.0F, 0.0F, -100.0F, 1.0F});

    EXPECT_NEAR(clip.z / clip.w, 1.0F, kEpsilon);
}

TEST(Matrix4x4UVETest, ViewFromPositionAndRotationUVE_IdentityRotation_TransformsWorldPointRelativeToEye) {
    const Matrix4x4UVE view = Matrix4x4UVE::ViewFromPositionAndRotationUVE(Vector3UVE{0.0F, 0.0F, 5.0F}, QuaternionUVE{});

    const Vector3UVE viewSpacePoint = TransformPointUVE(view, Vector3UVE{0.0F, 0.0F, 0.0F});

    EXPECT_NEAR(viewSpacePoint.x, 0.0F, kEpsilon);
    EXPECT_NEAR(viewSpacePoint.y, 0.0F, kEpsilon);
    EXPECT_NEAR(viewSpacePoint.z, -5.0F, kEpsilon);
}

TEST(Matrix4x4UVETest, ViewFromPositionAndRotationUVE_NinetyDegreeYaw_PointAheadOfNewForwardIsStraightAhead) {
    const float halfNinety = std::sqrt(2.0F) / 2.0F;
    const QuaternionUVE ninetyAboutY{0.0F, halfNinety, 0.0F, halfNinety};
    const Matrix4x4UVE view = Matrix4x4UVE::ViewFromPositionAndRotationUVE(Vector3UVE{0.0F, 0.0F, 0.0F}, ninetyAboutY);

    // After a 90-degree yaw, the camera's new world-space forward direction is (-1, 0, 0), so a
    // point 5 units along that direction should land straight ahead in view space: (0, 0, -5).
    const Vector3UVE viewSpacePoint = TransformPointUVE(view, Vector3UVE{-5.0F, 0.0F, 0.0F});

    EXPECT_NEAR(viewSpacePoint.x, 0.0F, kEpsilon);
    EXPECT_NEAR(viewSpacePoint.y, 0.0F, kEpsilon);
    EXPECT_NEAR(viewSpacePoint.z, -5.0F, kEpsilon);
}

TEST(Matrix4x4UVETest, MatrixMultiply_WithIdentity_IsUnchanged) {
    const Matrix4x4UVE matrix = Matrix4x4UVE::ComposeTrsUVE(Vector3UVE{1.0F, 2.0F, 3.0F}, QuaternionUVE{},
                                                              Vector3UVE{1.0F, 1.0F, 1.0F});
    constexpr Matrix4x4UVE identity = Matrix4x4UVE::IdentityUVE();

    EXPECT_EQ(matrix * identity, matrix);
    EXPECT_EQ(identity * matrix, matrix);
}

TEST(Matrix4x4UVETest, MatrixMultiply_ComposesInRightToLeftOrder) {
    const Matrix4x4UVE translateX = Matrix4x4UVE::ComposeTrsUVE(Vector3UVE{10.0F, 0.0F, 0.0F}, QuaternionUVE{},
                                                                  Vector3UVE{1.0F, 1.0F, 1.0F});
    const Matrix4x4UVE translateY = Matrix4x4UVE::ComposeTrsUVE(Vector3UVE{0.0F, 5.0F, 0.0F}, QuaternionUVE{},
                                                                  Vector3UVE{1.0F, 1.0F, 1.0F});

    const Matrix4x4UVE combined = translateX * translateY;
    const Vector3UVE transformed = TransformPointUVE(combined, Vector3UVE{0.0F, 0.0F, 0.0F});

    EXPECT_NEAR(transformed.x, 10.0F, kEpsilon);
    EXPECT_NEAR(transformed.y, 5.0F, kEpsilon);
    EXPECT_NEAR(transformed.z, 0.0F, kEpsilon);
}

TEST(Matrix4x4UVETest, EqualityOperators_CompareAllSixteenComponents) {
    constexpr Matrix4x4UVE a = Matrix4x4UVE::IdentityUVE();
    constexpr Matrix4x4UVE b = Matrix4x4UVE::IdentityUVE();
    Matrix4x4UVE c = Matrix4x4UVE::IdentityUVE();
    c.m[0][3] = 1.0F;

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(a != c);
    EXPECT_FALSE(a == c);
}

TEST(Matrix4x4UVETest, ToStringUVE_FormatsAllFourRows) {
    constexpr Matrix4x4UVE identity = Matrix4x4UVE::IdentityUVE();
    const std::string text = ToStringUVE(identity);

    EXPECT_NE(text.find("1.000000"), std::string::npos);
    EXPECT_NE(text.find(";"), std::string::npos);
}

} // namespace
} // namespace UVE::Math::Tests
