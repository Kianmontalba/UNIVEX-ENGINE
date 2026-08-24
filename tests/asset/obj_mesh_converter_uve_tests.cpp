// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/obj_mesh_converter_uve.h"

#include <string_view>

#include <gtest/gtest.h>

namespace UVE::Asset::Tests {
namespace {

TEST(ObjMeshConverterUVETest, ConvertObjMeshUVE_ExplicitAttributesPublishesTriangleMesh) {
    constexpr std::string_view source = R"OBJ(
# one textured triangle
v 0 0 0
v 1 0 0
v 0 1 0
vt 0 0
vt 1 0
vt 0 1
vn 0 0 1
f 1/1/1 2/2/1 3/3/1
)OBJ";

    MeshAssetUVE mesh;
    ASSERT_TRUE(ConvertObjMeshUVE(source, mesh));
    ASSERT_EQ(mesh.vertices.size(), 3U);
    EXPECT_EQ(mesh.indices, (std::vector<std::uint32_t>{0U, 1U, 2U}));
    EXPECT_EQ(mesh.vertices[0].position, (Math::Vector3UVE{0.0F, 0.0F, 0.0F}));
    EXPECT_EQ(mesh.vertices[1].position, (Math::Vector3UVE{1.0F, 0.0F, 0.0F}));
    EXPECT_EQ(mesh.vertices[2].position, (Math::Vector3UVE{0.0F, 1.0F, 0.0F}));
    EXPECT_FLOAT_EQ(mesh.vertices[1].u, 1.0F);
    EXPECT_FLOAT_EQ(mesh.vertices[2].v, 1.0F);
    EXPECT_EQ(mesh.vertices[0].normal, (Math::Vector3UVE{0.0F, 0.0F, 1.0F}));
    EXPECT_EQ(mesh.localBounds,
              (Math::AabbUVE{Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 0.0F}}));
}

TEST(ObjMeshConverterUVETest, ConvertObjMeshUVE_AcceptsFiniteHomogeneousPositionWeight) {
    constexpr std::string_view source = R"OBJ(
v 0 0 0 1
v 2 0 0 2
v 0 2 0 2
f 1 2 3
)OBJ";

    MeshAssetUVE mesh;
    ASSERT_TRUE(ConvertObjMeshUVE(source, mesh));
    ASSERT_EQ(mesh.vertices.size(), 3U);
    EXPECT_EQ(mesh.vertices[1].position, (Math::Vector3UVE{1.0F, 0.0F, 0.0F}));
    EXPECT_EQ(mesh.vertices[2].position, (Math::Vector3UVE{0.0F, 1.0F, 0.0F}));
}

TEST(ObjMeshConverterUVETest, ConvertObjMeshUVE_RejectsTrailingPositionCoordinateAtomically) {
    MeshAssetUVE original;
    original.vertices = {
        MeshVertexUVE{Math::Vector3UVE{4.0F, 5.0F, 6.0F}, Math::Vector3UVE{0.0F, 1.0F, 0.0F}, 0.0F, 0.0F}};
    original.indices = {0U};
    original.localBounds = Math::AabbUVE{Math::Vector3UVE{4.0F, 5.0F, 6.0F}, Math::Vector3UVE{4.0F, 5.0F, 6.0F}};
    MeshAssetUVE output = original;

    constexpr std::string_view invalidSource = R"OBJ(
v 0 0 0 1 99
v 1 0 0
v 0 1 0
f 1 2 3
)OBJ";
    EXPECT_FALSE(ConvertObjMeshUVE(invalidSource, output));
    EXPECT_EQ(output.vertices.size(), original.vertices.size());
    EXPECT_EQ(output.vertices[0].position, original.vertices[0].position);
    EXPECT_EQ(output.indices, original.indices);
    EXPECT_EQ(output.localBounds, original.localBounds);
}

