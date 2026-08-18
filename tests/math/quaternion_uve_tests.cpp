// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


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

TEST(QuaternionUVETest, CheckedHelpers_NormalizeInvertAndConstructAxisAngle) {
    QuaternionUVE normalized{};
    EXPECT_TRUE(TryNormalizeUVE(QuaternionUVE{0.0F, 0.0F, 2.0F, 2.0F}, normalized));
    EXPECT_NEAR(LengthSquaredUVE(normalized), 1.0F, kEpsilon);
    EXPECT_TRUE(IsFiniteUVE(normalized));

    QuaternionUVE inverse{};
    ASSERT_TRUE(TryInverseUVE(normalized, inverse));
    const QuaternionUVE identity = MultiplyUVE(normalized, inverse);
    EXPECT_NEAR(identity.x, 0.0F, kEpsilon);
    EXPECT_NEAR(identity.y, 0.0F, kEpsilon);
    EXPECT_NEAR(identity.z, 0.0F, kEpsilon);
    EXPECT_NEAR(identity.w, 1.0F, kEpsilon);

    QuaternionUVE ninetyAboutZ{};
    ASSERT_TRUE(TryMakeAxisAngleUVE(Vector3UVE{0.0F, 0.0F, 1.0F}, std::numbers::pi_v<float> * 0.5F,
                                     ninetyAboutZ));
    const Vector3UVE rotated = RotateVectorUVE(ninetyAboutZ, Vector3UVE{1.0F, 0.0F, 0.0F});
    EXPECT_NEAR(rotated.x, 0.0F, kEpsilon);
    EXPECT_NEAR(rotated.y, 1.0F, kEpsilon);
    EXPECT_NEAR(rotated.z, 0.0F, kEpsilon);
}

TEST(QuaternionUVETest, CheckedHelpers_RejectNonFiniteOrZeroInputWithoutChangingOutput) {
    QuaternionUVE preserved{1.0F, 2.0F, 3.0F, 4.0F};
    EXPECT_FALSE(TryNormalizeUVE(QuaternionUVE{0.0F, 0.0F, 0.0F, 0.0F}, preserved));
    EXPECT_EQ(preserved, (QuaternionUVE{1.0F, 2.0F, 3.0F, 4.0F}));
    EXPECT_FALSE(TryInverseUVE(QuaternionUVE{NAN, 0.0F, 0.0F, 1.0F}, preserved));
    EXPECT_EQ(preserved, (QuaternionUVE{1.0F, 2.0F, 3.0F, 4.0F}));
    EXPECT_FALSE(TryMakeAxisAngleUVE(Vector3UVE{}, 0.0F, preserved));
    EXPECT_EQ(preserved, (QuaternionUVE{1.0F, 2.0F, 3.0F, 4.0F}));
}

TEST(QuaternionUVETest, CheckedHelpers_EulerLookAtSlerpAndAxisAngleDecomposition) {
    QuaternionUVE euler{};
    ASSERT_TRUE(TryMakeEulerUVE(Vector3UVE{0.0F, 0.0F, std::numbers::pi_v<float> * 0.5F}, euler));
    Vector3UVE axis{};
    float radians = 0.0F;
    ASSERT_TRUE(TryToAxisAngleUVE(euler, axis, radians));
    EXPECT_NEAR(axis.z, 1.0F, kEpsilon);
    EXPECT_NEAR(radians, std::numbers::pi_v<float> * 0.5F, kEpsilon);

    QuaternionUVE lookAt{};
    ASSERT_TRUE(TryMakeLookAtUVE(Vector3UVE{0.0F, 0.0F, 1.0F}, Vector3UVE{0.0F, 1.0F, 0.0F}, lookAt));
    const Vector3UVE forward = RotateVectorUVE(lookAt, Vector3UVE{0.0F, 0.0F, 1.0F});
    EXPECT_NEAR(forward.x, 0.0F, kEpsilon);
    EXPECT_NEAR(forward.y, 0.0F, kEpsilon);
    EXPECT_NEAR(forward.z, 1.0F, kEpsilon);

    QuaternionUVE half{};
    ASSERT_TRUE(TrySlerpUVE(QuaternionUVE{}, euler, 0.5F, half));
    EXPECT_NEAR(LengthSquaredUVE(half), 1.0F, kEpsilon);
    EXPECT_FALSE(TryMakeLookAtUVE(Vector3UVE{0.0F, 0.0F, 0.0F}, Vector3UVE{0.0F, 1.0F, 0.0F}, half));
}

TEST(QuaternionUVETest, ToStringUVE_FormatsAllFourComponents) {
    const QuaternionUVE rotation{0.0F, 0.0F, 0.0F, 1.0F};
    const std::string text = ToStringUVE(rotation);

    EXPECT_NE(text.find("1.000000"), std::string::npos);
}

} // namespace
} // namespace UVE::Math::Tests
