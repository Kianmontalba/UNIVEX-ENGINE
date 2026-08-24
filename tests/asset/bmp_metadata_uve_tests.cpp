// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/bmp_metadata_uve.h"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <vector>

#include <gtest/gtest.h>

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

[[nodiscard]] std::vector<std::byte> MakeBmp24TwoByTwoUVE() {
    std::vector<std::byte> bmp;
    bmp.reserve(70U);
    bmp.push_back(std::byte{'B'});
    bmp.push_back(std::byte{'M'});
    AppendU32LittleEndianUVE(bmp, 70U);
    AppendU16LittleEndianUVE(bmp, 0U);
    AppendU16LittleEndianUVE(bmp, 0U);
    AppendU32LittleEndianUVE(bmp, 54U);
    AppendU32LittleEndianUVE(bmp, 40U);
    AppendU32LittleEndianUVE(bmp, 2U);
    AppendU32LittleEndianUVE(bmp, 2U);
    AppendU16LittleEndianUVE(bmp, 1U);
    AppendU16LittleEndianUVE(bmp, 24U);
    AppendU32LittleEndianUVE(bmp, 0U);
    AppendU32LittleEndianUVE(bmp, 16U);
    AppendU32LittleEndianUVE(bmp, 2835U);
    AppendU32LittleEndianUVE(bmp, 2835U);
    AppendU32LittleEndianUVE(bmp, 0U);
    AppendU32LittleEndianUVE(bmp, 0U);
    // Bottom-up storage: blue, white, then top red, green. Each 24-bit row has two pad bytes.
    const std::byte pixels[] = {
        // Bottom row: blue, white, then two bytes of row padding.
        std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF},
        std::byte{0x00}, std::byte{0x00},
        // Top row: red, green, then two bytes of row padding.
        std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0xFF}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00}};
    bmp.insert(bmp.end(), std::begin(pixels), std::end(pixels));
    return bmp;
}

[[nodiscard]] std::vector<std::byte> MakeBmp8IndexedThreeByTwoUVE() {
    std::vector<std::byte> bmp;
    bmp.reserve(70U);
    bmp.push_back(std::byte{'B'});
    bmp.push_back(std::byte{'M'});
    AppendU32LittleEndianUVE(bmp, 70U);
    AppendU16LittleEndianUVE(bmp, 0U);
    AppendU16LittleEndianUVE(bmp, 0U);
    AppendU32LittleEndianUVE(bmp, 62U);
    AppendU32LittleEndianUVE(bmp, 40U);
    AppendU32LittleEndianUVE(bmp, 3U);
    AppendU32LittleEndianUVE(bmp, 2U);
    AppendU16LittleEndianUVE(bmp, 1U);
    AppendU16LittleEndianUVE(bmp, 8U);
    AppendU32LittleEndianUVE(bmp, 0U);
    AppendU32LittleEndianUVE(bmp, 8U);
    AppendU32LittleEndianUVE(bmp, 2835U);
    AppendU32LittleEndianUVE(bmp, 2835U);
    AppendU32LittleEndianUVE(bmp, 2U);
    AppendU32LittleEndianUVE(bmp, 0U);
    // BGRA palette entries: index 0 red, index 1 blue; each row has one pad byte.
    const std::byte paletteAndPixels[] = {
        std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0x00},
        std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
        // Bottom row: blue, red, blue, then one byte of row padding.
        std::byte{0x01}, std::byte{0x00}, std::byte{0x01}, std::byte{0x00},
        // Top row: red, blue, red, then one byte of row padding.
        std::byte{0x00}, std::byte{0x01}, std::byte{0x00}, std::byte{0x00}};
    bmp.insert(bmp.end(), std::begin(paletteAndPixels), std::end(paletteAndPixels));
    return bmp;
}

