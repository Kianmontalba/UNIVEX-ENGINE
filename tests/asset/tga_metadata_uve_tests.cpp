// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/tga_metadata_uve.h"

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

[[nodiscard]] std::vector<std::byte> MakeTga24TwoByTwoBottomLeftUVE() {
    std::vector<std::byte> tga;
    tga.reserve(30U);
    tga.insert(tga.end(), 12U, std::byte{0});
    tga[2] = std::byte{2};
    AppendU16LittleEndianUVE(tga, 2U);
    AppendU16LittleEndianUVE(tga, 2U);
    tga.push_back(std::byte{24});
    tga.push_back(std::byte{0});
    // Bottom-left storage, BGR pixels with no row padding.
    const std::byte pixels[] = {
        // Bottom row: blue, white.
        std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF},
        // Top row: red, green.
        std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}};
    tga.insert(tga.end(), std::begin(pixels), std::end(pixels));
    return tga;
}

[[nodiscard]] std::vector<std::byte> MakeTga24RleTwoByTwoTopLeftUVE() {
    std::vector<std::byte> tga;
    tga.reserve(28U);
    tga.insert(tga.end(), 12U, std::byte{0});
    tga[2] = std::byte{10};
    AppendU16LittleEndianUVE(tga, 2U);
    AppendU16LittleEndianUVE(tga, 2U);
    tga.push_back(std::byte{24});
    tga.push_back(std::byte{0x20});
    // One raw packet for red/green, followed by one run packet for two blue pixels.
    tga.insert(tga.end(), {std::byte{0x01}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                           std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0x81},
                           std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}});
    return tga;
}

[[nodiscard]] std::vector<std::byte> MakeTga8GrayscaleTwoByOneBottomRightUVE() {
    std::vector<std::byte> tga;
    tga.reserve(20U);
    tga.insert(tga.end(), 12U, std::byte{0});
    tga[2] = std::byte{3};
    AppendU16LittleEndianUVE(tga, 2U);
    AppendU16LittleEndianUVE(tga, 1U);
    tga.push_back(std::byte{8});
    tga.push_back(std::byte{0x10});
    tga.insert(tga.end(), {std::byte{0x11}, std::byte{0xEE}});
    return tga;
}

[[nodiscard]] std::vector<std::byte> MakeTga8GrayscaleRleThreeByOneTopLeftUVE() {
    std::vector<std::byte> tga;
    tga.reserve(20U);
    tga.insert(tga.end(), 12U, std::byte{0});
    tga[2] = std::byte{11};
    AppendU16LittleEndianUVE(tga, 3U);
    AppendU16LittleEndianUVE(tga, 1U);
    tga.push_back(std::byte{8});
    tga.push_back(std::byte{0x20});
    tga.insert(tga.end(), {std::byte{0x82}, std::byte{0x77}});
    return tga;
}

[[nodiscard]] std::vector<std::byte> MakeTga16TrueColorTwoByOneTopLeftUVE() {
    std::vector<std::byte> tga;
    tga.reserve(22U);
    tga.insert(tga.end(), 18U, std::byte{0});
    tga[2] = std::byte{2};
    tga[12] = std::byte{2};
    tga[14] = std::byte{1};
    tga[16] = std::byte{16};
    tga[17] = std::byte{0x21};
    // Little-endian BGR5551: opaque red, transparent blue.
    tga.insert(tga.end(), {std::byte{0x00}, std::byte{0xFC}, std::byte{0x1F}, std::byte{0x00}});
    return tga;
}

[[nodiscard]] std::vector<std::byte> MakeTga16TrueColor565TwoByOneTopLeftUVE() {
    std::vector<std::byte> tga;
    tga.reserve(22U);
    tga.insert(tga.end(), 18U, std::byte{0});
    tga[2] = std::byte{2};
    tga[12] = std::byte{2};
    tga[14] = std::byte{1};
    tga[16] = std::byte{16};
    tga[17] = std::byte{0x20};
    // Little-endian BGR565: opaque red, opaque blue.
    tga.insert(tga.end(), {std::byte{0x00}, std::byte{0xF8}, std::byte{0x1F}, std::byte{0x00}});
    return tga;
}

