// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/mtl_material_converter_uve.h"

#include <string_view>

#include <gtest/gtest.h>

#include "uve/asset/asset_guid_uve.h"

namespace UVE::Asset::Tests {
namespace {

TEST(MtlMaterialConverterUVETest, ConvertMtlMaterialUVE_MapsBoundedPbrProperties) {
    constexpr std::string_view source = R"MTL(
newmtl PaintedMetal
Kd 0.2 0.4 0.6
Ka 0.1 0.1 0.1
Ks 0.8 0.6 0.4
Ke 1.0 2.0 3.0
Ns 500
Ni 1.5
d 0.5
illum 2
map_Kd -s 1 1 1 -o 0 0 0 -clamp on albedo.png
)MTL";

    MaterialAssetUVE material;
    ASSERT_TRUE(ConvertMtlMaterialUVE(source, material));
    EXPECT_EQ(material.albedoColor, (Math::Vector3UVE{0.2F, 0.4F, 0.6F}));
    EXPECT_EQ(material.emissiveColor, (Math::Vector3UVE{1.0F, 2.0F, 3.0F}));
    EXPECT_FLOAT_EQ(material.metallic, 0.6F);
    EXPECT_FLOAT_EQ(material.roughness, 0.5F);
    EXPECT_TRUE(material.isTransparent);
    EXPECT_EQ(material.albedoTexture, kInvalidAssetGuidUVE);
    EXPECT_EQ(material.normalTexture, kInvalidAssetGuidUVE);
    EXPECT_EQ(material.aoTexture, kInvalidAssetGuidUVE);
    EXPECT_EQ(material.vertexShader, kInvalidAssetGuidUVE);
    EXPECT_EQ(material.fragmentShader, kInvalidAssetGuidUVE);
}

TEST(MtlMaterialConverterUVETest, ConvertMtlMaterialUVE_MapReferencesRemainUnresolved) {
    constexpr std::string_view source = "newmtl Neutral\nmap_Ka normal.png\nmap_Bump -clamp off normal.png\n";

    MaterialAssetUVE material;
    ASSERT_TRUE(ConvertMtlMaterialUVE(source, material));
    EXPECT_EQ(material.albedoTexture, kInvalidAssetGuidUVE);
    EXPECT_EQ(material.normalTexture, kInvalidAssetGuidUVE);
    EXPECT_EQ(material.aoTexture, kInvalidAssetGuidUVE);
}

TEST(MtlMaterialConverterUVETest, ConvertMtlMaterialUVE_InvalidInputPreservesExistingOutput) {
    MaterialAssetUVE original;
    original.albedoColor = Math::Vector3UVE{0.3F, 0.2F, 0.1F};
    original.metallic = 0.8F;
    original.isTransparent = true;
    MaterialAssetUVE output = original;

    constexpr std::string_view invalidSource = "newmtl Broken\nKd 1 0\n";
    EXPECT_FALSE(ConvertMtlMaterialUVE(invalidSource, output));
    EXPECT_EQ(output.albedoColor, original.albedoColor);
    EXPECT_FLOAT_EQ(output.metallic, original.metallic);
    EXPECT_EQ(output.isTransparent, original.isTransparent);
}

TEST(MtlMaterialConverterUVETest, ConvertMtlMaterialUVE_RejectsInvalidDissolveAndMultipleMaterials) {
    MaterialAssetUVE material;
    EXPECT_FALSE(ConvertMtlMaterialUVE("newmtl Broken\nd 1.5\n", material));
    EXPECT_FALSE(ConvertMtlMaterialUVE("newmtl First\nnewmtl Second\n", material));
    EXPECT_FALSE(ConvertMtlMaterialUVE("Kd 1 1 1\n", material));
}

} // namespace
} // namespace UVE::Asset::Tests
