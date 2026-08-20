// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/asset_importer_uve.h"

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>

#include <gtest/gtest.h>
#include <jpeglib.h>

#include "uve/asset/asset_database_uve.h"
#include "uve/asset/texture_asset_uve.h"

namespace UVE::Asset::Tests {
namespace {

[[nodiscard]] std::vector<std::byte> MakeOneByOneJpegUVE(const bool progressive = false) {
    jpeg_compress_struct compressor{};
    jpeg_error_mgr error{};
    jpeg_std_error(&error);
    compressor.err = &error;
    jpeg_create_compress(&compressor);
    unsigned char* encoded = nullptr;
    unsigned long encodedSize = 0UL;
    jpeg_mem_dest(&compressor, &encoded, &encodedSize);
    compressor.image_width = 1U;
    compressor.image_height = 1U;
    compressor.input_components = 3;
    compressor.in_color_space = JCS_RGB;
    jpeg_set_defaults(&compressor);
    if (progressive) jpeg_simple_progression(&compressor);
    jpeg_set_quality(&compressor, 100, TRUE);
    jpeg_start_compress(&compressor, TRUE);
    JSAMPLE pixel[3] = {255U, 0U, 0U};
    JSAMPROW row = pixel;
    jpeg_write_scanlines(&compressor, &row, 1U);
    jpeg_finish_compress(&compressor);
    std::vector<std::byte> result(reinterpret_cast<std::byte*>(encoded),
                                   reinterpret_cast<std::byte*>(encoded) + encodedSize);
    std::free(encoded);
    jpeg_destroy_compress(&compressor);
    return result;
}

void WriteBytesUVE(const std::filesystem::path& path, const std::vector<std::byte>& bytes) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(output.is_open());
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    ASSERT_TRUE(output.good());
}

void RemoveFilesUVE(const std::initializer_list<std::filesystem::path>& paths) {
    for (const auto& path : paths) std::filesystem::remove(path);
}

TEST(JpegImporterUVETest, ImportUVE_ValidJpgPublishesUveTexAndRegistersGuid) {
    const std::filesystem::path sourcePath = "uve_jpeg_importer_tests_red.jpg";
    const std::filesystem::path destinationPath = "uve_jpeg_importer_tests_red.uvetex";
    RemoveFilesUVE({sourcePath, destinationPath});
    WriteBytesUVE(sourcePath, MakeOneByOneJpegUVE());

    AssetImporterUVE importer;
    AssetDatabaseUVE database;
    const auto classification = importer.ClassifySourceUVE(sourcePath);
    EXPECT_EQ(classification.kind, AssetImportSourceKindUVE::RawTexture);
    EXPECT_TRUE(classification.importerRegistered);
    EXPECT_TRUE(classification.requiresFormatSpecificParser);
    EXPECT_EQ(classification.diagnostic, "format-specific parser is registered");
    const AssetGuidUVE guid = importer.ImportUVE(sourcePath, destinationPath, database);

    ASSERT_NE(guid, kInvalidAssetGuidUVE);
    EXPECT_EQ(database.ResolveUVE(guid), destinationPath);
    TextureAssetUVE texture;
    ASSERT_TRUE(LoadTextureAssetUVE(destinationPath, texture));
    EXPECT_EQ(texture.width, 1U);
    EXPECT_EQ(texture.height, 1U);
    EXPECT_EQ(texture.format, TextureFormatUVE::RGBA8Unorm);
    ASSERT_EQ(texture.pixels.size(), 4U);
    EXPECT_GT(std::to_integer<unsigned int>(texture.pixels[0]), 200U);
    EXPECT_LT(std::to_integer<unsigned int>(texture.pixels[1]), 50U);
    EXPECT_EQ(texture.pixels[3], std::byte{0xFF});
    RemoveFilesUVE({sourcePath, destinationPath});
}

TEST(JpegImporterUVETest, ImportUVE_ValidProgressiveJpegPublishesThroughJpegExtension) {
    const std::filesystem::path sourcePath = "uve_jpeg_importer_tests_progressive.jpeg";
    const std::filesystem::path destinationPath = "uve_jpeg_importer_tests_progressive.uvetex";
    RemoveFilesUVE({sourcePath, destinationPath});
    WriteBytesUVE(sourcePath, MakeOneByOneJpegUVE(true));

    AssetImporterUVE importer;
    AssetDatabaseUVE database;
    const auto classification = importer.ClassifySourceUVE(sourcePath);
    EXPECT_TRUE(classification.importerRegistered);
    const AssetGuidUVE guid = importer.ImportUVE(sourcePath, destinationPath, database);

    ASSERT_NE(guid, kInvalidAssetGuidUVE);
    TextureAssetUVE texture;
    ASSERT_TRUE(LoadTextureAssetUVE(destinationPath, texture));
    EXPECT_EQ(texture.width, 1U);
    EXPECT_EQ(texture.height, 1U);
    EXPECT_EQ(texture.pixels.size(), 4U);
    RemoveFilesUVE({sourcePath, destinationPath});
}

TEST(JpegImporterUVETest, ImportUVE_InvalidJpegPreservesExistingDestinationAndDoesNotRegister) {
    const std::filesystem::path sourcePath = "uve_jpeg_importer_tests_invalid.jpg";
    const std::filesystem::path destinationPath = "uve_jpeg_importer_tests_existing.uvetex";
    RemoveFilesUVE({sourcePath, destinationPath});
    WriteBytesUVE(sourcePath, {std::byte{0xFF}, std::byte{0xD8}, std::byte{0x00}});
    TextureAssetUVE original;
    original.width = 1U;
    original.height = 1U;
    original.format = TextureFormatUVE::RGBA8Unorm;
    original.pixels = {std::byte{0x11}, std::byte{0x22}, std::byte{0x33}, std::byte{0xFF}};
    ASSERT_TRUE(SaveTextureAssetUVE(original, destinationPath));

    AssetImporterUVE importer;
    AssetDatabaseUVE database;
    const AssetGuidUVE guid = importer.ImportUVE(sourcePath, destinationPath, database);

    EXPECT_EQ(guid, kInvalidAssetGuidUVE);
    EXPECT_TRUE(database.ResolveUVE(guid).empty());
    TextureAssetUVE retained;
    ASSERT_TRUE(LoadTextureAssetUVE(destinationPath, retained));
    EXPECT_EQ(retained.width, original.width);
    EXPECT_EQ(retained.height, original.height);
    EXPECT_EQ(retained.pixels, original.pixels);
    RemoveFilesUVE({sourcePath, destinationPath});
}

TEST(JpegImporterUVETest, ImportUVE_WrongDestinationExtensionFailsBeforePublish) {
    const std::filesystem::path sourcePath = "uve_jpeg_importer_tests_wrong_destination.jpg";
    const std::filesystem::path destinationPath = "uve_jpeg_importer_tests_wrong_destination.uvemodel";
    RemoveFilesUVE({sourcePath, destinationPath});
    WriteBytesUVE(sourcePath, MakeOneByOneJpegUVE());

    AssetImporterUVE importer;
    AssetDatabaseUVE database;
    EXPECT_EQ(importer.ImportUVE(sourcePath, destinationPath, database), kInvalidAssetGuidUVE);
    EXPECT_FALSE(std::filesystem::exists(destinationPath));
    RemoveFilesUVE({sourcePath, destinationPath});
}

} // namespace
} // namespace UVE::Asset::Tests