[[nodiscard]] std::vector<std::byte> MakeBmp1IndexedTenByTwoUVE() {
    std::vector<std::byte> bmp;
    bmp.reserve(70U);
    bmp.push_back(std::byte{'B'});
    bmp.push_back(std::byte{'M'});
    AppendU32LittleEndianUVE(bmp, 70U);
    AppendU16LittleEndianUVE(bmp, 0U);
    AppendU16LittleEndianUVE(bmp, 0U);
    AppendU32LittleEndianUVE(bmp, 62U);
    AppendU32LittleEndianUVE(bmp, 40U);
    AppendU32LittleEndianUVE(bmp, 10U);
    AppendU32LittleEndianUVE(bmp, 2U);
    AppendU16LittleEndianUVE(bmp, 1U);
    AppendU16LittleEndianUVE(bmp, 1U);
    AppendU32LittleEndianUVE(bmp, 0U);
    AppendU32LittleEndianUVE(bmp, 8U);
    AppendU32LittleEndianUVE(bmp, 2835U);
    AppendU32LittleEndianUVE(bmp, 2835U);
    AppendU32LittleEndianUVE(bmp, 2U);
    AppendU32LittleEndianUVE(bmp, 0U);
    // BGRA palette entries 0..1 are black and white; each row has two pad bytes.
    const std::byte paletteAndPixels[] = {
        std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
        std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0x00},
        // Bottom row: 10110010 10, then two bytes of row padding.
        std::byte{0xB2}, std::byte{0x80}, std::byte{0xAA}, std::byte{0xBB},
        // Top row: 01001101 01, then two bytes of row padding.
        std::byte{0x4D}, std::byte{0x40}, std::byte{0xCC}, std::byte{0xDD}};
    bmp.insert(bmp.end(), std::begin(paletteAndPixels), std::end(paletteAndPixels));
    return bmp;
}

[[nodiscard]] std::vector<std::byte> MakeBmp4IndexedFiveByTwoUVE() {
    std::vector<std::byte> bmp;
    bmp.reserve(78U);
    bmp.push_back(std::byte{'B'});
    bmp.push_back(std::byte{'M'});
    AppendU32LittleEndianUVE(bmp, 78U);
    AppendU16LittleEndianUVE(bmp, 0U);
    AppendU16LittleEndianUVE(bmp, 0U);
    AppendU32LittleEndianUVE(bmp, 70U);
    AppendU32LittleEndianUVE(bmp, 40U);
    AppendU32LittleEndianUVE(bmp, 5U);
    AppendU32LittleEndianUVE(bmp, 2U);
    AppendU16LittleEndianUVE(bmp, 1U);
    AppendU16LittleEndianUVE(bmp, 4U);
    AppendU32LittleEndianUVE(bmp, 0U);
    AppendU32LittleEndianUVE(bmp, 8U);
    AppendU32LittleEndianUVE(bmp, 2835U);
    AppendU32LittleEndianUVE(bmp, 2835U);
    AppendU32LittleEndianUVE(bmp, 4U);
    AppendU32LittleEndianUVE(bmp, 0U);
    // BGRA palette entries 0..3 are red, green, blue, white; each 4-bit row has one pad byte.
    const std::byte paletteAndPixels[] = {
        std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0x00},
        std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0x00},
        std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
        std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0x00},
        // Bottom row: blue, white, red, green, blue, then one byte of row padding.
        std::byte{0x23}, std::byte{0x01}, std::byte{0x20}, std::byte{0xAA},
        // Top row: red, green, blue, white, red, then one byte of row padding.
        std::byte{0x01}, std::byte{0x23}, std::byte{0x00}, std::byte{0xBB}};
    bmp.insert(bmp.end(), std::begin(paletteAndPixels), std::end(paletteAndPixels));
    return bmp;
}

[[nodiscard]] std::vector<std::byte> MakeBmp16TwoByOneUVE() {
    std::vector<std::byte> bmp;
    bmp.reserve(58U);
    bmp.push_back(std::byte{'B'});
    bmp.push_back(std::byte{'M'});
    AppendU32LittleEndianUVE(bmp, 58U);
    AppendU16LittleEndianUVE(bmp, 0U);
    AppendU16LittleEndianUVE(bmp, 0U);
    AppendU32LittleEndianUVE(bmp, 54U);
    AppendU32LittleEndianUVE(bmp, 40U);
    AppendU32LittleEndianUVE(bmp, 2U);
    AppendU32LittleEndianUVE(bmp, 1U);
    AppendU16LittleEndianUVE(bmp, 1U);
    AppendU16LittleEndianUVE(bmp, 16U);
    AppendU32LittleEndianUVE(bmp, 0U);
    AppendU32LittleEndianUVE(bmp, 4U);
    AppendU32LittleEndianUVE(bmp, 2835U);
    AppendU32LittleEndianUVE(bmp, 2835U);
    AppendU32LittleEndianUVE(bmp, 0U);
    AppendU32LittleEndianUVE(bmp, 0U);
    // Bottom-up BI_RGB BGR555: opaque red followed by opaque blue.
    bmp.insert(bmp.end(), {std::byte{0x00}, std::byte{0x7C}, std::byte{0x1F}, std::byte{0x00}});
    return bmp;
}