[[nodiscard]] std::vector<std::byte> MakeTga16TrueColor565RleThreeByOneTopLeftUVE() {
    std::vector<std::byte> tga;
    tga.reserve(23U);
    tga.insert(tga.end(), 18U, std::byte{0});
    tga[2] = std::byte{10};
    tga[12] = std::byte{3};
    tga[14] = std::byte{1};
    tga[16] = std::byte{16};
    tga[17] = std::byte{0x20};
    // One opaque green BGR565 sample in a three-pixel run.
    tga.insert(tga.end(), {std::byte{0x82}, std::byte{0xE0}, std::byte{0x07}});
    return tga;
}

[[nodiscard]] std::vector<std::byte> MakeTga16TrueColorRleThreeByOneTopLeftUVE() {
    std::vector<std::byte> tga;
    tga.reserve(23U);
    tga.insert(tga.end(), 18U, std::byte{0});
    tga[2] = std::byte{10};
    tga[12] = std::byte{3};
    tga[14] = std::byte{1};
    tga[16] = std::byte{16};
    tga[17] = std::byte{0x21};
    // One opaque green BGR5551 sample in a three-pixel run.
    tga.insert(tga.end(), {std::byte{0x82}, std::byte{0xE0}, std::byte{0x83}});
    return tga;
}

[[nodiscard]] std::vector<std::byte> MakeTga16GrayscaleAlphaTwoByOneTopLeftUVE() {
    std::vector<std::byte> tga;
    tga.reserve(22U);
    tga.insert(tga.end(), 18U, std::byte{0});
    tga[2] = std::byte{3};
    tga[12] = std::byte{2};
    tga[14] = std::byte{1};
    tga[16] = std::byte{16};
    tga[17] = std::byte{0x20};
    tga.insert(tga.end(), {std::byte{0x40}, std::byte{0x10}, std::byte{0xC0}, std::byte{0x80}});
    return tga;
}

[[nodiscard]] std::vector<std::byte> MakeTga16GrayscaleAlphaRleThreeByOneTopLeftUVE() {
    std::vector<std::byte> tga;
    tga.reserve(22U);
    tga.insert(tga.end(), 18U, std::byte{0});
    tga[2] = std::byte{11};
    tga[12] = std::byte{3};
    tga[14] = std::byte{1};
    tga[16] = std::byte{16};
    tga[17] = std::byte{0x20};
    tga.insert(tga.end(), {std::byte{0x82}, std::byte{0x7F}, std::byte{0x55}});
    return tga;
}

[[nodiscard]] std::vector<std::byte> MakeTga8PaletteTwoByOneTopLeftUVE() {
    std::vector<std::byte> tga;
    tga.reserve(26U);
    tga.insert(tga.end(), 18U, std::byte{0});
    tga[1] = std::byte{1};
    tga[2] = std::byte{1};
    tga[5] = std::byte{2};
    tga[7] = std::byte{24};
    tga[12] = std::byte{2};
    tga[14] = std::byte{1};
    tga[16] = std::byte{8};
    tga[17] = std::byte{0x20};
    // Palette entries: red, green (BGR order), followed by palette indices 0 and 1.
    tga.insert(tga.end(), {std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0x00},
                           std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0x01}});
    return tga;
}

[[nodiscard]] std::vector<std::byte> MakeTga8PaletteRleThreeByOneTopLeftUVE() {
    std::vector<std::byte> tga;
    tga.reserve(23U);
    tga.insert(tga.end(), 18U, std::byte{0});
    tga[1] = std::byte{1};
    tga[2] = std::byte{9};
    tga[5] = std::byte{1};
    tga[7] = std::byte{24};
    tga[12] = std::byte{3};
    tga[14] = std::byte{1};
    tga[16] = std::byte{8};
    tga[17] = std::byte{0x20};
    // One blue palette entry and one run packet containing three index-zero pixels.
    tga.insert(tga.end(), {std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0x82}, std::byte{0x00}});
    return tga;
}

[[nodiscard]] std::vector<std::byte> MakeTga32PaletteTwoByOneTopLeftUVE() {
    std::vector<std::byte> tga;
    tga.reserve(26U);
    tga.insert(tga.end(), 18U, std::byte{0});
    tga[1] = std::byte{1};
    tga[2] = std::byte{1};
    tga[5] = std::byte{2};
    tga[7] = std::byte{32};
    tga[12] = std::byte{2};
    tga[14] = std::byte{1};
    tga[16] = std::byte{8};
    tga[17] = std::byte{0x28};
    // BGRA palette entries: red at alpha 0x40, blue at alpha 0x90, followed by indices 0 and 1.
    tga.insert(tga.end(), {std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0x40},
                           std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0x90},
                           std::byte{0x00}, std::byte{0x01}});
    return tga;
}

