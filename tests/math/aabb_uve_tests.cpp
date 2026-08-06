// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/math/aabb_uve.h"

#include <cmath>
#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace UVE::Math::Tests {
namespace {

constexpr float kEpsilon = 1e-4F;

TEST(AabbUVETest, FromCenterExtentsUVE_ComputesMinAndMax) {
    const AabbUVE box = AabbUVE::FromCenterExtentsUVE(Vector3UVE{1.0F, 2.0F, 3.0F}, Vector3UVE{1.0F, 2.0F, 3.0F});

    EXPECT_EQ(box.min, (Vector3UVE{0.0F, 0.0F, 0.0F}));
    EXPECT_EQ(box.max, (Vector3UVE{2.0F, 4.0F, 6.0F}));
}

TEST(AabbUVETest, GetCenterUVE_And_GetExtentsUVE_RoundTripFromCenterExtentsUVE) {
    const Vector3UVE center{1.0F, 2.0F, 3.0F};
    const Vector3UVE extents{4.0F, 5.0F, 6.0F};
    const AabbUVE box = AabbUVE::FromCenterExtentsUVE(center, extents);

    EXPECT_EQ(box.GetCenterUVE(), center);
    EXPECT_EQ(box.GetExtentsUVE(), extents);
}

TEST(AabbUVETest, UnionUVE_CombinesTwoBoxesIntoTheirEnclosingBox) {
    const AabbUVE a{Vector3UVE{-1.0F, -1.0F, -1.0F}, Vector3UVE{1.0F, 1.0F, 1.0F}};
    const AabbUVE b{Vector3UVE{0.0F, 0.0F, 0.0F}, Vector3UVE{5.0F, 5.0F, 5.0F}};

    const AabbUVE combined = a.UnionUVE(b);

    EXPECT_EQ(combined.min, (Vector3UVE{-1.0F, -1.0F, -1.0F}));
    EXPECT_EQ(combined.max, (Vector3UVE{5.0F, 5.0F, 5.0F}));
}

TEST(AabbUVETest, TransformUVE_Translation_ShiftsBoxWithoutChangingSize) {
    const AabbUVE box{Vector3UVE{-1.0F, -1.0F, -1.0F}, Vector3UVE{1.0F, 1.0F, 1.0F}};
    const Matrix4x4UVE translation =
        Matrix4x4UVE::ComposeTrsUVE(Vector3UVE{10.0F, 0.0F, 0.0F}, QuaternionUVE{}, Vector3UVE{1.0F, 1.0F, 1.0F});

    const AabbUVE transformed = box.TransformUVE(translation);

    EXPECT_NEAR(transformed.min.x, 9.0F, kEpsilon);
    EXPECT_NEAR(transformed.max.x, 11.0F, kEpsilon);
    EXPECT_NEAR(transformed.min.y, -1.0F, kEpsilon);
    EXPECT_NEAR(transformed.max.y, 1.0F, kEpsilon);
}

TEST(AabbUVETest, TransformUVE_Scale_ScalesBoxAboutOrigin) {
    const AabbUVE box{Vector3UVE{-1.0F, -1.0F, -1.0F}, Vector3UVE{1.0F, 1.0F, 1.0F}};
    const Matrix4x4UVE scale =
        Matrix4x4UVE::ComposeTrsUVE(Vector3UVE{}, QuaternionUVE{}, Vector3UVE{2.0F, 2.0F, 2.0F});

    const AabbUVE transformed = box.TransformUVE(scale);

    EXPECT_NEAR(transformed.min.x, -2.0F, kEpsilon);
    EXPECT_NEAR(transformed.max.x, 2.0F, kEpsilon);
}

TEST(AabbUVETest, TransformUVE_NinetyDegreesAboutZ_SwapsXAndYExtents) {
    const AabbUVE box{Vector3UVE{-1.0F, -2.0F, -3.0F}, Vector3UVE{1.0F, 2.0F, 3.0F}};
    const float halfNinety = std::sqrt(2.0F) / 2.0F;
    const QuaternionUVE ninetyAboutZ{0.0F, 0.0F, halfNinety, halfNinety};
    const Matrix4x4UVE rotation =
        Matrix4x4UVE::ComposeTrsUVE(Vector3UVE{}, ninetyAboutZ, Vector3UVE{1.0F, 1.0F, 1.0F});

    const AabbUVE transformed = box.TransformUVE(rotation);

    EXPECT_NEAR(transformed.min.x, -2.0F, kEpsilon);
    EXPECT_NEAR(transformed.max.x, 2.0F, kEpsilon);
    EXPECT_NEAR(transformed.min.y, -1.0F, kEpsilon);
    EXPECT_NEAR(transformed.max.y, 1.0F, kEpsilon);
    EXPECT_NEAR(transformed.min.z, -3.0F, kEpsilon);
    EXPECT_NEAR(transformed.max.z, 3.0F, kEpsilon);
}

TEST(AabbUVETest, EqualityOperators_CompareMinAndMax) {
    const AabbUVE a{Vector3UVE{0.0F, 0.0F, 0.0F}, Vector3UVE{1.0F, 1.0F, 1.0F}};
    const AabbUVE b{Vector3UVE{0.0F, 0.0F, 0.0F}, Vector3UVE{1.0F, 1.0F, 1.0F}};
    const AabbUVE c{Vector3UVE{0.0F, 0.0F, 0.0F}, Vector3UVE{2.0F, 1.0F, 1.0F}};

    EXPECT_TRUE(a == b);
    EXPECT_TRUE(a != c);
}

TEST(AabbUVETest, IntersectsUVE_OverlappingBoxes_ReturnsTrue) {
    const AabbUVE a{Vector3UVE{0.0F, 0.0F, 0.0F}, Vector3UVE{2.0F, 2.0F, 2.0F}};
    const AabbUVE b{Vector3UVE{1.0F, 1.0F, 1.0F}, Vector3UVE{3.0F, 3.0F, 3.0F}};

    EXPECT_TRUE(a.IntersectsUVE(b));
    EXPECT_TRUE(b.IntersectsUVE(a));
}

TEST(AabbUVETest, IntersectsUVE_DisjointBoxes_ReturnsFalse) {
    const AabbUVE a{Vector3UVE{0.0F, 0.0F, 0.0F}, Vector3UVE{1.0F, 1.0F, 1.0F}};
    const AabbUVE b{Vector3UVE{10.0F, 10.0F, 10.0F}, Vector3UVE{11.0F, 11.0F, 11.0F}};

    EXPECT_FALSE(a.IntersectsUVE(b));
    EXPECT_FALSE(b.IntersectsUVE(a));
}

TEST(AabbUVETest, IntersectsUVE_TouchingEdges_ReturnsFalse) {
    const AabbUVE a{Vector3UVE{0.0F, 0.0F, 0.0F}, Vector3UVE{1.0F, 1.0F, 1.0F}};
    const AabbUVE b{Vector3UVE{1.0F, 0.0F, 0.0F}, Vector3UVE{2.0F, 1.0F, 1.0F}};

    EXPECT_FALSE(a.IntersectsUVE(b));
}

TEST(AabbUVETest, ComputePenetrationUVE_DisjointBoxes_ReturnsNullopt) {
    const AabbUVE a{Vector3UVE{0.0F, 0.0F, 0.0F}, Vector3UVE{1.0F, 1.0F, 1.0F}};
    const AabbUVE b{Vector3UVE{10.0F, 10.0F, 10.0F}, Vector3UVE{11.0F, 11.0F, 11.0F}};

    EXPECT_FALSE(ComputePenetrationUVE(a, b).has_value());
}

TEST(AabbUVETest, ComputePenetrationUVE_OverlappingAlongX_ReturnsXAxisAndCorrectDepth) {
    // a spans x[0,2], b spans x[1,3] (both y/z identical and much larger, so x is the
    // shallowest, correct MTV axis) — overlap along x is [1,2], depth 1.
    const AabbUVE a{Vector3UVE{0.0F, 0.0F, 0.0F}, Vector3UVE{2.0F, 10.0F, 10.0F}};
    const AabbUVE b{Vector3UVE{1.0F, 0.0F, 0.0F}, Vector3UVE{3.0F, 10.0F, 10.0F}};

    const std::optional<PenetrationUVE> penetration = ComputePenetrationUVE(a, b);
    ASSERT_TRUE(penetration.has_value());
    EXPECT_NEAR(penetration->depth, 1.0F, kEpsilon);
    EXPECT_NEAR(penetration->axis.x, 1.0F, kEpsilon); // points from a's center toward b's center
    EXPECT_NEAR(penetration->axis.y, 0.0F, kEpsilon);
    EXPECT_NEAR(penetration->axis.z, 0.0F, kEpsilon);
}

TEST(AabbUVETest, ComputePenetrationUVE_AxisPointsFromFirstTowardSecond) {
    const AabbUVE a{Vector3UVE{1.0F, 0.0F, 0.0F}, Vector3UVE{3.0F, 10.0F, 10.0F}};
    const AabbUVE b{Vector3UVE{0.0F, 0.0F, 0.0F}, Vector3UVE{2.0F, 10.0F, 10.0F}};

    const std::optional<PenetrationUVE> penetration = ComputePenetrationUVE(a, b);
    ASSERT_TRUE(penetration.has_value());
    // b's center.x < a's center.x, so the axis (from a toward b) points in -x.
    EXPECT_NEAR(penetration->axis.x, -1.0F, kEpsilon);
}

TEST(AabbUVETest, IntersectRayUVE_DirectHitFromOutside_ReturnsDistanceAndNormal) {
    const AabbUVE box{Vector3UVE{-1.0F, -1.0F, -1.0F}, Vector3UVE{1.0F, 1.0F, 1.0F}};
    const RayUVE ray{Vector3UVE{-5.0F, 0.0F, 0.0F}, Vector3UVE{1.0F, 0.0F, 0.0F}};

    const std::optional<RayHitUVE> hit = IntersectRayUVE(ray, box, 100.0F);

    ASSERT_TRUE(hit.has_value());
    EXPECT_NEAR(hit->distance, 4.0F, kEpsilon);
    EXPECT_NEAR(hit->normal.x, -1.0F, kEpsilon);
    EXPECT_NEAR(hit->normal.y, 0.0F, kEpsilon);
    EXPECT_NEAR(hit->normal.z, 0.0F, kEpsilon);
}

TEST(AabbUVETest, IntersectRayUVE_HitFromBelow_ReturnsMinusYNormal) {
    const AabbUVE box{Vector3UVE{-1.0F, -1.0F, -1.0F}, Vector3UVE{1.0F, 1.0F, 1.0F}};
    const RayUVE ray{Vector3UVE{0.0F, -5.0F, 0.0F}, Vector3UVE{0.0F, 1.0F, 0.0F}};

    const std::optional<RayHitUVE> hit = IntersectRayUVE(ray, box, 100.0F);

    ASSERT_TRUE(hit.has_value());
    EXPECT_NEAR(hit->distance, 4.0F, kEpsilon);
    EXPECT_NEAR(hit->normal.y, -1.0F, kEpsilon);
}

TEST(AabbUVETest, IntersectRayUVE_PointingAwayFromBox_ReturnsNullopt) {
    const AabbUVE box{Vector3UVE{-1.0F, -1.0F, -1.0F}, Vector3UVE{1.0F, 1.0F, 1.0F}};
    const RayUVE ray{Vector3UVE{-5.0F, 0.0F, 0.0F}, Vector3UVE{-1.0F, 0.0F, 0.0F}};

    EXPECT_FALSE(IntersectRayUVE(ray, box, 100.0F).has_value());
}

TEST(AabbUVETest, IntersectRayUVE_OriginInsideBox_ReturnsZeroDistance) {
    const AabbUVE box{Vector3UVE{-1.0F, -1.0F, -1.0F}, Vector3UVE{1.0F, 1.0F, 1.0F}};
    const RayUVE ray{Vector3UVE{0.0F, 0.0F, 0.0F}, Vector3UVE{1.0F, 0.0F, 0.0F}};

    const std::optional<RayHitUVE> hit = IntersectRayUVE(ray, box, 100.0F);

    ASSERT_TRUE(hit.has_value());
    EXPECT_NEAR(hit->distance, 0.0F, kEpsilon);
}

TEST(AabbUVETest, IntersectRayUVE_ParallelToTwoSlabsWithinRange_StillHitsWithoutNaN) {
    // direction.x == 0 and direction.y == 0 — exercises the dir-is-zero branch for two axes.
    const AabbUVE box{Vector3UVE{-1.0F, -1.0F, -1.0F}, Vector3UVE{1.0F, 1.0F, 1.0F}};
    const RayUVE ray{Vector3UVE{0.0F, 0.0F, -10.0F}, Vector3UVE{0.0F, 0.0F, 1.0F}};

    const std::optional<RayHitUVE> hit = IntersectRayUVE(ray, box, 100.0F);

    ASSERT_TRUE(hit.has_value());
    EXPECT_TRUE(std::isfinite(hit->distance));
    EXPECT_NEAR(hit->distance, 9.0F, kEpsilon);
    EXPECT_NEAR(hit->normal.z, -1.0F, kEpsilon);
}

TEST(AabbUVETest, IntersectRayUVE_ParallelToSlabOutsideRange_ReturnsNulloptWithoutNaN) {
    // direction.x == 0, but origin.x is outside the box's x-slab entirely — must miss cleanly.
    const AabbUVE box{Vector3UVE{-1.0F, -1.0F, -1.0F}, Vector3UVE{1.0F, 1.0F, 1.0F}};
    const RayUVE ray{Vector3UVE{5.0F, 0.0F, -10.0F}, Vector3UVE{0.0F, 0.0F, 1.0F}};

    EXPECT_FALSE(IntersectRayUVE(ray, box, 100.0F).has_value());
}

TEST(AabbUVETest, IntersectRayUVE_HitBeyondMaxDistance_ReturnsNullopt) {
    const AabbUVE box{Vector3UVE{-1.0F, -1.0F, -1.0F}, Vector3UVE{1.0F, 1.0F, 1.0F}};
    const RayUVE ray{Vector3UVE{-5.0F, 0.0F, 0.0F}, Vector3UVE{1.0F, 0.0F, 0.0F}};

    EXPECT_FALSE(IntersectRayUVE(ray, box, 2.0F).has_value()); // box starts at distance 4
}

TEST(AabbUVETest, ToStringUVE_ContainsMinAndMax) {
    const AabbUVE box{Vector3UVE{0.0F, 0.0F, 0.0F}, Vector3UVE{1.0F, 2.0F, 3.0F}};
    const std::string text = ToStringUVE(box);

    EXPECT_NE(text.find("3.000000"), std::string::npos);
}

} // namespace
} // namespace UVE::Math::Tests
