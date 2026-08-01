//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/math/vector2_uve.h"

#include <string>

#include <gtest/gtest.h>

namespace UVE::Math::Tests {
namespace {

TEST(Vector2UVETest, DefaultConstruction_IsZero) {
    constexpr Vector2UVE vector{};
    EXPECT_EQ(vector.x, 0.0F);
    EXPECT_EQ(vector.y, 0.0F);
}

TEST(Vector2UVETest, Addition_SumsEachComponent) {
    constexpr Vector2UVE lhs{1.0F, 2.0F};
    constexpr Vector2UVE rhs{10.0F, 20.0F};
    constexpr Vector2UVE sum = lhs + rhs;

    EXPECT_EQ(sum.x, 11.0F);
    EXPECT_EQ(sum.y, 22.0F);
}

TEST(Vector2UVETest, Subtraction_DiffsEachComponent) {
    constexpr Vector2UVE lhs{10.0F, 20.0F};
    constexpr Vector2UVE rhs{1.0F, 2.0F};
    constexpr Vector2UVE diff = lhs - rhs;

    EXPECT_EQ(diff.x, 9.0F);
    EXPECT_EQ(diff.y, 18.0F);
}

TEST(Vector2UVETest, EqualityOperators_CompareBothComponents) {
    constexpr Vector2UVE a{1.0F, 2.0F};
    constexpr Vector2UVE b{1.0F, 2.0F};
    constexpr Vector2UVE c{1.0F, 3.0F};

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
    EXPECT_FALSE(a != b);
}

TEST(Vector2UVETest, ToStringUVE_ContainsBothComponents) {
    const Vector2UVE vector{1.0F, 2.0F};
    const std::string text = ToStringUVE(vector);

    EXPECT_NE(text.find("1.000000"), std::string::npos);
    EXPECT_NE(text.find("2.000000"), std::string::npos);
}

} // namespace
} // namespace UVE::Math::Tests
