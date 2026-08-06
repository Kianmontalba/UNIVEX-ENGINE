// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/math/frustum_uve.h"

#include <numbers>

#include <gtest/gtest.h>

namespace UVE::Math::Tests {
namespace {

// A 90-degree-vertical-FOV, square-aspect, near=1/far=100 frustum looking down -Z from the
// world origin (identity view) — shared by every test below.
[[nodiscard]] FrustumUVE MakeTestFrustumUVE() {
    const Matrix4x4UVE projection =
        Matrix4x4UVE::PerspectiveUVE(std::numbers::pi_v<float> / 2.0F, 1.0F, 1.0F, 100.0F);
    const Matrix4x4UVE view = Matrix4x4UVE::ViewFromPositionAndRotationUVE(Vector3UVE{0.0F, 0.0F, 0.0F}, QuaternionUVE{});
    return FrustumUVE::FromViewProjectionUVE(projection * view);
}

TEST(FrustumUVETest, IntersectsUVE_BoxDirectlyAheadWithinRange_IsVisible) {
    const FrustumUVE frustum = MakeTestFrustumUVE();
    const AabbUVE box = AabbUVE::FromCenterExtentsUVE(Vector3UVE{0.0F, 0.0F, -10.0F}, Vector3UVE{1.0F, 1.0F, 1.0F});

    EXPECT_TRUE(frustum.IntersectsUVE(box));
}

TEST(FrustumUVETest, IntersectsUVE_BoxBehindCamera_IsNotVisible) {
    const FrustumUVE frustum = MakeTestFrustumUVE();
    const AabbUVE box = AabbUVE::FromCenterExtentsUVE(Vector3UVE{0.0F, 0.0F, 10.0F}, Vector3UVE{1.0F, 1.0F, 1.0F});

    EXPECT_FALSE(frustum.IntersectsUVE(box));
}

TEST(FrustumUVETest, IntersectsUVE_BoxFarOffToTheSide_IsNotVisible) {
    const FrustumUVE frustum = MakeTestFrustumUVE();
    const AabbUVE box = AabbUVE::FromCenterExtentsUVE(Vector3UVE{1000.0F, 0.0F, -10.0F}, Vector3UVE{1.0F, 1.0F, 1.0F});

    EXPECT_FALSE(frustum.IntersectsUVE(box));
}

TEST(FrustumUVETest, IntersectsUVE_BoxBeyondFarPlane_IsNotVisible) {
    const FrustumUVE frustum = MakeTestFrustumUVE();
    const AabbUVE box = AabbUVE::FromCenterExtentsUVE(Vector3UVE{0.0F, 0.0F, -1000.0F}, Vector3UVE{1.0F, 1.0F, 1.0F});

    EXPECT_FALSE(frustum.IntersectsUVE(box));
}

TEST(FrustumUVETest, IntersectsUVE_BoxCloserThanNearPlane_IsNotVisible) {
    const FrustumUVE frustum = MakeTestFrustumUVE();
    const AabbUVE box = AabbUVE::FromCenterExtentsUVE(Vector3UVE{0.0F, 0.0F, -0.1F}, Vector3UVE{0.01F, 0.01F, 0.01F});

    EXPECT_FALSE(frustum.IntersectsUVE(box));
}

} // namespace
} // namespace UVE::Math::Tests
