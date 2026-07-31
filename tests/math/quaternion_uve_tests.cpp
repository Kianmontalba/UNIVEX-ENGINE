//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/math/quaternion_uve.h"

#include <cmath>
#include <string>

#include <gtest/gtest.h>

namespace UVE::Math::Tests {
namespace {

constexpr float kEpsilon = 1e-5F;

TEST(QuaternionUVETest, DefaultConstruction_IsIdentity) {
    constexpr QuaternionUVE rotation{};
    EXPECT_EQ(rotation.x, 0.0F);
    EXPECT_EQ(rotation.y, 0.0F);
    EXPECT_EQ(rotation.z, 0.0F);
    EXPECT_EQ(rotation.w, 1.0F);
}

TEST(QuaternionUVETest, EqualityOperators_CompareAllFourComponents) {
    constexpr QuaternionUVE a{0.0F, 0.0F, 0.0F, 1.0F};
    constexpr QuaternionUVE b{0.0F, 0.0F, 0.0F, 1.0F};
    constexpr QuaternionUVE c{1.0F, 0.0F, 0.0F, 0.0F};

    EXPECT_TRUE(a == b);
    EXPECT_TRUE(a != c);
}

TEST(QuaternionUVETest, MultiplyUVE_WithIdentity_IsUnchanged) {
    constexpr QuaternionUVE identity{};
    constexpr QuaternionUVE rotation{0.1F, 0.2F, 0.3F, 0.9F};

    EXPECT_EQ(MultiplyUVE(rotation, identity), rotation);
    EXPECT_EQ(MultiplyUVE(identity, rotation), rotation);
}

TEST(QuaternionUVETest, MultiplyUVE_TwoNinetyDegreeZRotations_YieldsOneHundredEightyDegreeRotation) {
    const float halfNinety = std::sqrt(2.0F) / 2.0F;
    const QuaternionUVE ninetyAboutZ{0.0F, 0.0F, halfNinety, halfNinety};

    const QuaternionUVE oneEighty = MultiplyUVE(ninetyAboutZ, ninetyAboutZ);

    EXPECT_NEAR(oneEighty.x, 0.0F, kEpsilon);
    EXPECT_NEAR(oneEighty.y, 0.0F, kEpsilon);
    EXPECT_NEAR(oneEighty.z, 1.0F, kEpsilon);
    EXPECT_NEAR(oneEighty.w, 0.0F, kEpsilon);
}

TEST(QuaternionUVETest, RotateVectorUVE_WithIdentity_IsUnchanged) {
    constexpr QuaternionUVE identity{};
    constexpr Vector3UVE vector{1.0F, 2.0F, 3.0F};

    EXPECT_EQ(RotateVectorUVE(identity, vector), vector);
}

TEST(QuaternionUVETest, RotateVectorUVE_OneHundredEightyDegreesAboutZ_NegatesXAndY) {
    constexpr QuaternionUVE oneEightyAboutZ{0.0F, 0.0F, 1.0F, 0.0F};
    constexpr Vector3UVE vector{1.0F, 1.0F, 5.0F};

    const Vector3UVE rotated = RotateVectorUVE(oneEightyAboutZ, vector);

    EXPECT_NEAR(rotated.x, -1.0F, kEpsilon);
    EXPECT_NEAR(rotated.y, -1.0F, kEpsilon);
    EXPECT_NEAR(rotated.z, 5.0F, kEpsilon);
}

TEST(QuaternionUVETest, RotateVectorUVE_NinetyDegreesAboutZ_RotatesXAxisToYAxis) {
    const float halfNinety = std::sqrt(2.0F) / 2.0F;
    const QuaternionUVE ninetyAboutZ{0.0F, 0.0F, halfNinety, halfNinety};
    constexpr Vector3UVE xAxis{1.0F, 0.0F, 0.0F};

    const Vector3UVE rotated = RotateVectorUVE(ninetyAboutZ, xAxis);

    EXPECT_NEAR(rotated.x, 0.0F, kEpsilon);
    EXPECT_NEAR(rotated.y, 1.0F, kEpsilon);
    EXPECT_NEAR(rotated.z, 0.0F, kEpsilon);
}

TEST(QuaternionUVETest, ToStringUVE_FormatsAllFourComponents) {
    const QuaternionUVE rotation{0.0F, 0.0F, 0.0F, 1.0F};
    const std::string text = ToStringUVE(rotation);

    EXPECT_NE(text.find("1.000000"), std::string::npos);
}

} // namespace
} // namespace UVE::Math::Tests
