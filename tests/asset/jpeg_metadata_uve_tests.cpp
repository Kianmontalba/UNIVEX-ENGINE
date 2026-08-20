// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/asset/jpeg_metadata_uve.h"
#include <cstdint>
#include <limits>
#include <vector>
#include <gtest/gtest.h>
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