[[nodiscard]] std::vector<std::byte> MakeTga32PaletteRleThreeByOneTopLeftUVE() {
    std::vector<std::byte> tga;
    tga.reserve(24U);
    tga.insert(tga.end(), 18U, std::byte{0});
    tga[1] = std::byte{1};
    tga[2] = std::byte{9};
    tga[5] = std::byte{1};
    tga[7] = std::byte{32};
    tga[12] = std::byte{3};
    tga[14] = std::byte{1};
    tga[16] = std::byte{8};
    tga[17] = std::byte{0x28};
    // One BGRA green palette entry at alpha 0x66 and one run packet of three index-zero pixels.
    tga.insert(tga.end(), {std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0x66},
                           std::byte{0x82}, std::byte{0x00}});
    return tga;
}

[[nodiscard]] std::vector<std::byte> MakeTga16PaletteTwoByOneTopLeftUVE() {
    std::vector<std::byte> tga;
    tga.reserve(22U);
    tga.insert(tga.end(), 18U, std::byte{0});
    tga[1] = std::byte{1};
    tga[2] = std::byte{1};
    tga[5] = std::byte{2};
    tga[7] = std::byte{16};
    tga[12] = std::byte{2};
    tga[14] = std::byte{1};
    tga[16] = std::byte{8};
    tga[17] = std::byte{0x21};
    // BGR5551 palette entries: opaque red, transparent blue, followed by indices 0 and 1.
    tga.insert(tga.end(), {std::byte{0x00}, std::byte{0xFC}, std::byte{0x1F}, std::byte{0x00},
                           std::byte{0x00}, std::byte{0x01}});
    return tga;
}

[[nodiscard]] std::vector<std::byte> MakeTga16PaletteRleThreeByOneTopLeftUVE() {
    std::vector<std::byte> tga;
    tga.reserve(22U);
    tga.insert(tga.end(), 18U, std::byte{0});
    tga[1] = std::byte{1};
    tga[2] = std::byte{9};
    tga[5] = std::byte{1};
    tga[7] = std::byte{16};
    tga[12] = std::byte{3};
    tga[14] = std::byte{1};
    tga[16] = std::byte{8};
    tga[17] = std::byte{0x21};
    // One opaque green BGR5551 palette entry and one run packet of three index-zero pixels.
    tga.insert(tga.end(), {std::byte{0xE0}, std::byte{0x83}, std::byte{0x82}, std::byte{0x00}});
    return tga;
}

[[nodiscard]] std::vector<std::byte> MakeTga32TrueColorAlphaTwoByOneTopLeftUVE() {
    std::vector<std::byte> tga;
    tga.reserve(26U);
    tga.insert(tga.end(), 18U, std::byte{0});
    tga[2] = std::byte{2};
    tga[12] = std::byte{2};
    tga[14] = std::byte{1};
    tga[16] = std::byte{32};
    tga[17] = std::byte{0x28};
    // BGRA true-color pixels: red at alpha 0x40, blue at alpha 0x90.
    tga.insert(tga.end(), {std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0x40},
                           std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0x90}});
    return tga;
}

[[nodiscard]] std::vector<std::byte> MakeTga32TrueColorAlphaRleThreeByOneTopLeftUVE() {
    std::vector<std::byte> tga;
    tga.reserve(23U);
    tga.insert(tga.end(), 18U, std::byte{0});
    tga[2] = std::byte{10};
    tga[12] = std::byte{3};
    tga[14] = std::byte{1};
    tga[16] = std::byte{32};
    tga[17] = std::byte{0x28};
    // One BGRA red sample at alpha 0x66 in a three-pixel run.
    tga.insert(tga.end(), {std::byte{0x82}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0x66}});
    return tga;
}

[[nodiscard]] std::vector<std::byte> MakeTga32OneByTwoTopRightUVE() {
    std::vector<std::byte> tga;
    tga.reserve(26U);
    tga.insert(tga.end(), 12U, std::byte{0});
    tga[2] = std::byte{2};
    AppendU16LittleEndianUVE(tga, 1U);
    AppendU16LittleEndianUVE(tga, 2U);
    tga.push_back(std::byte{32});
    tga.push_back(std::byte{0x30});
    // Top-right storage; source alpha is intentionally ignored for opaque output.
    const std::byte pixels[] = {std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0x12},
                                std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0x34}};
    tga.insert(tga.end(), std::begin(pixels), std::end(pixels));
    return tga;
}

