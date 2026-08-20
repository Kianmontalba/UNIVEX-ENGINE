// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/asset_importer_uve.h"

#include <filesystem>
#include <fstream>
#include <string_view>

#include <gtest/gtest.h>

#include "uve/asset/asset_database_uve.h"
#include "uve/asset/material_asset_uve.h"

namespace UVE::Asset::Tests {
namespace {

void WriteMtlFixtureUVE(const std::filesystem::path& path, const std::string_view source) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(output.is_open());
    output.write(source.data(), static_cast<std::streamsize>(source.size()));
    ASSERT_TRUE(output.good());
}

[[nodiscard]] MaterialAssetUVE MakeExistingMaterialUVE() {
    MaterialAssetUVE material;
    material.albedoColor = Math::Vector3UVE{0.3F, 0.2F, 0.1F};
    material.metallic = 0.8F;
    material.roughness = 0.2F;
    material.isTransparent = true;
    return material;
}

TEST(MtlImporterUVETest, ImportUVE_ValidMtlPublishesUveMatAndRegistersGuid) {
    const std::filesystem::path sourcePath = "uve_mtl_importer_tests_material.mtl";
    const std::filesystem::path destinationPath = "uve_mtl_importer_tests_material.uvemat";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    WriteMtlFixtureUVE(sourcePath, "newmtl Painted\nKd 0.2 0.4 0.6\nKs 0.75 0.75 0.75\nNs 500\n");

    AssetImporterUVE importer;
    AssetDatabaseUVE database;
    const AssetGuidUVE guid = importer.ImportUVE(sourcePath, destinationPath, database);

    ASSERT_NE(guid, kInvalidAssetGuidUVE);
    EXPECT_EQ(database.ResolveUVE(guid), destinationPath);
    MaterialAssetUVE material;
    ASSERT_TRUE(LoadMaterialAssetUVE(destinationPath, material));
    EXPECT_EQ(material.albedoColor, (Math::Vector3UVE{0.2F, 0.4F, 0.6F}));
    EXPECT_FLOAT_EQ(material.metallic, 0.75F);
    EXPECT_FLOAT_EQ(material.roughness, 0.5F);
    EXPECT_FALSE(material.isTransparent);
    EXPECT_EQ(material.albedoTexture, kInvalidAssetGuidUVE);

    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
}

TEST(MtlImporterUVETest, ImportUVE_InvalidMtlPreservesExistingDestinationAndDoesNotRegister) {
    const std::filesystem::path sourcePath = "uve_mtl_importer_tests_invalid.mtl";
    const std::filesystem::path destinationPath = "uve_mtl_importer_tests_existing.uvemat";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    WriteMtlFixtureUVE(sourcePath, "newmtl Broken\nd 1.5\n");
    const MaterialAssetUVE original = MakeExistingMaterialUVE();
    ASSERT_TRUE(SaveMaterialAssetUVE(original, destinationPath));

    AssetImporterUVE importer;
    AssetDatabaseUVE database;
    const AssetGuidUVE guid = importer.ImportUVE(sourcePath, destinationPath, database);

    EXPECT_EQ(guid, kInvalidAssetGuidUVE);
    EXPECT_TRUE(database.ResolveUVE(guid).empty());
    MaterialAssetUVE retained;
    ASSERT_TRUE(LoadMaterialAssetUVE(destinationPath, retained));
    EXPECT_EQ(retained.albedoColor, original.albedoColor);
    EXPECT_FLOAT_EQ(retained.metallic, original.metallic);
    EXPECT_EQ(retained.isTransparent, original.isTransparent);

    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
}

TEST(MtlImporterUVETest, ImportUVE_WrongDestinationExtensionFailsBeforePublish) {
    const std::filesystem::path sourcePath = "uve_mtl_importer_tests_wrong_destination.mtl";
    const std::filesystem::path destinationPath = "uve_mtl_importer_tests_wrong_destination.uvemat.tmp";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    WriteMtlFixtureUVE(sourcePath, "newmtl Valid\nKd 1 1 1\n");

    AssetImporterUVE importer;
    AssetDatabaseUVE database;
    EXPECT_EQ(importer.ImportUVE(sourcePath, destinationPath, database), kInvalidAssetGuidUVE);
    EXPECT_FALSE(std::filesystem::exists(destinationPath));

    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
}

} // namespace
} // namespace UVE::Asset::Tests
