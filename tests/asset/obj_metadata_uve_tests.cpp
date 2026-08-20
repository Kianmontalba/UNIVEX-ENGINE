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