TEST(TgaMetadataUVETest, DecodeTgaRgba8ImageUVE_DecodesBottomLeft24BitToTopDownOpaqueRgba) {
    TgaRgba8ImageUVE image;
    ASSERT_TRUE(DecodeTgaRgba8ImageUVE(MakeTga24TwoByTwoBottomLeftUVE(), image));
    EXPECT_EQ(image.width, 2U);
    EXPECT_EQ(image.height, 2U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0xFF},
                                 std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}}));
}

TEST(TgaMetadataUVETest, DecodeTgaRgba8ImageUVE_DecodesRle24BitPacketsToTopDownOpaqueRgba) {
    TgaRgba8ImageUVE image;
    ASSERT_TRUE(DecodeTgaRgba8ImageUVE(MakeTga24RleTwoByTwoTopLeftUVE(), image));
    EXPECT_EQ(image.width, 2U);
    EXPECT_EQ(image.height, 2U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0xFF}}));
}

TEST(TgaMetadataUVETest, DecodeTgaRgba8ImageUVE_Decodes8BitGrayscaleWithOriginToOpaqueRgba) {
    TgaRgba8ImageUVE image;
    ASSERT_TRUE(DecodeTgaRgba8ImageUVE(MakeTga8GrayscaleTwoByOneBottomRightUVE(), image));
    EXPECT_EQ(image.width, 2U);
    EXPECT_EQ(image.height, 1U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0xEE}, std::byte{0xEE}, std::byte{0xEE}, std::byte{0xFF},
                                 std::byte{0x11}, std::byte{0x11}, std::byte{0x11}, std::byte{0xFF}}));
}

TEST(TgaMetadataUVETest, DecodeTgaRgba8ImageUVE_DecodesRle8BitGrayscaleToOpaqueRgba) {
    TgaRgba8ImageUVE image;
    ASSERT_TRUE(DecodeTgaRgba8ImageUVE(MakeTga8GrayscaleRleThreeByOneTopLeftUVE(), image));
    EXPECT_EQ(image.width, 3U);
    EXPECT_EQ(image.height, 1U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0x77}, std::byte{0x77}, std::byte{0x77}, std::byte{0xFF},
                                 std::byte{0x77}, std::byte{0x77}, std::byte{0x77}, std::byte{0xFF},
                                 std::byte{0x77}, std::byte{0x77}, std::byte{0x77}, std::byte{0xFF}}));
}

TEST(TgaMetadataUVETest, DecodeTgaRgba8ImageUVE_Decodes16BitTrueColor565ToRgba) {
    TgaRgba8ImageUVE image;
    ASSERT_TRUE(DecodeTgaRgba8ImageUVE(MakeTga16TrueColor565TwoByOneTopLeftUVE(), image));
    EXPECT_EQ(image.width, 2U);
    EXPECT_EQ(image.height, 1U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0xFF}}));
}

TEST(TgaMetadataUVETest, DecodeTgaRgba8ImageUVE_DecodesRle16BitTrueColor565ToRgba) {
    TgaRgba8ImageUVE image;
    ASSERT_TRUE(DecodeTgaRgba8ImageUVE(MakeTga16TrueColor565RleThreeByOneTopLeftUVE(), image));
    EXPECT_EQ(image.width, 3U);
    EXPECT_EQ(image.height, 1U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0xFF}}));
}

TEST(TgaMetadataUVETest, DecodeTgaRgba8ImageUVE_Decodes16BitTrueColor5551ToRgba) {
    TgaRgba8ImageUVE image;
    ASSERT_TRUE(DecodeTgaRgba8ImageUVE(MakeTga16TrueColorTwoByOneTopLeftUVE(), image));
    EXPECT_EQ(image.width, 2U);
    EXPECT_EQ(image.height, 1U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}}));
}

TEST(TgaMetadataUVETest, DecodeTgaRgba8ImageUVE_DecodesRle16BitTrueColor5551ToRgba) {
    TgaRgba8ImageUVE image;
    ASSERT_TRUE(DecodeTgaRgba8ImageUVE(MakeTga16TrueColorRleThreeByOneTopLeftUVE(), image));
    EXPECT_EQ(image.width, 3U);
    EXPECT_EQ(image.height, 1U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0xFF}}));
}

