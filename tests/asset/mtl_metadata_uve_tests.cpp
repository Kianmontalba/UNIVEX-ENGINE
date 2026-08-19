// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/asset/mtl_metadata_uve.h"
#include <gtest/gtest.h>
namespace UVE::Asset::Tests {
TEST(MtlMetadataUVETest, ParseMtlMetadataUVE_CountsMaterialPropertiesAndMaps) {
    const auto metadata = ParseMtlMetadataUVE("newmtl Brick\nKd 1 0.5 0.2\nNs 32\nmap_Kd brick.png\nmap_Bump brick_n.png\nillum 2\n");
    ASSERT_TRUE(metadata.has_value()); EXPECT_EQ(metadata->materialCount,1U); EXPECT_EQ(metadata->textureMapCount,2U);
    EXPECT_EQ(metadata->vectorPropertyCount,1U); EXPECT_EQ(metadata->scalarPropertyCount,2U); EXPECT_EQ(metadata->ignoredStatementCount,0U);
}
TEST(MtlMetadataUVETest, ParseMtlMetadataUVE_CountsIgnoredDirectives) {
    const auto metadata = ParseMtlMetadataUVE("# comment\nnewmtl A\nfoo custom\n"); ASSERT_TRUE(metadata.has_value());
    EXPECT_EQ(metadata->materialCount,1U); EXPECT_EQ(metadata->ignoredStatementCount,1U);
}
TEST(MtlMetadataUVETest, ParseMtlMetadataUVE_RejectsMissingRequiredTokens) {
    EXPECT_FALSE(ParseMtlMetadataUVE("newmtl\n").has_value()); EXPECT_FALSE(ParseMtlMetadataUVE("Kd 1 0\n").has_value());
    EXPECT_FALSE(ParseMtlMetadataUVE("map_Kd\n").has_value());
}
}
