// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <cmath>
#include <cstddef>

#include <gtest/gtest.h>

#include "uve/render/primitive_geometry_uve.h"

namespace UVE::Render::Tests {
namespace {

[[nodiscard]] float TriangleTwiceAreaSquaredUVE(const Asset::MeshVertexUVE& first,
                                                const Asset::MeshVertexUVE& second,
                                                const Asset::MeshVertexUVE& third) noexcept {
    return Math::LengthSquaredUVE(Math::CrossUVE(second.position - first.position, third.position - first.position));
}

[[nodiscard]] bool IsFiniteUVE(const Math::Vector3UVE& vector) noexcept {
    return std::isfinite(vector.x) && std::isfinite(vector.y) && std::isfinite(vector.z);
}

} // namespace

TEST(PrimitiveGeometryUVETest, CubeAndPlane_HaveDeterministicTopologyBoundsAndOutwardFaces) {
    const PrimitiveGeometryUVE& cube = GetPrimitiveGeometryUVE(Scene::PrimitiveMeshKindUVE::Cube);
    EXPECT_EQ(cube.vertices.size(), 24U);
    EXPECT_EQ(cube.indices.size(), 36U);
    EXPECT_EQ(cube.localBounds.min, (Math::Vector3UVE{-0.5F, -0.5F, -0.5F}));
    EXPECT_EQ(cube.localBounds.max, (Math::Vector3UVE{0.5F, 0.5F, 0.5F}));

    const PrimitiveGeometryUVE& plane = GetPrimitiveGeometryUVE(Scene::PrimitiveMeshKindUVE::Plane);
    EXPECT_EQ(plane.vertices.size(), 4U);
    EXPECT_EQ(plane.indices.size(), 6U);
    EXPECT_EQ(plane.localBounds.min, (Math::Vector3UVE{-0.5F, -0.000001F, -0.5F}));
    EXPECT_EQ(plane.localBounds.max, (Math::Vector3UVE{0.5F, 0.000001F, 0.5F}));

    for (const PrimitiveGeometryUVE* geometry : {&cube, &plane}) {
        for (std::size_t index = 0U; index < geometry->indices.size(); index += 3U) {
            const Asset::MeshVertexUVE& first = geometry->vertices[geometry->indices[index]];
            const Asset::MeshVertexUVE& second = geometry->vertices[geometry->indices[index + 1U]];
            const Asset::MeshVertexUVE& third = geometry->vertices[geometry->indices[index + 2U]];
            EXPECT_GT(TriangleTwiceAreaSquaredUVE(first, second, third), 0.00000001F);
            const Math::Vector3UVE faceNormal = Math::CrossUVE(second.position - first.position, third.position - first.position);
            EXPECT_GT(Math::DotUVE(faceNormal, first.normal + second.normal + third.normal), 0.0F);
        }
    }
}

TEST(PrimitiveGeometryUVETest, UVSphere_HasStableSeamPolesUnitNormalsAndNonDegenerateOutwardTopology) {
    const PrimitiveGeometryUVE& sphere = GetPrimitiveGeometryUVE(Scene::PrimitiveMeshKindUVE::UVSphere);
    constexpr std::size_t kSlicesUVE = 24U;
    constexpr std::size_t kStacksUVE = 16U;
    constexpr std::size_t kColumnsUVE = kSlicesUVE + 1U;
    EXPECT_EQ(sphere.vertices.size(), kColumnsUVE * (kStacksUVE + 1U));
    EXPECT_EQ(sphere.indices.size(), 6U * kSlicesUVE * (kStacksUVE - 1U));
    EXPECT_EQ(sphere.localBounds.min, (Math::Vector3UVE{-0.5F, -0.5F, -0.5F}));
    EXPECT_EQ(sphere.localBounds.max, (Math::Vector3UVE{0.5F, 0.5F, 0.5F}));

    for (std::size_t stack = 0U; stack <= kStacksUVE; ++stack) {
        const Asset::MeshVertexUVE& seamStart = sphere.vertices[stack * kColumnsUVE];
        const Asset::MeshVertexUVE& seamEnd = sphere.vertices[stack * kColumnsUVE + kSlicesUVE];
        EXPECT_EQ(seamStart.position, seamEnd.position);
        EXPECT_EQ(seamStart.normal, seamEnd.normal);
        EXPECT_EQ(seamStart.u, 0.0F);
        EXPECT_EQ(seamEnd.u, 1.0F);
    }

    for (std::size_t slice = 0U; slice <= kSlicesUVE; ++slice) {
        const Asset::MeshVertexUVE& northPole = sphere.vertices[slice];
        const Asset::MeshVertexUVE& southPole = sphere.vertices[kStacksUVE * kColumnsUVE + slice];
        EXPECT_EQ(northPole.position, (Math::Vector3UVE{0.0F, 0.5F, 0.0F}));
        EXPECT_EQ(northPole.normal, (Math::Vector3UVE{0.0F, 1.0F, 0.0F}));
        EXPECT_EQ(southPole.position, (Math::Vector3UVE{0.0F, -0.5F, 0.0F}));
        EXPECT_EQ(southPole.normal, (Math::Vector3UVE{0.0F, -1.0F, 0.0F}));
    }

    for (const Asset::MeshVertexUVE& vertex : sphere.vertices) {
        EXPECT_TRUE(IsFiniteUVE(vertex.position));
        EXPECT_TRUE(IsFiniteUVE(vertex.normal));
        EXPECT_TRUE(std::isfinite(vertex.u));
        EXPECT_TRUE(std::isfinite(vertex.v));
        EXPECT_NEAR(Math::LengthSquaredUVE(vertex.normal), 1.0F, 0.00001F);
    }
    for (std::size_t index = 0U; index < sphere.indices.size(); index += 3U) {
        const Asset::MeshVertexUVE& first = sphere.vertices[sphere.indices[index]];
        const Asset::MeshVertexUVE& second = sphere.vertices[sphere.indices[index + 1U]];
        const Asset::MeshVertexUVE& third = sphere.vertices[sphere.indices[index + 2U]];
        EXPECT_GT(TriangleTwiceAreaSquaredUVE(first, second, third), 0.00000001F);
        const Math::Vector3UVE faceNormal = Math::CrossUVE(second.position - first.position, third.position - first.position);
        EXPECT_GT(Math::DotUVE(faceNormal, first.position + second.position + third.position), 0.0F);
    }
}

} // namespace UVE::Render::Tests
