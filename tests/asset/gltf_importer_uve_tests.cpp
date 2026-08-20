// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/asset_importer_uve.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/asset_database_uve.h"
#include "uve/asset/mesh_asset_uve.h"

namespace UVE::Asset::Tests {
namespace {

void AppendU32LittleEndianUVE(std::vector<std::byte>& bytes, const std::uint32_t value) {
    for (unsigned int shift = 0U; shift < 32U; shift += 8U) {
        bytes.push_back(std::byte{static_cast<unsigned char>((value >> shift) & 0xFFU)});
    }
}

void AppendU16LittleEndianUVE(std::vector<std::byte>& bytes, const std::uint16_t value) {
    bytes.push_back(std::byte{static_cast<unsigned char>(value & 0xFFU)});
    bytes.push_back(std::byte{static_cast<unsigned char>((value >> 8U) & 0xFFU)});
}

void AppendFloatLittleEndianUVE(std::vector<std::byte>& bytes, const float value) {
    const std::uint32_t bits = std::bit_cast<std::uint32_t>(value);
    AppendU32LittleEndianUVE(bytes, bits);
}

void AppendVector3UVE(std::vector<std::byte>& bytes, const float x, const float y, const float z) {
    AppendFloatLittleEndianUVE(bytes, x);
    AppendFloatLittleEndianUVE(bytes, y);
    AppendFloatLittleEndianUVE(bytes, z);
}

[[nodiscard]] std::vector<std::byte> MakeTriangleBufferUVE() {
    std::vector<std::byte> bytes;
    AppendVector3UVE(bytes, 0.0F, 0.0F, 0.0F);
    AppendVector3UVE(bytes, 1.0F, 0.0F, 0.0F);
    AppendVector3UVE(bytes, 0.0F, 1.0F, 0.0F);
    AppendVector3UVE(bytes, 0.0F, 0.0F, 1.0F);
    AppendVector3UVE(bytes, 0.0F, 0.0F, 1.0F);
    AppendVector3UVE(bytes, 0.0F, 0.0F, 1.0F);
    for (const float value : {0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F}) {
        AppendFloatLittleEndianUVE(bytes, value);
    }
    AppendU16LittleEndianUVE(bytes, 0U);
    AppendU16LittleEndianUVE(bytes, 1U);
    AppendU16LittleEndianUVE(bytes, 2U);
    return bytes;
}

[[nodiscard]] std::string MakeGltfJsonUVE(const std::string_view uri) {
    const std::string bufferUri = uri.empty() ? std::string{} : "\"uri\":\"" + std::string(uri) + "\",\n    ";
    return "{\n"
           "  \"asset\": {\"version\": \"2.0\"},\n"
           "  \"buffers\": [{" + bufferUri + "\"byteLength\": 102}],\n"
           "  \"bufferViews\": [\n"
           "    {\"buffer\":0,\"byteOffset\":0,\"byteLength\":36},\n"
           "    {\"buffer\":0,\"byteOffset\":36,\"byteLength\":36},\n"
           "    {\"buffer\":0,\"byteOffset\":72,\"byteLength\":24},\n"
           "    {\"buffer\":0,\"byteOffset\":96,\"byteLength\":6}\n"
           "  ],\n"
           "  \"accessors\": [\n"
           "    {\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},\n"
           "    {\"bufferView\":1,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},\n"
           "    {\"bufferView\":2,\"componentType\":5126,\"count\":3,\"type\":\"VEC2\"},\n"
           "    {\"bufferView\":3,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"}\n"
           "  ],\n"
           "  \"meshes\": [{\"primitives\":[{\"attributes\":{\"POSITION\":0,\"NORMAL\":1,\"TEXCOORD_0\":2},\"indices\":3,\"mode\":4}]}]\n"
           "}\n";
}

void WriteBytesUVE(const std::filesystem::path& path, const std::vector<std::byte>& bytes) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(output.is_open());
    if (!bytes.empty()) {
        output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
    ASSERT_TRUE(output.good());
}

void WriteTextUVE(const std::filesystem::path& path, const std::string_view text) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(output.is_open());
    output.write(text.data(), static_cast<std::streamsize>(text.size()));
    ASSERT_TRUE(output.good());
}

void WriteGlbUVE(const std::filesystem::path& path, const std::string& json,
                 const std::vector<std::byte>& binary) {
    std::vector<std::byte> bytes;
    std::vector<std::byte> jsonBytes(reinterpret_cast<const std::byte*>(json.data()),
                                     reinterpret_cast<const std::byte*>(json.data() + json.size()));
    while (jsonBytes.size() % 4U != 0U) jsonBytes.push_back(std::byte{' '});
    AppendU32LittleEndianUVE(bytes, 0x46546C67U);
    AppendU32LittleEndianUVE(bytes, 2U);
    AppendU32LittleEndianUVE(bytes, static_cast<std::uint32_t>(12U + 8U + jsonBytes.size() + 8U + binary.size()));
    AppendU32LittleEndianUVE(bytes, static_cast<std::uint32_t>(jsonBytes.size()));
    AppendU32LittleEndianUVE(bytes, 0x4E4F534AU);
    bytes.insert(bytes.end(), jsonBytes.begin(), jsonBytes.end());
    AppendU32LittleEndianUVE(bytes, static_cast<std::uint32_t>(binary.size()));
    AppendU32LittleEndianUVE(bytes, 0x004E4942U);
    bytes.insert(bytes.end(), binary.begin(), binary.end());
    WriteBytesUVE(path, bytes);
}