TEST(TgaMetadataUVETest, DecodeTgaRgba8ImageUVE_Decodes16BitGrayscaleAlphaToRgba) {
    TgaRgba8ImageUVE image;
    ASSERT_TRUE(DecodeTgaRgba8ImageUVE(MakeTga16GrayscaleAlphaTwoByOneTopLeftUVE(), image));
    EXPECT_EQ(image.width, 2U);
    EXPECT_EQ(image.height, 1U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0x40}, std::byte{0x40}, std::byte{0x40}, std::byte{0x10},
                                 std::byte{0xC0}, std::byte{0xC0}, std::byte{0xC0}, std::byte{0x80}}));
}

TEST(TgaMetadataUVETest, DecodeTgaRgba8ImageUVE_DecodesRle16BitGrayscaleAlphaToRgba) {
    TgaRgba8ImageUVE image;
    ASSERT_TRUE(DecodeTgaRgba8ImageUVE(MakeTga16GrayscaleAlphaRleThreeByOneTopLeftUVE(), image));
    EXPECT_EQ(image.width, 3U);
    EXPECT_EQ(image.height, 1U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0x7F}, std::byte{0x7F}, std::byte{0x7F}, std::byte{0x55},
                                 std::byte{0x7F}, std::byte{0x7F}, std::byte{0x7F}, std::byte{0x55},
                                 std::byte{0x7F}, std::byte{0x7F}, std::byte{0x7F}, std::byte{0x55}}));
}

TEST(TgaMetadataUVETest, DecodeTgaRgba8ImageUVE_Decodes8BitPaletteToTopDownOpaqueRgba) {
    TgaRgba8ImageUVE image;
    ASSERT_TRUE(DecodeTgaRgba8ImageUVE(MakeTga8PaletteTwoByOneTopLeftUVE(), image));
    EXPECT_EQ(image.width, 2U);
    EXPECT_EQ(image.height, 1U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0xFF}}));
}

TEST(TgaMetadataUVETest, DecodeTgaRgba8ImageUVE_DecodesRle8BitPaletteToOpaqueRgba) {
    TgaRgba8ImageUVE image;
    ASSERT_TRUE(DecodeTgaRgba8ImageUVE(MakeTga8PaletteRleThreeByOneTopLeftUVE(), image));
    EXPECT_EQ(image.width, 3U);
    EXPECT_EQ(image.height, 1U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0xFF}}));
}

TEST(TgaMetadataUVETest, DecodeTgaRgba8ImageUVE_Decodes32BitPaletteAlphaToRgba) {
    TgaRgba8ImageUVE image;
    ASSERT_TRUE(DecodeTgaRgba8ImageUVE(MakeTga32PaletteTwoByOneTopLeftUVE(), image));
    EXPECT_EQ(image.width, 2U);
    EXPECT_EQ(image.height, 1U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0x40},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0x90}}));
}

TEST(TgaMetadataUVETest, DecodeTgaRgba8ImageUVE_DecodesRle32BitPaletteAlphaToRgba) {
    TgaRgba8ImageUVE image;
    ASSERT_TRUE(DecodeTgaRgba8ImageUVE(MakeTga32PaletteRleThreeByOneTopLeftUVE(), image));
    EXPECT_EQ(image.width, 3U);
    EXPECT_EQ(image.height, 1U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0x66},
                                 std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0x66},
                                 std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0x66}}));
}

TEST(TgaMetadataUVETest, DecodeTgaRgba8ImageUVE_Decodes16BitPaletteToRgba) {
    TgaRgba8ImageUVE image;
    ASSERT_TRUE(DecodeTgaRgba8ImageUVE(MakeTga16PaletteTwoByOneTopLeftUVE(), image));
    EXPECT_EQ(image.width, 2U);
    EXPECT_EQ(image.height, 1U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}}));
}

TEST(TgaMetadataUVETest, DecodeTgaRgba8ImageUVE_DecodesRle16BitPaletteToRgba) {
    TgaRgba8ImageUVE image;
    ASSERT_TRUE(DecodeTgaRgba8ImageUVE(MakeTga16PaletteRleThreeByOneTopLeftUVE(), image));
    EXPECT_EQ(image.width, 3U);
    EXPECT_EQ(image.height, 1U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0xFF},
                                 std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0xFF}}));
}

TEST(TgaMetadataUVETest, DecodeTgaRgba8ImageUVE_Decodes32BitTrueColorAlphaToRgba) {
    TgaRgba8ImageUVE image;
    ASSERT_TRUE(DecodeTgaRgba8ImageUVE(MakeTga32TrueColorAlphaTwoByOneTopLeftUVE(), image));
    EXPECT_EQ(image.width, 2U);
    EXPECT_EQ(image.height, 1U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0x40},
                                 std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0x90}}));
}

