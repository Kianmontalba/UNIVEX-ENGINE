// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/asset/mtl_metadata_uve.h"
#include <string>
#include <gtest/gtest.h>
namespace UVE::Asset::Tests {
TEST(MtlMetadataUVETest, ValidateMtlTextureReferenceUVE_AcceptsBoundedRelativePaths) {
    EXPECT_TRUE(ValidateMtlTextureReferenceUVE("textures/brick.png"));
    EXPECT_TRUE(ValidateMtlTextureReferenceUVE("brick.png"));
    EXPECT_TRUE(ValidateMtlTextureReferenceUVE(std::string(kMaximumMtlTextureReferenceBytesUVE, 'a')));
}

TEST(MtlMetadataUVETest, ValidateMtlTextureReferenceUVE_RejectsUnsafeReferences) {
    EXPECT_FALSE(ValidateMtlTextureReferenceUVE(""));
    EXPECT_FALSE(ValidateMtlTextureReferenceUVE("/textures/brick.png"));
    EXPECT_FALSE(ValidateMtlTextureReferenceUVE("C:/textures/brick.png"));
    EXPECT_FALSE(ValidateMtlTextureReferenceUVE("textures/../brick.png"));
    EXPECT_FALSE(ValidateMtlTextureReferenceUVE("textures//brick.png"));
    EXPECT_FALSE(ValidateMtlTextureReferenceUVE(std::string(kMaximumMtlTextureReferenceBytesUVE + 1U, 'a')));
    EXPECT_FALSE(ValidateMtlTextureReferenceUVE(std::string("brick\0.png", 9U)));
}

TEST(MtlMetadataUVETest, ParseMtlMaterialPropertyUVE_CopiesScalarVectorAndTextureFacts) {
    MtlMaterialPropertyUVE property;
    ASSERT_TRUE(ParseMtlMaterialPropertyUVE("Ns 32.5", property));
    EXPECT_EQ(property.kind, MtlMaterialPropertyKindUVE::Scalar);
    EXPECT_FLOAT_EQ(property.scalarValue, 32.5F);
    ASSERT_TRUE(ParseMtlMaterialPropertyUVE("Kd 1.0 0.5 0.25", property));
    EXPECT_EQ(property.kind, MtlMaterialPropertyKindUVE::Vector3);
    EXPECT_FLOAT_EQ(property.vectorValue[0], 1.0F);
    EXPECT_FLOAT_EQ(property.vectorValue[1], 0.5F);
    EXPECT_FLOAT_EQ(property.vectorValue[2], 0.25F);
    ASSERT_TRUE(ParseMtlMaterialPropertyUVE("map_Kd textures/brick.png", property));
    EXPECT_EQ(property.kind, MtlMaterialPropertyKindUVE::TextureReference);
    EXPECT_EQ(property.textureReference, "textures/brick.png");
}

TEST(MtlMetadataUVETest, ParseMtlMaterialPropertyUVE_RejectsInvalidAndExtraTokensAtomically) {
    const MtlMaterialPropertyUVE original{MtlMaterialPropertyKindUVE::Scalar, 7.0F, {1.0F, 2.0F, 3.0F}, "stable.png"};
    MtlMaterialPropertyUVE property = original;
    EXPECT_FALSE(ParseMtlMaterialPropertyUVE("Kd 1 0", property));
    EXPECT_EQ(property, original);
    EXPECT_FALSE(ParseMtlMaterialPropertyUVE("Ns nan", property));
    EXPECT_EQ(property, original);
    EXPECT_FALSE(ParseMtlMaterialPropertyUVE("map_Kd ../brick.png", property));
    EXPECT_EQ(property, original);
    EXPECT_FALSE(ParseMtlMaterialPropertyUVE("map_Kd -s 1 1 1 brick.png", property));
    EXPECT_EQ(property, original);
    EXPECT_FALSE(ParseMtlMaterialPropertyUVE("Kd 1 0 0 extra", property));
    EXPECT_EQ(property, original);
}

TEST(MtlMetadataUVETest, ParseMtlTextureMapUVE_ParsesBoundedOptionsAndDefaults) {
    MtlTextureMapUVE map;
    ASSERT_TRUE(ParseMtlTextureMapUVE("map_Kd -s 2 3 4 -o -1 0.5 0 -clamp on textures/brick.png", map));
    EXPECT_EQ(map.textureReference, "textures/brick.png");
    EXPECT_EQ(map.scale, (std::array<float, 3U>{2.0F, 3.0F, 4.0F}));
    EXPECT_EQ(map.offset, (std::array<float, 3U>{-1.0F, 0.5F, 0.0F}));
    EXPECT_TRUE(map.clamp);
    ASSERT_TRUE(ParseMtlTextureMapUVE("map_Bump brick_n.png", map));
    EXPECT_EQ(map.textureReference, "brick_n.png");
    EXPECT_EQ(map.scale, (std::array<float, 3U>{1.0F, 1.0F, 1.0F}));
    EXPECT_EQ(map.offset, (std::array<float, 3U>{0.0F, 0.0F, 0.0F}));
    EXPECT_FALSE(map.clamp);
}

TEST(MtlMetadataUVETest, ParseMtlTextureMapUVE_RejectsInvalidOptionsAtomically) {
    const MtlTextureMapUVE original{"stable.png", {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, false};
    MtlTextureMapUVE map = original;
    EXPECT_FALSE(ParseMtlTextureMapUVE("map_Kd -s 1 2 textures/brick.png", map));
    EXPECT_EQ(map, original);
    EXPECT_FALSE(ParseMtlTextureMapUVE("map_Kd -clamp maybe brick.png", map));
    EXPECT_EQ(map, original);
    EXPECT_FALSE(ParseMtlTextureMapUVE("map_Kd -s nan 1 1 brick.png", map));
    EXPECT_EQ(map, original);
    EXPECT_FALSE(ParseMtlTextureMapUVE("map_Kd ../brick.png", map));
    EXPECT_EQ(map, original);
    EXPECT_FALSE(ParseMtlTextureMapUVE("map_Kd brick.png other.png", map));
    EXPECT_EQ(map, original);
    EXPECT_FALSE(ParseMtlTextureMapUVE("map_Kd -unknown brick.png", map));
    EXPECT_EQ(map, original);
}

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

TEST(MtlMetadataUVETest, ParseMtlMetadataUVE_RejectsTrailingTokensOnRecognizedDeclarations) {
    EXPECT_FALSE(ParseMtlMetadataUVE("newmtl Brick extra\n").has_value());
    EXPECT_FALSE(ParseMtlMetadataUVE("newmtl Brick\nKd 1 0.5 0.2 extra\n").has_value());
    EXPECT_FALSE(ParseMtlMetadataUVE("newmtl Brick\nNs 32 extra\n").has_value());
    EXPECT_FALSE(ParseMtlMetadataUVE("newmtl Brick\nmap_Kd brick.png other.png\n").has_value());
}
}
