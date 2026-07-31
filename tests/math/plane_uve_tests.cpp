//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/math/plane_uve.h"

#include <gtest/gtest.h>

namespace UVE::Math::Tests {
namespace {

constexpr float kEpsilon = 1e-5F;

TEST(PlaneUVETest, FromNormalAndPointUVE_PointUsedToConstructIt_HasZeroDistance) {
    const PlaneUVE plane = PlaneUVE::FromNormalAndPointUVE(Vector3UVE{0.0F, 1.0F, 0.0F}, Vector3UVE{5.0F, 3.0F, -2.0F});

    EXPECT_NEAR(plane.GetSignedDistanceUVE(Vector3UVE{5.0F, 3.0F, -2.0F}), 0.0F, kEpsilon);
}

TEST(PlaneUVETest, GetSignedDistanceUVE_PointOnNormalSide_IsPositive) {
    const PlaneUVE plane = PlaneUVE::FromNormalAndPointUVE(Vector3UVE{0.0F, 1.0F, 0.0F}, Vector3UVE{0.0F, 0.0F, 0.0F});

    EXPECT_NEAR(plane.GetSignedDistanceUVE(Vector3UVE{0.0F, 10.0F, 0.0F}), 10.0F, kEpsilon);
}

TEST(PlaneUVETest, GetSignedDistanceUVE_PointOnOppositeSide_IsNegative) {
    const PlaneUVE plane = PlaneUVE::FromNormalAndPointUVE(Vector3UVE{0.0F, 1.0F, 0.0F}, Vector3UVE{0.0F, 0.0F, 0.0F});

    EXPECT_NEAR(plane.GetSignedDistanceUVE(Vector3UVE{0.0F, -4.0F, 0.0F}), -4.0F, kEpsilon);
}

} // namespace
} // namespace UVE::Math::Tests