TEST(TgaMetadataUVETest, DecodeTgaRgba8ImageUVE_DecodesRle32BitTrueColorAlphaToRgba) {
    TgaRgba8ImageUVE image;
    ASSERT_TRUE(DecodeTgaRgba8ImageUVE(MakeTga32TrueColorAlphaRleThreeByOneTopLeftUVE(), image));
    EXPECT_EQ(image.width, 3U);
    EXPECT_EQ(image.height, 1U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{
                                 std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0x66},
                                 std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0x66},
                                 std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0x66}}));
}

TEST(TgaMetadataUVETest, DecodeTgaRgba8ImageUVE_DecodesTopRight32BitWithOpaqueAlpha) {
    TgaRgba8ImageUVE image;
    ASSERT_TRUE(DecodeTgaRgba8ImageUVE(MakeTga32OneByTwoTopRightUVE(), image));
    EXPECT_EQ(image.width, 1U);
    EXPECT_EQ(image.height, 2U);
    EXPECT_EQ(image.pixels, (std::vector<std::byte>{std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF},
                                                      std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0xFF}}));
}

TEST(TgaMetadataUVETest, DecodeTgaRgba8ImageUVE_RejectsMalformedInputAtomically) {
    const TgaRgba8ImageUVE original{1U, 1U, {std::byte{0x11}, std::byte{0x22}, std::byte{0x33}, std::byte{0x44}}};
    TgaRgba8ImageUVE image = original;
    std::vector<std::byte> truncated = MakeTga24TwoByTwoBottomLeftUVE();
    truncated.resize(29U);
    EXPECT_FALSE(DecodeTgaRgba8ImageUVE(truncated, image));
    EXPECT_EQ(image.width, original.width);
    EXPECT_EQ(image.height, original.height);
    EXPECT_EQ(image.pixels, original.pixels);
    std::vector<std::byte> invalidDescriptor = MakeTga24TwoByTwoBottomLeftUVE();
    invalidDescriptor[17] = std::byte{0x40};
    EXPECT_FALSE(DecodeTgaRgba8ImageUVE(invalidDescriptor, image));
    EXPECT_EQ(image.pixels, original.pixels);
    std::vector<std::byte> truncatedRle = MakeTga24RleTwoByTwoTopLeftUVE();
    truncatedRle.resize(truncatedRle.size() - 1U);
    EXPECT_FALSE(DecodeTgaRgba8ImageUVE(truncatedRle, image));
    std::vector<std::byte> invalidGrayscaleDepth = MakeTga8GrayscaleTwoByOneBottomRightUVE();
    invalidGrayscaleDepth[16] = std::byte{24};
    EXPECT_FALSE(DecodeTgaRgba8ImageUVE(invalidGrayscaleDepth, image));
    std::vector<std::byte> invalidTrueColorDescriptor = MakeTga16TrueColorTwoByOneTopLeftUVE();
    invalidTrueColorDescriptor[17] = std::byte{0x22};
    EXPECT_FALSE(DecodeTgaRgba8ImageUVE(invalidTrueColorDescriptor, image));
    std::vector<std::byte> invalidPaletteDescriptor = MakeTga16PaletteTwoByOneTopLeftUVE();
    invalidPaletteDescriptor[17] = std::byte{0x22};
    EXPECT_FALSE(DecodeTgaRgba8ImageUVE(invalidPaletteDescriptor, image));
    std::vector<std::byte> invalidPaletteIndex = MakeTga8PaletteTwoByOneTopLeftUVE();
    invalidPaletteIndex[25] = std::byte{2};
    EXPECT_FALSE(DecodeTgaRgba8ImageUVE(invalidPaletteIndex, image));
    std::vector<std::byte> truncatedPalette = MakeTga8PaletteTwoByOneTopLeftUVE();
    truncatedPalette.resize(20U);
    EXPECT_FALSE(DecodeTgaRgba8ImageUVE(truncatedPalette, image));
    std::vector<std::byte> oversizedImageId = MakeTga8PaletteTwoByOneTopLeftUVE();
    oversizedImageId[0] = std::byte{0xFF};
    EXPECT_FALSE(DecodeTgaRgba8ImageUVE(oversizedImageId, image));
    EXPECT_EQ(image.width, original.width);
    EXPECT_EQ(image.height, original.height);
    EXPECT_EQ(image.pixels, original.pixels);
}

} // namespace
} // namespace UVE::Asset::Tests