void RemoveFilesUVE(const std::initializer_list<std::filesystem::path>& paths) {
    for (const auto& path : paths) std::filesystem::remove(path);
}

TEST(GltfImporterUVETest, ImportUVE_ValidExternalBufferPublishesUveModelAndRegistersGuid) {
    const std::filesystem::path sourcePath = "uve_gltf_importer_tests_triangle.gltf";
    const std::filesystem::path bufferPath = "uve_gltf_importer_tests_triangle.bin";
    const std::filesystem::path destinationPath = "uve_gltf_importer_tests_triangle.uvemodel";
    RemoveFilesUVE({sourcePath, bufferPath, destinationPath});
    const auto binary = MakeTriangleBufferUVE();
    WriteBytesUVE(bufferPath, binary);
    WriteTextUVE(sourcePath, MakeGltfJsonUVE(bufferPath.filename().string()));

    AssetImporterUVE importer;
    AssetDatabaseUVE database;
    const auto classification = importer.ClassifySourceUVE(sourcePath);
    EXPECT_EQ(classification.kind, AssetImportSourceKindUVE::RawModel);
    EXPECT_TRUE(classification.importerRegistered);
    EXPECT_TRUE(classification.requiresFormatSpecificParser);
    EXPECT_EQ(classification.diagnostic, "format-specific parser is registered");
    const AssetGuidUVE guid = importer.ImportUVE(sourcePath, destinationPath, database);

    ASSERT_NE(guid, kInvalidAssetGuidUVE);
    EXPECT_EQ(database.ResolveUVE(guid), destinationPath);
    MeshAssetUVE mesh;
    ASSERT_TRUE(LoadMeshAssetUVE(destinationPath, mesh));
    ASSERT_EQ(mesh.vertices.size(), 3U);
    EXPECT_EQ(mesh.indices, (std::vector<std::uint32_t>{0U, 1U, 2U}));
    EXPECT_EQ(mesh.vertices[0].normal, (Math::Vector3UVE{0.0F, 0.0F, 1.0F}));
    EXPECT_EQ(mesh.localBounds,
              (Math::AabbUVE{Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 0.0F}}));
    RemoveFilesUVE({sourcePath, bufferPath, destinationPath});
}

TEST(GltfImporterUVETest, ImportUVE_ValidGlbPublishesUveModelAndRegistersBothExtensions) {
    const std::filesystem::path sourcePath = "uve_gltf_importer_tests_triangle.glb";
    const std::filesystem::path destinationPath = "uve_gltf_importer_tests_triangle_glb.uvemodel";
    RemoveFilesUVE({sourcePath, destinationPath});
    const auto binary = MakeTriangleBufferUVE();
    WriteGlbUVE(sourcePath, MakeGltfJsonUVE(""), binary);

    AssetImporterUVE importer;
    AssetDatabaseUVE database;
    const auto glbClassification = importer.ClassifySourceUVE(sourcePath);
    EXPECT_TRUE(glbClassification.importerRegistered);
    EXPECT_EQ(glbClassification.diagnostic, "format-specific parser is registered");
    const auto gltfClassification = importer.ClassifySourceUVE("fixture.gltf");
    EXPECT_TRUE(gltfClassification.importerRegistered);
    const AssetGuidUVE guid = importer.ImportUVE(sourcePath, destinationPath, database);

    ASSERT_NE(guid, kInvalidAssetGuidUVE);
    MeshAssetUVE mesh;
    ASSERT_TRUE(LoadMeshAssetUVE(destinationPath, mesh));
    EXPECT_EQ(mesh.indices, (std::vector<std::uint32_t>{0U, 1U, 2U}));
    RemoveFilesUVE({sourcePath, destinationPath});
}

TEST(GltfImporterUVETest, ImportUVE_InvalidSourcePreservesExistingDestinationAndDoesNotRegister) {
    const std::filesystem::path sourcePath = "uve_gltf_importer_tests_invalid.gltf";
    const std::filesystem::path destinationPath = "uve_gltf_importer_tests_existing.uvemodel";
    RemoveFilesUVE({sourcePath, destinationPath});
    WriteTextUVE(sourcePath, "{\"asset\":{\"version\":\"2.0\"},\"buffers\":[]}");
    MeshAssetUVE original;
    original.vertices = {MeshVertexUVE{Math::Vector3UVE{4.0F, 5.0F, 6.0F}, Math::Vector3UVE{0.0F, 1.0F, 0.0F}, 0.0F,
                                       0.0F}};
    original.indices = {0U};
    original.localBounds = Math::AabbUVE{Math::Vector3UVE{4.0F, 5.0F, 6.0F}, Math::Vector3UVE{4.0F, 5.0F, 6.0F}};
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
    RemoveFilesUVE({sourcePath, destinationPath});
}

TEST(GltfImporterUVETest, ImportUVE_WrongDestinationExtensionFailsBeforePublish) {
    const std::filesystem::path sourcePath = "uve_gltf_importer_tests_wrong_destination.gltf";
    const std::filesystem::path destinationPath = "uve_gltf_importer_tests_wrong_destination.uvemat";
    RemoveFilesUVE({sourcePath, destinationPath});
    WriteTextUVE(sourcePath, MakeGltfJsonUVE("missing.bin"));

    AssetImporterUVE importer;
    AssetDatabaseUVE database;
    EXPECT_EQ(importer.ImportUVE(sourcePath, destinationPath, database), kInvalidAssetGuidUVE);
    EXPECT_FALSE(std::filesystem::exists(destinationPath));
    RemoveFilesUVE({sourcePath, destinationPath});
}

} // namespace
} // namespace UVE::Asset::Tests
