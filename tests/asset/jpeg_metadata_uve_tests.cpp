// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/asset/jpeg_metadata_uve.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>
#include <gtest/gtest.h>
#include <jpeglib.h>
namespace UVE::Asset::Tests {
namespace {
void AddSofUVE(std::vector<std::byte>& bytes, const std::uint8_t marker, const std::uint16_t width,
              const std::uint16_t height, const std::uint8_t components) {
    bytes.push_back(std::byte{0xFF}); bytes.push_back(std::byte{marker});
    const std::uint16_t length = static_cast<std::uint16_t>(8U + 3U * components);
    bytes.push_back(std::byte{static_cast<unsigned char>(length >> 8U)});
    bytes.push_back(std::byte{static_cast<unsigned char>(length & 0xFFU)});
    bytes.push_back(std::byte{8}); bytes.push_back(std::byte{static_cast<unsigned char>(height >> 8U)});
    bytes.push_back(std::byte{static_cast<unsigned char>(height & 0xFFU)}); bytes.push_back(std::byte{static_cast<unsigned char>(width >> 8U)});
    bytes.push_back(std::byte{static_cast<unsigned char>(width & 0xFFU)}); bytes.push_back(std::byte{components});
    for (std::uint8_t index = 0U; index < components; ++index) bytes.insert(bytes.end(), {std::byte{static_cast<unsigned char>(index + 1U)}, std::byte{0x11}, std::byte{0}});
}

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

TEST(JpegMetadataUVETest, DecodeJpegRgba8ImageUVE_ExpandsRgbToRgba) {
    JpegRgba8ImageUVE image;
    ASSERT_TRUE(DecodeJpegRgba8ImageUVE(MakeOneByOneJpegUVE(), image));
    EXPECT_EQ(image.width, 1U);
    EXPECT_EQ(image.height, 1U);
    ASSERT_EQ(image.pixels.size(), 4U);
    EXPECT_GT(std::to_integer<unsigned int>(image.pixels[0]), 200U);
    EXPECT_LT(std::to_integer<unsigned int>(image.pixels[1]), 50U);
    EXPECT_LT(std::to_integer<unsigned int>(image.pixels[2]), 50U);
    EXPECT_EQ(image.pixels[3], std::byte{0xFF});
}

TEST(JpegMetadataUVETest, DecodeJpegRgba8ImageUVE_DecodesProgressiveRgbToRgba) {
    const auto bytes = MakeOneByOneJpegUVE(true);
    const auto metadata = ParseJpegMetadataUVE(bytes);
    ASSERT_TRUE(metadata.has_value());
    ASSERT_TRUE(metadata->progressive);
    JpegRgba8ImageUVE image;
    ASSERT_TRUE(DecodeJpegRgba8ImageUVE(bytes, image));
    ASSERT_EQ(image.pixels.size(), 4U);
    EXPECT_GT(std::to_integer<unsigned int>(image.pixels[0]), 200U);
    EXPECT_LT(std::to_integer<unsigned int>(image.pixels[1]), 50U);
    EXPECT_LT(std::to_integer<unsigned int>(image.pixels[2]), 50U);
    EXPECT_EQ(image.pixels[3], std::byte{0xFF});
}

TEST(JpegMetadataUVETest, DecodeJpegRgba8ImageUVE_RejectsInvalidInputAtomically) {
    JpegRgba8ImageUVE image{7U, 8U, {std::byte{0xAA}}};
    EXPECT_FALSE(DecodeJpegRgba8ImageUVE({std::byte{0xFF}, std::byte{0xD8}, std::byte{0x00}}, image));
    EXPECT_EQ(image.width, 7U);
    EXPECT_EQ(image.height, 8U);
    EXPECT_EQ(image.pixels, std::vector<std::byte>({std::byte{0xAA}}));
}

TEST(JpegMetadataUVETest, ValidateJpegRgba8PixelBudgetUVE_AcceptsBaselineFrame) {
    const JpegMetadataUVE metadata{640U, 480U, 8U, 3U, false};
    EXPECT_TRUE(ValidateJpegRgba8PixelBudgetUVE(metadata));
}

TEST(JpegMetadataUVETest, ValidateJpegRgba8PixelBudgetUVE_RejectsInvalidOrOversizedFacts) {
    EXPECT_FALSE(ValidateJpegRgba8PixelBudgetUVE(JpegMetadataUVE{0U, 480U, 8U, 3U, false}));
    EXPECT_FALSE(ValidateJpegRgba8PixelBudgetUVE(JpegMetadataUVE{640U, 480U, 12U, 3U, false}));
    EXPECT_FALSE(ValidateJpegRgba8PixelBudgetUVE(JpegMetadataUVE{4U, 4U, 8U, 3U, false}, 63ULL));
    EXPECT_TRUE(ValidateJpegRgba8PixelBudgetUVE(JpegMetadataUVE{4U, 4U, 8U, 3U, false}, 64ULL));
    EXPECT_FALSE(ValidateJpegRgba8PixelBudgetUVE(
        JpegMetadataUVE{0xFFFFFFFFU, 0xFFFFFFFFU, 8U, 4U, false},
        std::numeric_limits<std::uint64_t>::max()));
}

TEST(JpegMetadataUVETest, ParseJpegMetadataUVE_ReturnsBaselineFrameFacts) {
    std::vector<std::byte> bytes{std::byte{0xFF}, std::byte{0xD8}}; AddSofUVE(bytes, 0xC0U, 640U, 480U, 3U);
    const auto metadata = ParseJpegMetadataUVE(bytes); ASSERT_TRUE(metadata.has_value());
    EXPECT_EQ(metadata->width, 640U); EXPECT_EQ(metadata->height, 480U); EXPECT_EQ(metadata->precision, 8U);
    EXPECT_EQ(metadata->componentCount, 3U); EXPECT_FALSE(metadata->progressive);
}
TEST(JpegMetadataUVETest, ParseJpegMetadataUVE_ReturnsProgressiveFrameFactsAfterAppSegment) {
    std::vector<std::byte> bytes{std::byte{0xFF}, std::byte{0xD8}, std::byte{0xFF}, std::byte{0xE0}, std::byte{0}, std::byte{2}};
    AddSofUVE(bytes, 0xC2U, 32U, 16U, 1U); const auto metadata = ParseJpegMetadataUVE(bytes); ASSERT_TRUE(metadata.has_value());
    EXPECT_TRUE(metadata->progressive); EXPECT_EQ(metadata->componentCount, 1U);
}
TEST(JpegMetadataUVETest, ParseJpegMetadataUVE_RejectsMalformedInput) {
    EXPECT_FALSE(ParseJpegMetadataUVE({std::byte{0xFF}, std::byte{0xD8}, std::byte{0xFF}}).has_value());
    std::vector<std::byte> bad{std::byte{0xFF}, std::byte{0xD8}}; AddSofUVE(bad, 0xC0U, 1U, 1U, 5U);
    EXPECT_FALSE(ParseJpegMetadataUVE(bad).has_value());
}
} }