[[nodiscard]] std::vector<std::byte> MakeBmp16Bgr555BitfieldsTwoByOneUVE() {
    std::vector<std::byte> bmp;
    bmp.reserve(70U);
    bmp.push_back(std::byte{'B'});
    bmp.push_back(std::byte{'M'});
    AppendU32LittleEndianUVE(bmp, 70U);
    AppendU16LittleEndianUVE(bmp, 0U);
    AppendU16LittleEndianUVE(bmp, 0U);
    AppendU32LittleEndianUVE(bmp, 66U);
    AppendU32LittleEndianUVE(bmp, 40U);
    AppendU32LittleEndianUVE(bmp, 2U);
    AppendU32LittleEndianUVE(bmp, 1U);
    AppendU16LittleEndianUVE(bmp, 1U);
    AppendU16LittleEndianUVE(bmp, 16U);
    AppendU32LittleEndianUVE(bmp, 3U);
    AppendU32LittleEndianUVE(bmp, 4U);
    AppendU32LittleEndianUVE(bmp, 2835U);
    AppendU32LittleEndianUVE(bmp, 2835U);
    AppendU32LittleEndianUVE(bmp, 0U);
    AppendU32LittleEndianUVE(bmp, 0U);
    // BI_BITFIELDS BGR555 masks followed by bottom-up opaque red and green pixels.
    AppendU32LittleEndianUVE(bmp, 0x7C00U);
    AppendU32LittleEndianUVE(bmp, 0x03E0U);
    AppendU32LittleEndianUVE(bmp, 0x001FU);
    bmp.insert(bmp.end(), {std::byte{0x00}, std::byte{0x7C}, std::byte{0xE0}, std::byte{0x03}});
    return bmp;
}

[[nodiscard]] std::vector<std::byte> MakeBmp16BitfieldsTwoByOneUVE() {
    std::vector<std::byte> bmp;
    bmp.reserve(70U);
    bmp.push_back(std::byte{'B'});
    bmp.push_back(std::byte{'M'});
    AppendU32LittleEndianUVE(bmp, 70U);
    AppendU16LittleEndianUVE(bmp, 0U);
    AppendU16LittleEndianUVE(bmp, 0U);
    AppendU32LittleEndianUVE(bmp, 66U);
    AppendU32LittleEndianUVE(bmp, 40U);
    AppendU32LittleEndianUVE(bmp, 2U);
    AppendU32LittleEndianUVE(bmp, 1U);
    AppendU16LittleEndianUVE(bmp, 1U);
    AppendU16LittleEndianUVE(bmp, 16U);
    AppendU32LittleEndianUVE(bmp, 3U);
    AppendU32LittleEndianUVE(bmp, 4U);
    AppendU32LittleEndianUVE(bmp, 2835U);
    AppendU32LittleEndianUVE(bmp, 2835U);
    AppendU32LittleEndianUVE(bmp, 0U);
    AppendU32LittleEndianUVE(bmp, 0U);
    // BI_BITFIELDS BGR565 masks followed by bottom-up opaque red and blue pixels.
    AppendU32LittleEndianUVE(bmp, 0xF800U);
    AppendU32LittleEndianUVE(bmp, 0x07E0U);
    AppendU32LittleEndianUVE(bmp, 0x001FU);
    bmp.insert(bmp.end(), {std::byte{0x00}, std::byte{0xF8}, std::byte{0x1F}, std::byte{0x00}});
    return bmp;
}

[[nodiscard]] std::vector<std::byte> MakeBmp32TopDownOneByTwoUVE() {
    std::vector<std::byte> bmp;
    bmp.reserve(62U);
    bmp.push_back(std::byte{'B'});
    bmp.push_back(std::byte{'M'});
    AppendU32LittleEndianUVE(bmp, 62U);
    AppendU16LittleEndianUVE(bmp, 0U);
    AppendU16LittleEndianUVE(bmp, 0U);
    AppendU32LittleEndianUVE(bmp, 54U);
    AppendU32LittleEndianUVE(bmp, 40U);
    AppendU32LittleEndianUVE(bmp, 1U);
    AppendU32LittleEndianUVE(bmp, static_cast<std::uint32_t>(-2));
    AppendU16LittleEndianUVE(bmp, 1U);
    AppendU16LittleEndianUVE(bmp, 32U);
    AppendU32LittleEndianUVE(bmp, 0U);
    AppendU32LittleEndianUVE(bmp, 8U);
    AppendU32LittleEndianUVE(bmp, 2835U);
    AppendU32LittleEndianUVE(bmp, 2835U);
    AppendU32LittleEndianUVE(bmp, 0U);
    AppendU32LittleEndianUVE(bmp, 0U);
    // Top-down storage: source alpha is intentionally ignored for canonical opaque RGBA8 output.
    const std::byte pixels[] = {std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0x12},
                                std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0x34}};
    bmp.insert(bmp.end(), std::begin(pixels), std::end(pixels));
    return bmp;
}

