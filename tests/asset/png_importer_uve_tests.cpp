#include "uve/asset/asset_importer_uve.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

#include <gtest/gtest.h>
#include <zlib.h>

#include "uve/asset/asset_database_uve.h"
#include "uve/asset/texture_asset_uve.h"

namespace UVE::Asset::Tests {
namespace {

void AppendU32BEUVE(std::vector<std::byte>& bytes, const std::uint32_t value) {
    for (unsigned int shift = 24U;; shift -= 8U) {
        bytes.push_back(std::byte{static_cast<unsigned char>((value >> shift) & 0xFFU)});
        if (shift == 0U) break;
    }
}

void AppendPngChunkUVE(std::vector<std::byte>& png, const std::vector<std::byte>& type,
                       const std::vector<std::byte>& payload) {
    AppendU32BEUVE(png, static_cast<std::uint32_t>(payload.size()));
    png.insert(png.end(), type.begin(), type.end());
    png.insert(png.end(), payload.begin(), payload.end());
    std::vector<std::byte> crcInput(type);
    crcInput.insert(crcInput.end(), payload.begin(), payload.end());
    const uLong crc = crc32(0L, reinterpret_cast<const Bytef*>(crcInput.data()), static_cast<uInt>(crcInput.size()));
    AppendU32BEUVE(png, static_cast<std::uint32_t>(crc));
}

[[nodiscard]] std::vector<std::byte> MakePngTwoByOneRgba8UVE() {
    std::vector<std::byte> png{
        std::byte{0x89}, std::byte{0x50}, std::byte{0x4E}, std::byte{0x47}, std::byte{0x0D}, std::byte{0x0A},
        std::byte{0x1A}, std::byte{0x0A}};
    AppendPngChunkUVE(png, {std::byte{'I'}, std::byte{'H'}, std::byte{'D'}, std::byte{'R'}},
                      {std::byte{0}, std::byte{0}, std::byte{0}, std::byte{2}, std::byte{0}, std::byte{0},
                       std::byte{0}, std::byte{1}, std::byte{8}, std::byte{6}, std::byte{0}, std::byte{0}, std::byte{0}});
    const std::vector<std::byte> raw{std::byte{0}, std::byte{0xFF}, std::byte{0}, std::byte{0}, std::byte{0xFF},
                                     std::byte{0}, std::byte{0}, std::byte{0xFF}, std::byte{0xFF}};
    uLongf compressedSize = compressBound(static_cast<uLong>(raw.size()));
    std::vector<std::byte> compressed(static_cast<std::size_t>(compressedSize));
    if (compress2(reinterpret_cast<Bytef*>(compressed.data()), &compressedSize,
                  reinterpret_cast<const Bytef*>(raw.data()), static_cast<uLong>(raw.size()), Z_BEST_SPEED) != Z_OK) {
        return {};
    }
    compressed.resize(static_cast<std::size_t>(compressedSize));
    AppendPngChunkUVE(png, {std::byte{'I'}, std::byte{'D'}, std::byte{'A'}, std::byte{'T'}}, compressed);
    AppendPngChunkUVE(png, {std::byte{'I'}, std::byte{'E'}, std::byte{'N'}, std::byte{'D'}}, {});
    return png;
}

void WriteBytesUVE(const std::filesystem::path& path, const std::vector<std::byte>& bytes) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(output.is_open());
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    ASSERT_TRUE(output.good());
}

TEST(PngImporterUVETest, ImportUVE_DecodesPngToTextureEnvelopeAndRegistersGuid) {
    const std::filesystem::path sourcePath = "uve_png_importer_source.png";
    const std::filesystem::path destinationPath = "uve_png_importer_destination.uvetex";
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
    WriteBytesUVE(sourcePath, MakePngTwoByOneRgba8UVE());

    AssetImporterUVE importer;
    AssetDatabaseUVE assetDatabase;
    const AssetGuidUVE guid = importer.ImportUVE(sourcePath, destinationPath, assetDatabase);

    ASSERT_NE(guid, kInvalidAssetGuidUVE);
    EXPECT_EQ(assetDatabase.ResolveUVE(guid), destinationPath);
    TextureAssetUVE texture;
    ASSERT_TRUE(LoadTextureAssetUVE(destinationPath, texture));
    EXPECT_EQ(texture.width, 2U);
    EXPECT_EQ(texture.height, 1U);
    EXPECT_EQ(texture.format, TextureFormatUVE::RGBA8Unorm);
    EXPECT_EQ(texture.pixels, (std::vector<std::byte>{std::byte{0xFF}, std::byte{0}, std::byte{0}, std::byte{0xFF},
                                                         std::byte{0}, std::byte{0}, std::byte{0xFF}, std::byte{0xFF}}));

    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
}

TEST(PngImporterUVETest, ImportUVE_InvalidPngPreservesExistingDestination) {
    const std::filesystem::path sourcePath = "uve_png_importer_invalid_source.png";
    const std::filesystem::path destinationPath = "uve_png_importer_existing_destination.uvetex";
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
    EXPECT_TRUE(assetDatabase.ResolveUVE(kInvalidAssetGuidUVE).empty());

    std::filesystem::remove(sourcePath);
    std::filesystem::remove(destinationPath);
}

} // namespace
} // namespace UVE::Asset::Tests
