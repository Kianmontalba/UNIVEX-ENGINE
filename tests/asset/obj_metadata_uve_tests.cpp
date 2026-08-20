// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/obj_metadata_uve.h"

#include <cstdint>
#include <limits>
#include <string_view>

#include <gtest/gtest.h>

namespace UVE::Asset::Tests {
namespace {

TEST(ObjMetadataUVETest, ResolveObjIndexUVE_MapsPositiveAndNegativeObjIndices) {
    std::uint32_t resolved = 99U;
    EXPECT_TRUE(ResolveObjIndexUVE(1, 4U, resolved));
    EXPECT_EQ(resolved, 0U);
    EXPECT_TRUE(ResolveObjIndexUVE(4, 4U, resolved));
    EXPECT_EQ(resolved, 3U);
    EXPECT_TRUE(ResolveObjIndexUVE(-1, 4U, resolved));
    EXPECT_EQ(resolved, 3U);
    EXPECT_TRUE(ResolveObjIndexUVE(-4, 4U, resolved));
    EXPECT_EQ(resolved, 0U);
}

TEST(ObjMetadataUVETest, ResolveObjFaceVertexUVE_ResolvesSupportedTokenForms) {
    ObjFaceVertexUVE vertex;
    ASSERT_TRUE(ResolveObjFaceVertexUVE("1/2/3", 4U, 4U, 4U, vertex));
    EXPECT_EQ(vertex.positionIndex, 0U);
    ASSERT_TRUE(vertex.texcoordIndex.has_value());
    EXPECT_EQ(*vertex.texcoordIndex, 1U);
    ASSERT_TRUE(vertex.normalIndex.has_value());
    EXPECT_EQ(*vertex.normalIndex, 2U);
    ASSERT_TRUE(ResolveObjFaceVertexUVE("-1/-2/-3", 4U, 4U, 4U, vertex));
    EXPECT_EQ(vertex.positionIndex, 3U);
    EXPECT_EQ(*vertex.texcoordIndex, 2U);
    EXPECT_EQ(*vertex.normalIndex, 1U);
    ASSERT_TRUE(ResolveObjFaceVertexUVE("1//3", 4U, 0U, 4U, vertex));
    EXPECT_EQ(vertex.positionIndex, 0U);
    EXPECT_FALSE(vertex.texcoordIndex.has_value());
    ASSERT_TRUE(vertex.normalIndex.has_value());
    EXPECT_EQ(*vertex.normalIndex, 2U);
    ASSERT_TRUE(ResolveObjFaceVertexUVE("2/3", 4U, 4U, 0U, vertex));
    EXPECT_EQ(vertex.positionIndex, 1U);
    ASSERT_TRUE(vertex.texcoordIndex.has_value());
    EXPECT_EQ(*vertex.texcoordIndex, 2U);
    EXPECT_FALSE(vertex.normalIndex.has_value());
}

TEST(ObjMetadataUVETest, ResolveObjFaceVertexUVE_RejectsMalformedAndOutOfRangeTokensAtomically) {
    const ObjFaceVertexUVE original{7U, 8U, 9U};
    ObjFaceVertexUVE vertex = original;
    EXPECT_FALSE(ResolveObjFaceVertexUVE("", 4U, 4U, 4U, vertex));
    EXPECT_EQ(vertex, original);
    EXPECT_FALSE(ResolveObjFaceVertexUVE("1/", 4U, 4U, 4U, vertex));
    EXPECT_EQ(vertex, original);
    EXPECT_FALSE(ResolveObjFaceVertexUVE("1//", 4U, 4U, 4U, vertex));
    EXPECT_EQ(vertex, original);
    EXPECT_FALSE(ResolveObjFaceVertexUVE("1/2/3/4", 4U, 4U, 4U, vertex));
    EXPECT_EQ(vertex, original);
    EXPECT_FALSE(ResolveObjFaceVertexUVE("0/1/1", 4U, 4U, 4U, vertex));
    EXPECT_EQ(vertex, original);
    EXPECT_FALSE(ResolveObjFaceVertexUVE("1/5/1", 4U, 4U, 4U, vertex));
    EXPECT_EQ(vertex, original);
    EXPECT_FALSE(ResolveObjFaceVertexUVE("1/a/1", 4U, 4U, 4U, vertex));
    EXPECT_EQ(vertex, original);
}

TEST(ObjMetadataUVETest, ResolveObjIndexUVE_RejectsInvalidAndExtremeIndicesAtomically) {
    std::uint32_t resolved = 77U;
    EXPECT_FALSE(ResolveObjIndexUVE(0, 4U, resolved));
    EXPECT_EQ(resolved, 77U);
    EXPECT_FALSE(ResolveObjIndexUVE(5, 4U, resolved));
    EXPECT_EQ(resolved, 77U);
    EXPECT_FALSE(ResolveObjIndexUVE(-5, 4U, resolved));
    EXPECT_EQ(resolved, 77U);
    EXPECT_FALSE(ResolveObjIndexUVE(1, 0U, resolved));
    EXPECT_FALSE(ResolveObjIndexUVE(std::numeric_limits<std::int64_t>::min(), 4U, resolved));
    EXPECT_EQ(resolved, 77U);
}

TEST(ObjMetadataUVETest, ParseObjMetadataUVE_CountsDeclarationsAndTriangulatesPolygons) {
    constexpr std::string_view source =
        "# quad\n"
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 1 1 0\n"
        "v 0 1 0\n"
        "vt 0 0\n"
        "vt 1 0\n"
        "vt 1 1\n"
        "vt 0 1\n"
        "vn 0 0 1\n"
        "g Quad\n"
        "usemtl Floor\n"
        "mtllib floor.mtl\n"
        "f 1/1/1 2/2/1 3/3/1 4/4/1\n";
    const std::optional<ObjMetadataUVE> metadata = ParseObjMetadataUVE(source);
    ASSERT_TRUE(metadata.has_value());
    EXPECT_EQ(metadata->positionCount, 4U);
    EXPECT_EQ(metadata->texcoordCount, 4U);
    EXPECT_EQ(metadata->normalCount, 1U);
    EXPECT_EQ(metadata->faceCount, 1U);
    EXPECT_EQ(metadata->triangleCount, 2U);
    EXPECT_EQ(metadata->groupCount, 1U);
    EXPECT_EQ(metadata->materialUseCount, 1U);
    EXPECT_EQ(metadata->materialLibraryCount, 1U);
    EXPECT_EQ(metadata->ignoredStatementCount, 0U);
}

TEST(ObjMetadataUVETest, ParseObjMetadataUVE_AllowsTwoComponentUvAndCountsIgnoredDirectives) {
    const std::optional<ObjMetadataUVE> metadata = ParseObjMetadataUVE(
        "v 0 0 0\nvt 0.5 0.25\nusemtl M\n# comment\ns off\n");
    ASSERT_TRUE(metadata.has_value());
    EXPECT_EQ(metadata->positionCount, 1U);
    EXPECT_EQ(metadata->texcoordCount, 1U);
    EXPECT_EQ(metadata->materialUseCount, 1U);
    EXPECT_EQ(metadata->ignoredStatementCount, 1U);
}

TEST(ObjMetadataUVETest, ParseObjMetadataUVE_RejectsMalformedDeclarationsAndFaces) {
    EXPECT_FALSE(ParseObjMetadataUVE("v 0 0\n").has_value());
    EXPECT_FALSE(ParseObjMetadataUVE("f 1/1 2/2\n").has_value());
    EXPECT_FALSE(ParseObjMetadataUVE("f 1// 2//2 3//3\n").has_value());
}

} // namespace
} // namespace UVE::Asset::Tests
