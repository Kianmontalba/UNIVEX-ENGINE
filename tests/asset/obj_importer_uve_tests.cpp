// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/asset_importer_uve.h"

#include <filesystem>
#include <fstream>
#include <string_view>

#include <gtest/gtest.h>

#include "uve/asset/asset_database_uve.h"
#include "uve/asset/mesh_asset_uve.h"

namespace UVE::Asset::Tests {
namespace {

void WriteObjFixtureUVE(const std::filesystem::path& path, const std::string_view source) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(output.is_open());
    output.write(source.data(), static_cast<std::streamsize>(source.size()));
    ASSERT_TRUE(output.good());
}

[[nodiscard]] MeshAssetUVE MakeExistingMeshUVE() {
    MeshAssetUVE mesh;
    mesh.vertices = {
        MeshVertexUVE{Math::Vector3UVE{4.0F, 5.0F, 6.0F}, Math::Vector3UVE{0.0F, 1.0F, 0.0F}, 0.0F, 0.0F},
        MeshVertexUVE{Math::Vector3UVE{5.0F, 5.0F, 6.0F}, Math::Vector3UVE{0.0F, 1.0F, 0.0F}, 1.0F, 0.0F},
        MeshVertexUVE{Math::Vector3UVE{4.0F, 6.0F, 6.0F}, Math::Vector3UVE{0.0F, 1.0F, 0.0F}, 0.0F, 1.0F},
    };
    mesh.indices = {0U, 1U, 2U};
    mesh.localBounds = Math::AabbUVE{Math::Vector3UVE{4.0F, 5.0F, 6.0F}, Math::Vector3UVE{5.0F, 6.0F, 6.0F}};
    return mesh;
}

TEST(ObjImporterUVETest, ImportUVE_ValidObjPublishesUveModelAndRegistersGuid) {
    const std::filesystem::path sourcePath = "uve_obj_importer_tests_triangle.obj";
    const std::filesystem::path destinationPath = "uve_obj_importer_tests_triangle.uvemodel";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    WriteObjFixtureUVE(sourcePath, "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n");

    AssetImporterUVE importer;
    AssetDatabaseUVE database;
    const AssetGuidUVE guid = importer.ImportUVE(sourcePath, destinationPath, database);

    ASSERT_NE(guid, kInvalidAssetGuidUVE);
    EXPECT_EQ(database.ResolveUVE(guid), destinationPath);
    MeshAssetUVE mesh;
    ASSERT_TRUE(LoadMeshAssetUVE(destinationPath, mesh));
    ASSERT_EQ(mesh.vertices.size(), 3U);
    EXPECT_EQ(mesh.indices, (std::vector<std::uint32_t>{0U, 1U, 2U}));
    EXPECT_EQ(mesh.localBounds,
              (Math::AabbUVE{Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 0.0F}}));

    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
}

TEST(ObjImporterUVETest, ImportUVE_InvalidObjPreservesExistingDestinationAndDoesNotRegister) {
    const std::filesystem::path sourcePath = "uve_obj_importer_tests_invalid.obj";
    const std::filesystem::path destinationPath = "uve_obj_importer_tests_existing.uvemodel";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    WriteObjFixtureUVE(sourcePath, "v 0 0 0\nf 1 2 3\n");
    const MeshAssetUVE original = MakeExistingMeshUVE();
    ASSERT_TRUE(SaveMeshAssetUVE(original, destinationPath));

    AssetImporterUVE importer;
    AssetDatabaseUVE database;
    const AssetGuidUVE guid = importer.ImportUVE(sourcePath, destinationPath, database);

    EXPECT_EQ(guid, kInvalidAssetGuidUVE);
    EXPECT_TRUE(database.ResolveUVE(guid).empty());
    MeshAssetUVE retained;
    ASSERT_TRUE(LoadMeshAssetUVE(destinationPath, retained));
    EXPECT_EQ(retained.indices, original.indices);
    EXPECT_EQ(retained.localBounds, original.localBounds);

    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
}

TEST(ObjImporterUVETest, ImportUVE_WrongDestinationExtensionFailsBeforePublish) {
    const std::filesystem::path sourcePath = "uve_obj_importer_tests_wrong_destination.obj";
    const std::filesystem::path destinationPath = "uve_obj_importer_tests_wrong_destination.uvemodel.tmp";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    WriteObjFixtureUVE(sourcePath, "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n");

    AssetImporterUVE importer;
    AssetDatabaseUVE database;
    EXPECT_EQ(importer.ImportUVE(sourcePath, destinationPath, database), kInvalidAssetGuidUVE);
    EXPECT_FALSE(std::filesystem::exists(destinationPath));

    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
}

} // namespace
} // namespace UVE::Asset::Tests
