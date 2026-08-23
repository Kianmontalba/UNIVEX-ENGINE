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

void AppendU32LittleEndianUVE(std::vector<std::byte>& bytes, const std::uint32_t value) {
    for (unsigned int shift = 0U; shift < 32U; shift += 8U) {
        bytes.push_back(std::byte{static_cast<unsigned char>((value >> shift) & 0xFFU)});
    }
}

[[nodiscard]] std::vector<std::byte> MakeBmp24OneByOneUVE() {
    std::vector<std::byte> bmp;
    bmp.reserve(58U);
    bmp.push_back(std::byte{'B'});
    bmp.push_back(std::byte{'M'});
    AppendU32LittleEndianUVE(bmp, 58U);
    AppendU16LittleEndianUVE(bmp, 0U);
    AppendU16LittleEndianUVE(bmp, 0U);
    AppendU32LittleEndianUVE(bmp, 54U);
    AppendU32LittleEndianUVE(bmp, 40U);
    AppendU32LittleEndianUVE(bmp, 1U);
    AppendU32LittleEndianUVE(bmp, 1U);
    AppendU16LittleEndianUVE(bmp, 1U);
    AppendU16LittleEndianUVE(bmp, 24U);
    AppendU32LittleEndianUVE(bmp, 0U);
    AppendU32LittleEndianUVE(bmp, 4U);
    AppendU32LittleEndianUVE(bmp, 2835U);
    AppendU32LittleEndianUVE(bmp, 2835U);
    AppendU32LittleEndianUVE(bmp, 0U);
    AppendU32LittleEndianUVE(bmp, 0U);
    bmp.insert(bmp.end(), {std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}});
    return bmp;
}

void WriteBytesUVE(const std::filesystem::path& path, const std::vector<std::byte>& bytes) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(output.is_open());
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    ASSERT_TRUE(output.good());
}

TEST(BmpImporterUVETest, ImportUVE_DecodesBmpToTextureEnvelopeAndRegistersGuid) {
    const std::filesystem::path sourcePath = "uve_bmp_importer_source.bmp";
    const std::filesystem::path destinationPath = "uve_bmp_importer_destination.uvetex";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    WriteBytesUVE(sourcePath, MakeBmp24OneByOneUVE());

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

TEST(BmpImporterUVETest, ImportUVE_InvalidBmpPreservesExistingDestination) {
    const std::filesystem::path sourcePath = "uve_bmp_importer_invalid_source.bmp";
    const std::filesystem::path destinationPath = "uve_bmp_importer_existing_destination.uvetex";
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

TEST(BmpImporterUVETest, ImportUVE_WrongDestinationExtensionFailsBeforePublish) {
    const std::filesystem::path sourcePath = "uve_bmp_importer_wrong_destination.bmp";
    const std::filesystem::path destinationPath = "uve_bmp_importer_wrong_destination.uvemodel";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    WriteBytesUVE(sourcePath, MakeBmp24OneByOneUVE());

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
