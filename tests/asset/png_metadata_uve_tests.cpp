// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/png_metadata_uve.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

namespace UVE::Asset::Tests {
namespace {

void AppendU32BEUVE(std::vector<std::byte>& bytes, const std::uint32_t value) {
    for (unsigned int shift = 24U;; shift -= 8U) {
        bytes.push_back(std::byte{static_cast<unsigned char>((value >> shift) & 0xFFU)});
        if (shift == 0U) {
            break;
        }
    }
}

[[nodiscard]] std::vector<std::byte> MakePngIhdrUVE(const std::uint32_t width = 1920U,
                                                    const std::uint32_t height = 1080U,
                                                    const std::uint8_t bitDepth = 8U,
                                                    const std::uint8_t colorType = 6U,
                                                    const std::uint8_t interlace = 0U) {
    std::vector<std::byte> bytes{
        std::byte{137}, std::byte{80}, std::byte{78}, std::byte{71},
        std::byte{13}, std::byte{10}, std::byte{26}, std::byte{10},
    };
    AppendU32BEUVE(bytes, 13U);
    bytes.insert(bytes.end(), {std::byte{'I'}, std::byte{'H'}, std::byte{'D'}, std::byte{'R'}});
    AppendU32BEUVE(bytes, width);
    AppendU32BEUVE(bytes, height);
    bytes.push_back(std::byte{bitDepth});
    bytes.push_back(std::byte{colorType});
    bytes.push_back(std::byte{0});
    bytes.push_back(std::byte{0});
    bytes.push_back(std::byte{interlace});
    bytes.insert(bytes.end(), {std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}});
    return bytes;
}

TEST(PngMetadataUVETest, ParsePngMetadataUVE_ReturnsRgbaIhdrFacts) {
    const std::optional<PngMetadataUVE> metadata = ParsePngMetadataUVE(MakePngIhdrUVE());
    ASSERT_TRUE(metadata.has_value());
    EXPECT_EQ(metadata->width, 1920U);
    EXPECT_EQ(metadata->height, 1080U);
    EXPECT_EQ(metadata->bitDepth, 8U);
    EXPECT_EQ(metadata->colorType, 6U);
    EXPECT_TRUE(metadata->hasAlpha);
    EXPECT_EQ(metadata->interlaceMethod, 0U);
}

TEST(PngMetadataUVETest, ParsePngMetadataUVE_ReportsIndexedColorWithoutAlpha) {
    const std::optional<PngMetadataUVE> metadata = ParsePngMetadataUVE(MakePngIhdrUVE(64U, 32U, 4U, 3U, 1U));
    ASSERT_TRUE(metadata.has_value());
    EXPECT_EQ(metadata->width, 64U);
    EXPECT_EQ(metadata->height, 32U);
    EXPECT_EQ(metadata->bitDepth, 4U);
    EXPECT_EQ(metadata->colorType, 3U);
    EXPECT_FALSE(metadata->hasAlpha);
    EXPECT_EQ(metadata->interlaceMethod, 1U);
}

TEST(PngMetadataUVETest, ParsePngMetadataUVE_RejectsMalformedIhdrFacts) {
    std::vector<std::byte> badSignature = MakePngIhdrUVE();
    badSignature[0] = std::byte{'X'};
    EXPECT_FALSE(ParsePngMetadataUVE(badSignature).has_value());

    std::vector<std::byte> badDepth = MakePngIhdrUVE(1U, 1U, 4U, 2U);
    EXPECT_FALSE(ParsePngMetadataUVE(badDepth).has_value());

    std::vector<std::byte> badMethod = MakePngIhdrUVE(1U, 1U, 8U, 6U);
    badMethod[26] = std::byte{1};
    EXPECT_FALSE(ParsePngMetadataUVE(badMethod).has_value());

    std::vector<std::byte> truncated = MakePngIhdrUVE();
    truncated.resize(20U);
    EXPECT_FALSE(ParsePngMetadataUVE(truncated).has_value());
}

} // namespace
} // namespace UVE::Asset::Tests