TEST(ObjMeshConverterUVETest, ConvertObjMeshUVE_NegativeIndexQuadFanComputesFaceNormals) {
    constexpr std::string_view source = R"OBJ(
v 0 0 0
v 1 0 0
v 1 1 0
v 0 1 0
f -4 -3 -2 -1
)OBJ";

    MeshAssetUVE mesh;
    ASSERT_TRUE(ConvertObjMeshUVE(source, mesh));
    ASSERT_EQ(mesh.vertices.size(), 6U);
    EXPECT_EQ(mesh.indices, (std::vector<std::uint32_t>{0U, 1U, 2U, 3U, 4U, 5U}));
    for (const MeshVertexUVE& vertex : mesh.vertices) {
        EXPECT_EQ(vertex.normal, (Math::Vector3UVE{0.0F, 0.0F, 1.0F}));
        EXPECT_NEAR(Math::LengthUVE(vertex.tangent), 1.0F, 0.0001F);
    }
    EXPECT_EQ(mesh.localBounds,
              (Math::AabbUVE{Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 0.0F}}));
}

TEST(ObjMeshConverterUVETest, ConvertObjMeshUVE_InvalidInputPreservesExistingOutput) {
    MeshAssetUVE original;
    original.vertices = {
        MeshVertexUVE{Math::Vector3UVE{4.0F, 5.0F, 6.0F}, Math::Vector3UVE{0.0F, 1.0F, 0.0F}, 0.0F, 0.0F}};
    original.indices = {0U};
    original.localBounds = Math::AabbUVE{Math::Vector3UVE{4.0F, 5.0F, 6.0F}, Math::Vector3UVE{4.0F, 5.0F, 6.0F}};
    MeshAssetUVE output = original;

    constexpr std::string_view invalidSource = "v 0 0 0\nf 1 2 3\n";
    EXPECT_FALSE(ConvertObjMeshUVE(invalidSource, output));
    EXPECT_EQ(output.vertices.size(), original.vertices.size());
    EXPECT_EQ(output.vertices[0].position, original.vertices[0].position);
    EXPECT_EQ(output.indices, original.indices);
    EXPECT_EQ(output.localBounds, original.localBounds);
}

TEST(ObjMeshConverterUVETest, ConvertObjMeshUVE_DegenerateOrNonFiniteInputFails) {
    MeshAssetUVE mesh;
    EXPECT_FALSE(ConvertObjMeshUVE("v nan 0 0\nf 1 1 1\n", mesh));
    EXPECT_FALSE(ConvertObjMeshUVE("v 0 0 0\nv 1 0 0\nv 2 0 0\nf 1 2 3\n", mesh));
}

TEST(ObjMeshConverterUVETest, ConvertObjMeshUVE_RejectsFiniteNormalAndEdgeOverflowWithoutPublishing) {
    MeshAssetUVE original;
    original.vertices = {
        MeshVertexUVE{Math::Vector3UVE{4.0F, 5.0F, 6.0F}, Math::Vector3UVE{0.0F, 1.0F, 0.0F}, 0.0F, 0.0F}};
    original.indices = {0U};
    original.localBounds = Math::AabbUVE{Math::Vector3UVE{4.0F, 5.0F, 6.0F}, Math::Vector3UVE{4.0F, 5.0F, 6.0F}};

    MeshAssetUVE normalOverflow = original;
    EXPECT_FALSE(ConvertObjMeshUVE(
        "v 0 0 0\nv 1 0 0\nv 0 1 0\nvn 3.402823466e38 3.402823466e38 0\nf 1/1 2/1 3/1\n",
        normalOverflow));
    EXPECT_EQ(normalOverflow.vertices.size(), original.vertices.size());
    EXPECT_EQ(normalOverflow.vertices[0].position, original.vertices[0].position);

    MeshAssetUVE edgeOverflow = original;
    EXPECT_FALSE(ConvertObjMeshUVE(
        "v -3.402823466e38 0 0\nv 3.402823466e38 0 0\nv 0 1 0\nf 1 2 3\n",
        edgeOverflow));
    EXPECT_EQ(edgeOverflow.vertices.size(), original.vertices.size());
    EXPECT_EQ(edgeOverflow.vertices[0].position, original.vertices[0].position);
}

} // namespace
} // namespace UVE::Asset::Tests