TEST(BmpMetadataUVETest, DecodeBmpRgba8ImageUVE_DecodesBottomUp24BitRowsToTopDownRgba) {
    BmpRgba8ImageUVE image;
    ASSERT_TRUE(DecodeBmpRgba8ImageUVE(MakeBmp24TwoByTwoUVE(), image));
    EXPECT_EQ(image.width, 2U);
    EXPECT_EQ(image.height, 2U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0xFF},
                                 std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}}));
}

TEST(BmpMetadataUVETest, DecodeBmpRgba8ImageUVE_Decodes8BitIndexedRowsWithPaletteAndPadding) {
    BmpRgba8ImageUVE image;
    ASSERT_TRUE(DecodeBmpRgba8ImageUVE(MakeBmp8IndexedThreeByTwoUVE(), image));
    EXPECT_EQ(image.width, 3U);
    EXPECT_EQ(image.height, 2U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0xFF},
                                 std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0xFF},
                                 std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0xFF}}));
}

TEST(BmpMetadataUVETest, DecodeBmpRgba8ImageUVE_Decodes1BitIndexedRowsWithPaletteAndPadding) {
    BmpRgba8ImageUVE image;
    ASSERT_TRUE(DecodeBmpRgba8ImageUVE(MakeBmp1IndexedTenByTwoUVE(), image));
    EXPECT_EQ(image.width, 10U);
    EXPECT_EQ(image.height, 2U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF},
                                 std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF},
                                 std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF},
                                 std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}}));
}

TEST(BmpMetadataUVETest, DecodeBmpRgba8ImageUVE_Decodes4BitIndexedRowsWithPaletteAndPadding) {
    BmpRgba8ImageUVE image;
    ASSERT_TRUE(DecodeBmpRgba8ImageUVE(MakeBmp4IndexedFiveByTwoUVE(), image));
    EXPECT_EQ(image.width, 5U);
    EXPECT_EQ(image.height, 2U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0xFF},
                                 std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF},
                                 std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0xFF},
                                 std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF},
                                 std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0xFF}}));
}

TEST(BmpMetadataUVETest, DecodeBmpRgba8ImageUVE_Decodes16BitBgr555RowsToOpaqueRgba) {
    BmpRgba8ImageUVE image;
    ASSERT_TRUE(DecodeBmpRgba8ImageUVE(MakeBmp16TwoByOneUVE(), image));
    EXPECT_EQ(image.width, 2U);
    EXPECT_EQ(image.height, 1U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0xFF}}));
}

TEST(BmpMetadataUVETest, DecodeBmpRgba8ImageUVE_Decodes16BitBitfieldsBgr555RowsToOpaqueRgba) {
    BmpRgba8ImageUVE image;
    ASSERT_TRUE(DecodeBmpRgba8ImageUVE(MakeBmp16Bgr555BitfieldsTwoByOneUVE(), image));
    EXPECT_EQ(image.width, 2U);
    EXPECT_EQ(image.height, 1U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0xFF}}));
}

TEST(BmpMetadataUVETest, DecodeBmpRgba8ImageUVE_Decodes16BitBitfieldsBgr565RowsToOpaqueRgba) {
    BmpRgba8ImageUVE image;
    ASSERT_TRUE(DecodeBmpRgba8ImageUVE(MakeBmp16BitfieldsTwoByOneUVE(), image));
    EXPECT_EQ(image.width, 2U);
    EXPECT_EQ(image.height, 1U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0xFF}}));
}

TEST(BmpMetadataUVETest, DecodeBmpRgba8ImageUVE_DecodesTopDown32BitRowsWithOpaqueAlpha) {
    BmpRgba8ImageUVE image;
    ASSERT_TRUE(DecodeBmpRgba8ImageUVE(MakeBmp32TopDownOneByTwoUVE(), image));
    EXPECT_EQ(image.width, 1U);
    EXPECT_EQ(image.height, 2U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                                      std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0xFF}}));
}

