// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/asset_database_uve.h"
#include "uve/asset/asset_importer_uve.h"
#include "uve/asset/texture_asset_uve.h"

namespace UVE::Asset::Tests {
namespace {

void AppendU16LittleEndianUVE(std::vector<std::byte>& bytes, const std::uint16_t value) {
    bytes.push_back(std::byte{static_cast<unsigned char>(value & 0xFFU)});
    bytes.push_back(std::byte{static_cast<unsigned char>((value >> 8U) & 0xFFU)});
}

[[nodiscard]] std::vector<std::byte> MakeTga24OneByOneUVE() {
    std::vector<std::byte> tga;
    tga.reserve(21U);
    tga.insert(tga.end(), 12U, std::byte{0});
    tga[2] = std::byte{2};
    AppendU16LittleEndianUVE(tga, 1U);
    AppendU16LittleEndianUVE(tga, 1U);
    tga.push_back(std::byte{24});
    tga.push_back(std::byte{0});
    tga.insert(tga.end(), {std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}});
    return tga;
}

void WriteBytesUVE(const std::filesystem::path& path, const std::vector<std::byte>& bytes) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(output.is_open());
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    ASSERT_TRUE(output.good());
}

TEST(TgaImporterUVETest, ImportUVE_DecodesTgaToTextureEnvelopeAndRegistersGuid) {
    const std::filesystem::path sourcePath = "uve_tga_importer_source.tga";
    const std::filesystem::path destinationPath = "uve_tga_importer_destination.uvetex";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    WriteBytesUVE(sourcePath, MakeTga24OneByOneUVE());

    AssetImporterUVE importer;
    AssetDatabaseUVE assetDatabase;
    const AssetGuidUVE guid = importer.ImportUVE(sourcePath, destinationPath, assetDatabase);

    ASSERT_NE(guid, kInvalidAssetGuidUVE);
    EXPECT_EQ(assetDatabase.ResolveUVE(guid), destinationPath);
    TextureAssetUVE texture;
    ASSERT_TRUE(LoadTextureAssetUVE(destinationPath, texture));
    EXPECT_EQ(texture.width, 1U);
    EXPECT_EQ(texture.height, 1U);
    EXPECT_EQ(texture.format, TextureFormatUVE::RGBA8Unorm);
    EXPECT_EQ(texture.pixels, (std::vector<std::byte>{std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}}));

    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
}

TEST(TgaImporterUVETest, ImportUVE_InvalidTgaPreservesExistingDestination) {
    const std::filesystem::path sourcePath = "uve_tga_importer_invalid_source.tga";
    const std::filesystem::path destinationPath = "uve_tga_importer_existing_destination.uvetex";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    WriteBytesUVE(sourcePath, {std::byte{0x01}, std::byte{0x02}, std::byte{0x03}});
    TextureAssetUVE existing;
    existing.width = 1U;
    existing.height = 1U;
    existing.pixels = {std::byte{0x11}, std::byte{0x22}, std::byte{0x33}, std::byte{0x44}};
    ASSERT_TRUE(SaveTextureAssetUVE(existing, destinationPath));

    AssetImporterUVE importer;
    AssetDatabaseUVE assetDatabase;
    EXPECT_EQ(importer.ImportUVE(sourcePath, destinationPath, assetDatabase), kInvalidAssetGuidUVE);
    TextureAssetUVE retained;
    ASSERT_TRUE(LoadTextureAssetUVE(destinationPath, retained));
    EXPECT_EQ(retained.pixels, existing.pixels);
    EXPECT_TRUE(assetDatabase.GetRegisteredAssetsUVE().empty());

    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
}

TEST(TgaImporterUVETest, ImportUVE_WrongDestinationExtensionFailsBeforePublish) {
    const std::filesystem::path sourcePath = "uve_tga_importer_wrong_destination.tga";
    const std::filesystem::path destinationPath = "uve_tga_importer_wrong_destination.uvemodel";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    WriteBytesUVE(sourcePath, MakeTga24OneByOneUVE());

    AssetImporterUVE importer;
    AssetDatabaseUVE assetDatabase;
    EXPECT_EQ(importer.ImportUVE(sourcePath, destinationPath, assetDatabase), kInvalidAssetGuidUVE);
    EXPECT_FALSE(std::filesystem::exists(destinationPath));
    EXPECT_TRUE(assetDatabase.GetRegisteredAssetsUVE().empty());

    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
}

} // namespace
} // namespace UVE::Asset::Tests
