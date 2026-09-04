// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <cmath>
#include <cstddef>

#include <gtest/gtest.h>

#include "uve/render/gizmo_overlay_geometry_uve.h"

namespace UVE::Render::Tests {
namespace {

constexpr std::size_t kSidesUVE = 14U;
constexpr float kShaftLengthUVE = 0.72F;
constexpr float kConeHeightUVE = 0.28F;
constexpr float kConeRadiusUVE = 0.09F;

[[nodiscard]] bool IsFiniteUVE(const Math::Vector3UVE& vector) noexcept {
    return std::isfinite(vector.x) && std::isfinite(vector.y) && std::isfinite(vector.z);
}

[[nodiscard]] float TriangleTwiceAreaSquaredUVE(const Asset::MeshVertexUVE& first,
                                                const Asset::MeshVertexUVE& second,
                                                const Asset::MeshVertexUVE& third) noexcept {
    return Math::LengthSquaredUVE(Math::CrossUVE(second.position - first.position, third.position - first.position));
}

} // namespace

TEST(GizmoOverlayGeometryUVETest, HasDeterministicUnindexedTopologyAndBounds) {
    const PrimitiveGeometryUVE& arrow = GetGizmoArrowGeometryUVE();
    // Per side: 2 shaft triangles + 1 cone-side triangle + 1 cap triangle = 4 triangles, each
    // AddFlatTriangleUVE call appends exactly 3 fresh (unshared) vertices and 3 sequential indices.
    constexpr std::size_t kTrianglesPerSideUVE = 4U;
    const std::size_t expectedTriangles = kSidesUVE * kTrianglesPerSideUVE;
    EXPECT_EQ(arrow.vertices.size(), expectedTriangles * 3U);
    EXPECT_EQ(arrow.indices.size(), expectedTriangles * 3U);
    EXPECT_EQ(arrow.localBounds.min, (Math::Vector3UVE{-kConeRadiusUVE, -kConeRadiusUVE, 0.0F}));
    EXPECT_EQ(arrow.localBounds.max,
              (Math::Vector3UVE{kConeRadiusUVE, kConeRadiusUVE, kShaftLengthUVE + kConeHeightUVE}));
}

TEST(GizmoOverlayGeometryUVETest, EveryVertexIsFiniteWithAUnitNormal) {
    const PrimitiveGeometryUVE& arrow = GetGizmoArrowGeometryUVE();
    for (const Asset::MeshVertexUVE& vertex : arrow.vertices) {
        EXPECT_TRUE(IsFiniteUVE(vertex.position));
        EXPECT_TRUE(IsFiniteUVE(vertex.normal));
        EXPECT_NEAR(Math::LengthSquaredUVE(vertex.normal), 1.0F, 0.0001F);
    }
}

TEST(GizmoOverlayGeometryUVETest, EveryTriangleIsNonDegenerateAndOutwardFacing) {
    const PrimitiveGeometryUVE& arrow = GetGizmoArrowGeometryUVE();
    for (std::size_t index = 0U; index < arrow.indices.size(); index += 3U) {
        const Asset::MeshVertexUVE& first = arrow.vertices[arrow.indices[index]];
        const Asset::MeshVertexUVE& second = arrow.vertices[arrow.indices[index + 1U]];
        const Asset::MeshVertexUVE& third = arrow.vertices[arrow.indices[index + 2U]];
        EXPECT_GT(TriangleTwiceAreaSquaredUVE(first, second, third), 0.00000001F);

        const float radialFirst = std::sqrt((first.position.x * first.position.x) + (first.position.y * first.position.y));
        const bool isCapTriangle = radialFirst < 0.0001F; // the cap fan's shared apex sits on the Z axis
        if (isCapTriangle) {
            // The cone base cap closes the underside of the flare - it must face away from the
            // solid cone body above it, i.e. back toward the shaft (-Z).
            EXPECT_LT(first.normal.z, -0.9F);
            continue;
        }
        // Every shaft/cone-side triangle's normal must point away from the arrow's own Z axis
        // (the direction from the axis to the triangle is well-approximated by its own x/y
        // position, since the arrow itself is centered on that axis).
        const Math::Vector3UVE radialDirection{first.position.x, first.position.y, 0.0F};
        EXPECT_GT(Math::DotUVE(first.normal, radialDirection), 0.0F);
    }
}

} // namespace UVE::Render::Tests
