//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/math/vector3_uve.h"

#include <string>

#include <gtest/gtest.h>

namespace UVE::Math::Tests {
namespace {

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

TEST(Vector3UVETest, ToStringUVE_FormatsAllThreeComponents) {
    const Vector3UVE vector{1.0F, 2.0F, 3.0F};
    const std::string text = ToStringUVE(vector);

    EXPECT_NE(text.find("1.000000"), std::string::npos);
    EXPECT_NE(text.find("2.000000"), std::string::npos);
    EXPECT_NE(text.find("3.000000"), std::string::npos);
}

} // namespace
} // namespace UVE::Math::Tests
