// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/png_metadata_uve.h"

#include <cstddef>
#include <cstdint>
#include <limits>
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

TEST(PngMetadataUVETest, UnfilterPngRgba8ScanlineUVE_ReconstructsAllSupportedFilters) {
    const std::vector<std::byte> original{std::byte{10}, std::byte{20}, std::byte{30}, std::byte{40},
                                           std::byte{50}, std::byte{60}, std::byte{70}, std::byte{80}};
    const std::vector<std::byte> previous{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4},
                                            std::byte{5}, std::byte{6}, std::byte{7}, std::byte{8}};
    std::vector<std::byte> output;
    ASSERT_TRUE(UnfilterPngRgba8ScanlineUVE(PngFilterTypeUVE::None, original, {}, output));
    EXPECT_EQ(output, original);
    ASSERT_TRUE(UnfilterPngRgba8ScanlineUVE(PngFilterTypeUVE::Sub,
                                            {std::byte{10}, std::byte{20}, std::byte{30}, std::byte{40},
                                             std::byte{40}, std::byte{40}, std::byte{40}, std::byte{40}}, {}, output));
    EXPECT_EQ(output, original);
    ASSERT_TRUE(UnfilterPngRgba8ScanlineUVE(PngFilterTypeUVE::Up,
                                            {std::byte{9}, std::byte{18}, std::byte{27}, std::byte{36},
                                             std::byte{45}, std::byte{54}, std::byte{63}, std::byte{72}}, previous, output));
    EXPECT_EQ(output, original);
    ASSERT_TRUE(UnfilterPngRgba8ScanlineUVE(PngFilterTypeUVE::Average,
                                            {std::byte{10}, std::byte{19}, std::byte{29}, std::byte{38},
                                             std::byte{43}, std::byte{47}, std::byte{52}, std::byte{56}}, previous, output));
    EXPECT_EQ(output, original);
    ASSERT_TRUE(UnfilterPngRgba8ScanlineUVE(PngFilterTypeUVE::Paeth,
                                            {std::byte{9}, std::byte{18}, std::byte{27}, std::byte{36},
                                             std::byte{40}, std::byte{40}, std::byte{40}, std::byte{40}}, previous, output));
    EXPECT_EQ(output, original);
}

TEST(PngMetadataUVETest, UnfilterPngRgba8ScanlineUVE_RejectsInvalidInputsAtomically) {
    const std::vector<std::byte> original{std::byte{0xAA}, std::byte{0xBB}};
    std::vector<std::byte> output = original;
    EXPECT_FALSE(UnfilterPngRgba8ScanlineUVE(static_cast<PngFilterTypeUVE>(9U), {std::byte{1}, std::byte{2}}, {}, output));
    EXPECT_EQ(output, original);
    EXPECT_FALSE(UnfilterPngRgba8ScanlineUVE(PngFilterTypeUVE::Up, {std::byte{1}, std::byte{2}}, {}, output));
    EXPECT_EQ(output, original);
    EXPECT_FALSE(UnfilterPngRgba8ScanlineUVE(PngFilterTypeUVE::None, {std::byte{1}, std::byte{2}}, {std::byte{1}}, output));
    EXPECT_EQ(output, original);
    EXPECT_FALSE(UnfilterPngRgba8ScanlineUVE(PngFilterTypeUVE::None, {}, {}, output));
    EXPECT_EQ(output, original);
    const std::vector<std::byte> oversized(kMaximumPngRgba8ScanlineBytesUVE + 1U, std::byte{0});
    EXPECT_FALSE(UnfilterPngRgba8ScanlineUVE(PngFilterTypeUVE::None, oversized, {}, output));
    EXPECT_EQ(output, original);
}

TEST(PngMetadataUVETest, ValidatePngRgba8PixelBudgetUVE_AcceptsDefaultHdBudget) {
    const std::optional<PngMetadataUVE> metadata = ParsePngMetadataUVE(MakePngIhdrUVE());
    ASSERT_TRUE(metadata.has_value());
    EXPECT_TRUE(ValidatePngRgba8PixelBudgetUVE(*metadata));
}

TEST(PngMetadataUVETest, ValidatePngRgba8PixelBudgetUVE_RejectsZeroOrNonRgba8Facts) {
    PngMetadataUVE zeroWidth{.width = 0U, .height = 1080U, .bitDepth = 8U, .colorType = 6U};
    EXPECT_FALSE(ValidatePngRgba8PixelBudgetUVE(zeroWidth));

    PngMetadataUVE rgb{.width = 1920U, .height = 1080U, .bitDepth = 8U, .colorType = 2U};
    EXPECT_FALSE(ValidatePngRgba8PixelBudgetUVE(rgb));
}

TEST(PngMetadataUVETest, ValidatePngRgba8PixelBudgetUVE_RejectsOverflowAndOversizedBudget) {
    PngMetadataUVE huge{.width = 0xFFFFFFFFU, .height = 0xFFFFFFFFU, .bitDepth = 8U, .colorType = 6U};
    EXPECT_FALSE(ValidatePngRgba8PixelBudgetUVE(huge, std::numeric_limits<std::uint64_t>::max()));

    PngMetadataUVE small{.width = 4U, .height = 4U, .bitDepth = 8U, .colorType = 6U};
    EXPECT_FALSE(ValidatePngRgba8PixelBudgetUVE(small, 63ULL));
    EXPECT_TRUE(ValidatePngRgba8PixelBudgetUVE(small, 64ULL));
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
