// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/math/plane_uve.h"

#include <cmath>
#include <limits>

#include <gtest/gtest.h>

namespace UVE::Math::Tests {
namespace {

constexpr float kEpsilon = 1e-5F;

TEST(PlaneUVETest, FromNormalAndPointUVE_PointUsedToConstructIt_HasZeroDistance) {
    const PlaneUVE plane = PlaneUVE::FromNormalAndPointUVE(Vector3UVE{0.0F, 1.0F, 0.0F}, Vector3UVE{5.0F, 3.0F, -2.0F});

    EXPECT_NEAR(plane.GetSignedDistanceUVE(Vector3UVE{5.0F, 3.0F, -2.0F}), 0.0F, kEpsilon);
}

TEST(PlaneUVETest, ExtremeFiniteDotCancellation_PreservesPlaneConstructionAndDistance) {
    const float maximum = std::numeric_limits<float>::max();
    const float diagonal = 0.57735026919F;
    const Vector3UVE normal{diagonal, diagonal, -diagonal};
    const Vector3UVE point{maximum, maximum, maximum};
    const PlaneUVE plane = PlaneUVE::FromNormalAndPointUVE(normal, point);

    EXPECT_TRUE(std::isfinite(plane.distance));
    EXPECT_TRUE(std::isfinite(plane.GetSignedDistanceUVE(point)));
    EXPECT_LT(std::fabs(plane.GetSignedDistanceUVE(point)), maximum * 1.0e-5F);
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