TEST(BmpMetadataUVETest, DecodeBmpRgba8ImageUVE_RejectsMalformedInputAtomically) {
    const BmpRgba8ImageUVE original{1U, 1U, {std::byte{0x11}, std::byte{0x22}, std::byte{0x33}, std::byte{0x44}}};
    BmpRgba8ImageUVE image = original;
    const std::vector<std::byte> valid = MakeBmp24TwoByTwoUVE();
    std::vector<std::byte> truncated = valid;
    truncated.resize(53U);
    EXPECT_FALSE(DecodeBmpRgba8ImageUVE(truncated, image));
    EXPECT_EQ(image.width, original.width);
    EXPECT_EQ(image.height, original.height);
    EXPECT_EQ(image.pixels, original.pixels);
    EXPECT_FALSE(DecodeBmpRgba8ImageUVE({std::byte{'N'}, std::byte{'O'}}, image));
    EXPECT_EQ(image.pixels, original.pixels);
    const std::vector<std::byte> valid1BitIndexed = MakeBmp1IndexedTenByTwoUVE();
    std::vector<std::byte> excessive1BitPalette = valid1BitIndexed;
    excessive1BitPalette[46] = std::byte{0x03};
    excessive1BitPalette[47] = std::byte{0x00};
    excessive1BitPalette[48] = std::byte{0x00};
    excessive1BitPalette[49] = std::byte{0x00};
    EXPECT_FALSE(DecodeBmpRgba8ImageUVE(excessive1BitPalette, image));
    const std::vector<std::byte> valid4BitIndexed = MakeBmp4IndexedFiveByTwoUVE();
    std::vector<std::byte> excessive4BitPalette = valid4BitIndexed;
    excessive4BitPalette[46] = std::byte{0x11};
    excessive4BitPalette[47] = std::byte{0x00};
    excessive4BitPalette[48] = std::byte{0x00};
    excessive4BitPalette[49] = std::byte{0x00};
    EXPECT_FALSE(DecodeBmpRgba8ImageUVE(excessive4BitPalette, image));
    const std::vector<std::byte> validIndexed = MakeBmp8IndexedThreeByTwoUVE();
    std::vector<std::byte> excessivePalette = validIndexed;
    excessivePalette[46] = std::byte{0x01};
    excessivePalette[47] = std::byte{0x01};
    excessivePalette[48] = std::byte{0x00};
    excessivePalette[49] = std::byte{0x00};
    EXPECT_FALSE(DecodeBmpRgba8ImageUVE(excessivePalette, image));
    std::vector<std::byte> truncatedPalette = validIndexed;
    truncatedPalette.resize(61U);
    EXPECT_FALSE(DecodeBmpRgba8ImageUVE(truncatedPalette, image));
    std::vector<std::byte> earlyPixelOffset = validIndexed;
    earlyPixelOffset[10] = std::byte{0x3A};
    earlyPixelOffset[11] = std::byte{0x00};
    earlyPixelOffset[12] = std::byte{0x00};
    earlyPixelOffset[13] = std::byte{0x00};
    EXPECT_FALSE(DecodeBmpRgba8ImageUVE(earlyPixelOffset, image));
    std::vector<std::byte> invalidPaletteIndex = validIndexed;
    invalidPaletteIndex[62] = std::byte{0x02};
    EXPECT_FALSE(DecodeBmpRgba8ImageUVE(invalidPaletteIndex, image));
    const std::vector<std::byte> validBitfields = MakeBmp16BitfieldsTwoByOneUVE();
    std::vector<std::byte> invalidBitfieldsMasks = validBitfields;
    invalidBitfieldsMasks[55] = std::byte{0x00};
    EXPECT_FALSE(DecodeBmpRgba8ImageUVE(invalidBitfieldsMasks, image));
    std::vector<std::byte> truncatedBitfields = validBitfields;
    truncatedBitfields.resize(65U);
    EXPECT_FALSE(DecodeBmpRgba8ImageUVE(truncatedBitfields, image));
    std::vector<std::byte> earlyBitfieldsPixelOffset = validBitfields;
    earlyBitfieldsPixelOffset[10] = std::byte{0x41};
    earlyBitfieldsPixelOffset[11] = std::byte{0x00};
    earlyBitfieldsPixelOffset[12] = std::byte{0x00};
    earlyBitfieldsPixelOffset[13] = std::byte{0x00};
    EXPECT_FALSE(DecodeBmpRgba8ImageUVE(earlyBitfieldsPixelOffset, image));
    EXPECT_EQ(image.width, original.width);
    EXPECT_EQ(image.height, original.height);
    EXPECT_EQ(image.pixels, original.pixels);
}

} // namespace
} // namespace UVE::Asset::Tests
