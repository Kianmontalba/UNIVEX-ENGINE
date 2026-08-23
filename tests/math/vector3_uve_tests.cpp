// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/math/vector3_uve.h"

#include <cmath>
#include <string>

#include <gtest/gtest.h>

namespace UVE::Math::Tests {
namespace {

constexpr float kEpsilon = 1e-5F;

TEST(Vector3UVETest, DefaultConstruction_IsZero) {
    constexpr Vector3UVE vector{};
    EXPECT_EQ(vector.x, 0.0F);
    EXPECT_EQ(vector.y, 0.0F);
    EXPECT_EQ(vector.z, 0.0F);
}

TEST(Vector3UVETest, Addition_SumsEachComponent) {
    constexpr Vector3UVE lhs{1.0F, 2.0F, 3.0F};
    constexpr Vector3UVE rhs{10.0F, 20.0F, 30.0F};
    constexpr Vector3UVE sum = lhs + rhs;

    EXPECT_EQ(sum.x, 11.0F);
    EXPECT_EQ(sum.y, 22.0F);
    EXPECT_EQ(sum.z, 33.0F);
}

TEST(Vector3UVETest, ComponentWiseMultiplication_MultipliesEachComponent) {
    constexpr Vector3UVE lhs{2.0F, 3.0F, 4.0F};
    constexpr Vector3UVE rhs{5.0F, 6.0F, 7.0F};
    constexpr Vector3UVE product = lhs * rhs;

    EXPECT_EQ(product.x, 10.0F);
    EXPECT_EQ(product.y, 18.0F);
    EXPECT_EQ(product.z, 28.0F);
}

TEST(Vector3UVETest, EqualityOperators_CompareAllComponents) {
    constexpr Vector3UVE a{1.0F, 2.0F, 3.0F};
    constexpr Vector3UVE b{1.0F, 2.0F, 3.0F};
    constexpr Vector3UVE c{1.0F, 2.0F, 4.0F};

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(a != c);
    EXPECT_FALSE(a == c);
}

TEST(Vector3UVETest, Subtraction_DiffsEachComponent) {
    constexpr Vector3UVE lhs{10.0F, 20.0F, 30.0F};
    constexpr Vector3UVE rhs{1.0F, 2.0F, 3.0F};
    constexpr Vector3UVE diff = lhs - rhs;

    EXPECT_EQ(diff.x, 9.0F);
    EXPECT_EQ(diff.y, 18.0F);
    EXPECT_EQ(diff.z, 27.0F);
}

TEST(Vector3UVETest, UnaryNegate_FlipsEachComponent) {
    constexpr Vector3UVE v{1.0F, -2.0F, 3.0F};
    constexpr Vector3UVE negated = -v;

    EXPECT_EQ(negated.x, -1.0F);
    EXPECT_EQ(negated.y, 2.0F);
    EXPECT_EQ(negated.z, -3.0F);
}

TEST(Vector3UVETest, ScalarMultiplication_ScalesEachComponent) {
    constexpr Vector3UVE v{1.0F, 2.0F, 3.0F};
    constexpr Vector3UVE scaled = v * 2.0F;

    EXPECT_EQ(scaled.x, 2.0F);
    EXPECT_EQ(scaled.y, 4.0F);
    EXPECT_EQ(scaled.z, 6.0F);
}

TEST(Vector3UVETest, CompoundAssignmentOperators_MutateInPlace) {
    Vector3UVE v{1.0F, 2.0F, 3.0F};
    v += Vector3UVE{1.0F, 1.0F, 1.0F};
    EXPECT_EQ(v, (Vector3UVE{2.0F, 3.0F, 4.0F}));

    v -= Vector3UVE{1.0F, 1.0F, 1.0F};
    EXPECT_EQ(v, (Vector3UVE{1.0F, 2.0F, 3.0F}));

    v *= 2.0F;
    EXPECT_EQ(v, (Vector3UVE{2.0F, 4.0F, 6.0F}));
}

TEST(Vector3UVETest, DotUVE_KnownOrthogonalVectors_IsZero) {
    constexpr Vector3UVE right{1.0F, 0.0F, 0.0F};
    constexpr Vector3UVE up{0.0F, 1.0F, 0.0F};
    EXPECT_EQ(DotUVE(right, up), 0.0F);
}

TEST(Vector3UVETest, DotUVE_KnownVectors_MatchesHandComputedValue) {
    constexpr Vector3UVE lhs{1.0F, 2.0F, 3.0F};
    constexpr Vector3UVE rhs{4.0F, 5.0F, 6.0F};
    EXPECT_EQ(DotUVE(lhs, rhs), 32.0F); // 1*4 + 2*5 + 3*6
}

TEST(Vector3UVETest, CrossUVE_RightCrossUp_IsForward) {
    constexpr Vector3UVE right{1.0F, 0.0F, 0.0F};
    constexpr Vector3UVE up{0.0F, 1.0F, 0.0F};
    constexpr Vector3UVE forward = CrossUVE(right, up);

    EXPECT_EQ(forward, (Vector3UVE{0.0F, 0.0F, 1.0F}));
}

TEST(Vector3UVETest, LengthUVE_KnownVector_MatchesHandComputedValue) {
    const Vector3UVE v{3.0F, 4.0F, 0.0F};
    EXPECT_NEAR(LengthUVE(v), 5.0F, kEpsilon);
    EXPECT_NEAR(LengthSquaredUVE(v), 25.0F, kEpsilon);
}

TEST(Vector3UVETest, LengthUVE_PreservesFiniteMaximumAxis) {
    const float maximum = std::numeric_limits<float>::max();

    EXPECT_TRUE(std::isfinite(LengthUVE(Vector3UVE{maximum, 0.0F, 0.0F})));
    EXPECT_FLOAT_EQ(LengthUVE(Vector3UVE{maximum, 0.0F, 0.0F}), maximum);
}

TEST(Vector3UVETest, NormalizeUVE_NonZeroVector_ProducesUnitLength) {
    const Vector3UVE v{3.0F, 4.0F, 0.0F};
    const Vector3UVE normalized = NormalizeUVE(v);

    EXPECT_NEAR(LengthUVE(normalized), 1.0F, kEpsilon);
    EXPECT_NEAR(normalized.x, 0.6F, kEpsilon);
    EXPECT_NEAR(normalized.y, 0.8F, kEpsilon);
}

TEST(Vector3UVETest, NormalizeUVE_LargeFiniteVector_ProducesUnitLength) {
    const float maximum = std::numeric_limits<float>::max();
    const Vector3UVE normalized = NormalizeUVE(Vector3UVE{maximum, maximum, 0.0F});
    EXPECT_TRUE(std::isfinite(normalized.x));
    EXPECT_TRUE(std::isfinite(normalized.y));
    EXPECT_TRUE(std::isfinite(normalized.z));
    EXPECT_NEAR(LengthSquaredUVE(normalized), 1.0F, kEpsilon);
    EXPECT_NEAR(normalized.x, 0.70710677F, 1.0e-6F);
    EXPECT_NEAR(normalized.y, 0.70710677F, 1.0e-6F);
}

TEST(Vector3UVETest, NormalizeUVE_ZeroVector_ProducesInfRatherThanTrapping) {
    // Documents NormalizeUVE()'s zero-length contract (see its doc comment): callers must not
    // pass the zero vector, but IEEE754 float division by zero produces +/-inf, not a crash —
    // this test pins down that actual (not just documented) behavior.
    const Vector3UVE normalized = NormalizeUVE(Vector3UVE{});
    EXPECT_TRUE(std::isinf(normalized.x) || std::isnan(normalized.x));
}

TEST(Vector3UVETest, ToStringUVE_FormatsAllThreeComponents) {
    const Vector3UVE vector{1.0F, 2.0F, 3.0F};
    const std::string text = ToStringUVE(vector);

    EXPECT_NE(text.find("1.000000"), std::string::npos);
    EXPECT_NE(text.find("2.000000"), std::string::npos);
    EXPECT_NE(text.find("3.000000"), std::string::npos);
}

} // namespace
} // namespace UVE::Math::Tests
