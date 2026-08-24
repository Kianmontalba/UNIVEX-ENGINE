// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/png_metadata_uve.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include <gtest/gtest.h>
#include <zlib.h>

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

[[nodiscard]] std::vector<std::byte> MakePngOneByOneUVE(
    const std::uint8_t colorType, const std::vector<std::byte>& raw,
    const std::vector<std::byte>& palette = {}, const std::vector<std::byte>& alpha = {},
    const std::uint8_t bitDepth = 8U, const std::uint32_t width = 1U,
    const std::uint32_t height = 1U, const std::uint8_t interlace = 0U,
    const std::vector<std::byte>& transparency = {}) {
    std::vector<std::byte> png{
        std::byte{0x89}, std::byte{0x50}, std::byte{0x4E}, std::byte{0x47}, std::byte{0x0D}, std::byte{0x0A},
        std::byte{0x1A}, std::byte{0x0A}};
    const auto appendChunk = [](std::vector<std::byte>& output, const std::vector<std::byte>& type,
                                const std::vector<std::byte>& payload) {
        AppendU32BEUVE(output, static_cast<std::uint32_t>(payload.size()));
        output.insert(output.end(), type.begin(), type.end());
        output.insert(output.end(), payload.begin(), payload.end());
        std::vector<std::byte> crcInput(type);
        crcInput.insert(crcInput.end(), payload.begin(), payload.end());
        const uLong crc = crc32(0L, reinterpret_cast<const Bytef*>(crcInput.data()), static_cast<uInt>(crcInput.size()));
        AppendU32BEUVE(output, static_cast<std::uint32_t>(crc));
    };
    appendChunk(png, {std::byte{'I'}, std::byte{'H'}, std::byte{'D'}, std::byte{'R'}},
                {std::byte{static_cast<unsigned char>((width >> 24U) & 0xFFU)},
                 std::byte{static_cast<unsigned char>((width >> 16U) & 0xFFU)},
                 std::byte{static_cast<unsigned char>((width >> 8U) & 0xFFU)},
                 std::byte{static_cast<unsigned char>(width & 0xFFU)},
                 std::byte{static_cast<unsigned char>((height >> 24U) & 0xFFU)},
                 std::byte{static_cast<unsigned char>((height >> 16U) & 0xFFU)},
                 std::byte{static_cast<unsigned char>((height >> 8U) & 0xFFU)},
                 std::byte{static_cast<unsigned char>(height & 0xFFU)},
                 std::byte{bitDepth}, std::byte{colorType}, std::byte{0}, std::byte{0}, std::byte{interlace}});
    if (colorType == 3U) {
        if (palette.empty() || palette.size() % 3U != 0U || palette.size() > 768U ||
            (!alpha.empty() && alpha.size() > palette.size() / 3U)) {
            return {};
        }
        appendChunk(png, {std::byte{'P'}, std::byte{'L'}, std::byte{'T'}, std::byte{'E'}}, palette);
        if (!alpha.empty()) appendChunk(png, {std::byte{'t'}, std::byte{'R'}, std::byte{'N'}, std::byte{'S'}}, alpha);
    } else if (!transparency.empty()) {
        appendChunk(png, {std::byte{'t'}, std::byte{'R'}, std::byte{'N'}, std::byte{'S'}}, transparency);
    }
    uLongf compressedSize = compressBound(static_cast<uLong>(raw.size()));
    std::vector<std::byte> compressed(static_cast<std::size_t>(compressedSize));
    if (compress2(reinterpret_cast<Bytef*>(compressed.data()), &compressedSize,
                  reinterpret_cast<const Bytef*>(raw.data()), static_cast<uLong>(raw.size()), Z_BEST_SPEED) != Z_OK) {
        return {};
    }
    compressed.resize(static_cast<std::size_t>(compressedSize));
    appendChunk(png, {std::byte{'I'}, std::byte{'D'}, std::byte{'A'}, std::byte{'T'}}, compressed);
    appendChunk(png, {std::byte{'I'}, std::byte{'E'}, std::byte{'N'}, std::byte{'D'}}, {});
    return png;
}

[[nodiscard]] std::vector<std::byte> MakePngRgbOneByOneUVE() {
    return MakePngOneByOneUVE(2U, {std::byte{0}, std::byte{0xFF}, std::byte{0}, std::byte{0}});
}

[[nodiscard]] std::vector<std::byte> MakePngGrayOneByOneUVE() {
    return MakePngOneByOneUVE(0U, {std::byte{0}, std::byte{0x80}});
}

[[nodiscard]] std::vector<std::byte> MakePngGrayAlphaOneByOneUVE() {
    return MakePngOneByOneUVE(4U, {std::byte{0}, std::byte{0x80}, std::byte{0xC0}});
}

[[nodiscard]] std::vector<std::byte> MakePngGray1BitEightByOneUVE() {
    return MakePngOneByOneUVE(0U, {std::byte{0}, std::byte{0x59}}, {}, {}, 1U, 8U, 1U);
}

[[nodiscard]] std::vector<std::byte> MakePngGray2BitFourByOneUVE() {
    return MakePngOneByOneUVE(0U, {std::byte{0}, std::byte{0x1B}}, {}, {}, 2U, 4U, 1U);
}

[[nodiscard]] std::vector<std::byte> MakePngGray4BitTwoByOneUVE() {
    return MakePngOneByOneUVE(0U, {std::byte{0}, std::byte{0x1E}}, {}, {}, 4U, 2U, 1U);
}

[[nodiscard]] std::vector<std::byte> MakePngIndexed1BitEightByOneUVE() {
    return MakePngOneByOneUVE(3U, {std::byte{0}, std::byte{0x59}},
                               {std::byte{0xFF}, std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0xFF}, std::byte{0}},
                               {std::byte{0xFF}, std::byte{0x80}}, 1U, 8U, 1U);
}

[[nodiscard]] std::vector<std::byte> MakePngIndexed2BitFourByOneUVE() {
    return MakePngOneByOneUVE(3U, {std::byte{0}, std::byte{0x1B}},
                               {std::byte{0xFF}, std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0xFF}, std::byte{0},
                                std::byte{0}, std::byte{0}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}},
                               {}, 2U, 4U, 1U);
}

[[nodiscard]] std::vector<std::byte> MakePngIndexed4BitTwoByOneUVE() {
    std::vector<std::byte> palette(16U * 3U, std::byte{0});
    palette[3U] = std::byte{0xFF};
    palette[14U * 3U + 1U] = std::byte{0xFF};
    return MakePngOneByOneUVE(3U, {std::byte{0}, std::byte{0x1E}}, palette, {}, 4U, 2U, 1U);
}

[[nodiscard]] std::vector<std::byte> MakePngAdam7PackedGray1BitTwoByTwoUVE() {
    return MakePngOneByOneUVE(0U, {std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0x80},
                                   std::byte{0}, std::byte{0x80}}, {}, {}, 1U, 2U, 2U, 1U);
}

[[nodiscard]] std::vector<std::byte> MakePngAdam7PackedGray2BitTwoByTwoUVE() {
    return MakePngOneByOneUVE(0U, {std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0xC0},
                                   std::byte{0}, std::byte{0x60}}, {}, {}, 2U, 2U, 2U, 1U);
}

[[nodiscard]] std::vector<std::byte> MakePngAdam7PackedGray4BitTwoByTwoUVE() {
    return MakePngOneByOneUVE(0U, {std::byte{0}, std::byte{0x10}, std::byte{0}, std::byte{0xE0},
                                   std::byte{0}, std::byte{0x2C}}, {}, {}, 4U, 2U, 2U, 1U);
}

[[nodiscard]] std::vector<std::byte> MakePngAdam7PackedIndexed1BitTwoByTwoUVE() {
    return MakePngOneByOneUVE(3U, {std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0x80},
                                   std::byte{0}, std::byte{0x80}},
                               {std::byte{0xFF}, std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0xFF}, std::byte{0}},
                               {std::byte{0xFF}, std::byte{0x80}}, 1U, 2U, 2U, 1U);
}

[[nodiscard]] std::vector<std::byte> MakePngAdam7PackedIndexed2BitTwoByTwoUVE() {
    return MakePngOneByOneUVE(3U, {std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0xC0},
                                   std::byte{0}, std::byte{0x60}},
                               {std::byte{0xFF}, std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0xFF}, std::byte{0},
                                std::byte{0}, std::byte{0}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}},
                               {}, 2U, 2U, 2U, 1U);
}

[[nodiscard]] std::vector<std::byte> MakePngAdam7PackedIndexed4BitTwoByTwoUVE() {
    std::vector<std::byte> palette(16U * 3U, std::byte{0});
    palette[3U] = std::byte{0xFF};
    palette[14U * 3U + 1U] = std::byte{0xFF};
    palette[2U * 3U + 2U] = std::byte{0xFF};
    palette[12U * 3U] = std::byte{0xFF};
    return MakePngOneByOneUVE(3U, {std::byte{0}, std::byte{0x10}, std::byte{0}, std::byte{0xE0},
                                   std::byte{0}, std::byte{0x2C}}, palette, {}, 4U, 2U, 2U, 1U);
}

[[nodiscard]] std::vector<std::byte> MakePngIndexedOneByOneUVE(const std::byte index = std::byte{1}) {
    return MakePngOneByOneUVE(3U, {std::byte{0}, index},
                               {std::byte{0xFF}, std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0xFF}, std::byte{0}},
                               {std::byte{0xFF}, std::byte{0x80}});
}

[[nodiscard]] std::vector<std::byte> MakePngRgba16OneByOneUVE() {
    return MakePngOneByOneUVE(6U, {std::byte{0}, std::byte{0x12}, std::byte{0x34}, std::byte{0xAB}, std::byte{0xCD},
                                   std::byte{0x56}, std::byte{0x78}, std::byte{0x9A}, std::byte{0xBC}},
                               {}, {}, 16U);
}

[[nodiscard]] std::vector<std::byte> MakePngGray16OneByOneUVE() {
    return MakePngOneByOneUVE(0U, {std::byte{0}, std::byte{0x34}, std::byte{0x56}}, {}, {}, 16U);
}

[[nodiscard]] std::vector<std::byte> MakePngRgb16OneByOneUVE() {
    return MakePngOneByOneUVE(2U, {std::byte{0}, std::byte{0x12}, std::byte{0x34}, std::byte{0xAB}, std::byte{0xCD},
                                   std::byte{0xEF}, std::byte{0x01}}, {}, {}, 16U);
}

[[nodiscard]] std::vector<std::byte> MakePngGrayAlpha16OneByOneUVE() {
    return MakePngOneByOneUVE(4U, {std::byte{0}, std::byte{0x34}, std::byte{0x56}, std::byte{0xAB},
                                   std::byte{0xCD}}, {}, {}, 16U);
}

[[nodiscard]] std::vector<std::byte> MakePngAdam7RgbaTwoByTwoUVE() {
    return MakePngOneByOneUVE(6U,
                               {std::byte{0}, std::byte{0xFF}, std::byte{0}, std::byte{0}, std::byte{0xFF},
                                std::byte{0}, std::byte{0}, std::byte{0xFF}, std::byte{0}, std::byte{0xFF},
                                std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0xFF}, std::byte{0xFF},
                                std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}},
                               {}, {}, 8U, 2U, 2U, 1U);
}

[[nodiscard]] std::vector<std::byte> MakePngAdam7RgbTwoByTwoUVE() {
    return MakePngOneByOneUVE(2U,
                               {std::byte{0}, std::byte{0xFF}, std::byte{0}, std::byte{0},
                                std::byte{0}, std::byte{0}, std::byte{0xFF}, std::byte{0},
                                std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0xFF},
                                std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}},
                               {}, {}, 8U, 2U, 2U, 1U);
}

[[nodiscard]] std::vector<std::byte> MakePngAdam7GrayTwoByTwoUVE() {
    return MakePngOneByOneUVE(0U,
                               {std::byte{0}, std::byte{0x10},
                                std::byte{0}, std::byte{0x80},
                                std::byte{0}, std::byte{0xC0}, std::byte{0xFF}},
                               {}, {}, 8U, 2U, 2U, 1U);
}

[[nodiscard]] std::vector<std::byte> MakePngTransparentGray8OneByOneUVE() {
    return MakePngOneByOneUVE(0U, {std::byte{0}, std::byte{0x80}}, {}, {}, 8U, 1U, 1U, 0U,
                                   {std::byte{0}, std::byte{0x80}});
}

[[nodiscard]] std::vector<std::byte> MakePngTransparentRgb8OneByOneUVE() {
    return MakePngOneByOneUVE(2U, {std::byte{0}, std::byte{0xFF}, std::byte{0}, std::byte{0}}, {}, {}, 8U, 1U, 1U, 0U,
                                   {std::byte{0}, std::byte{0xFF}, std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}});
}

[[nodiscard]] std::vector<std::byte> MakePngTransparentGray16OneByOneUVE() {
    return MakePngOneByOneUVE(0U, {std::byte{0}, std::byte{0x34}, std::byte{0x56}}, {}, {}, 16U, 1U, 1U, 0U,
                                   {std::byte{0x34}, std::byte{0x56}});
}

[[nodiscard]] std::vector<std::byte> MakePngAdam7TransparentGrayTwoByTwoUVE() {
    return MakePngOneByOneUVE(0U,
                               {std::byte{0}, std::byte{0x10},
                                std::byte{0}, std::byte{0x80},
                                std::byte{0}, std::byte{0xC0}, std::byte{0xFF}},
                               {}, {}, 8U, 2U, 2U, 1U, {std::byte{0}, std::byte{0x80}});
}

[[nodiscard]] std::vector<std::byte> MakePngAdam7IndexedTwoByTwoUVE() {
    return MakePngOneByOneUVE(3U,
                               {std::byte{0}, std::byte{0},
                                std::byte{0}, std::byte{1},
                                std::byte{0}, std::byte{2}, std::byte{3}},
                               {std::byte{0xFF}, std::byte{0}, std::byte{0},
                                std::byte{0}, std::byte{0xFF}, std::byte{0},
                                std::byte{0}, std::byte{0}, std::byte{0xFF},
                                std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}},
                               {std::byte{0xFF}, std::byte{0x80}, std::byte{0x40}, std::byte{0xFF}},
                               8U, 2U, 2U, 1U);
}

[[nodiscard]] std::vector<std::byte> MakePngAdam7Gray16TwoByTwoUVE() {
    return MakePngOneByOneUVE(0U,
                               {std::byte{0}, std::byte{0x12}, std::byte{0x34},
                                std::byte{0}, std::byte{0xAB}, std::byte{0xCD},
                                std::byte{0}, std::byte{0x10}, std::byte{0x20},
                                std::byte{0xFF}, std::byte{0xEE}},
                               {}, {}, 16U, 2U, 2U, 1U);
}

[[nodiscard]] std::vector<std::byte> MakePngAdam7Rgb16TwoByTwoUVE() {
    return MakePngOneByOneUVE(2U,
                               {std::byte{0}, std::byte{0x12}, std::byte{0x01}, std::byte{0x34}, std::byte{0x02},
                                std::byte{0x56}, std::byte{0x03}, std::byte{0}, std::byte{0xAB}, std::byte{0x05},
                                std::byte{0xCD}, std::byte{0x06}, std::byte{0xEF}, std::byte{0x07}, std::byte{0},
                                std::byte{0x10}, std::byte{0x09}, std::byte{0x20}, std::byte{0x0A}, std::byte{0x30},
                                std::byte{0x0B}, std::byte{0xFF}, std::byte{0x0C}, std::byte{0xEE}, std::byte{0x0D},
                                std::byte{0xDD}, std::byte{0x0E}},
                               {}, {}, 16U, 2U, 2U, 1U);
}

[[nodiscard]] std::vector<std::byte> MakePngAdam7GrayAlpha16TwoByTwoUVE() {
    return MakePngOneByOneUVE(4U,
                               {std::byte{0}, std::byte{0x12}, std::byte{0x34}, std::byte{0xAB}, std::byte{0xCD},
                                std::byte{0}, std::byte{0x56}, std::byte{0x78}, std::byte{0xEF}, std::byte{0x01},
                                std::byte{0}, std::byte{0x10}, std::byte{0x20}, std::byte{0x30}, std::byte{0x40},
                                std::byte{0xFF}, std::byte{0xEE}, std::byte{0x80}, std::byte{0x90}},
                               {}, {}, 16U, 2U, 2U, 1U);
}

[[nodiscard]] std::vector<std::byte> MakePngAdam7GrayAlphaTwoByTwoUVE() {
    return MakePngOneByOneUVE(4U,
                               {std::byte{0}, std::byte{0x12}, std::byte{0xAB},
                                std::byte{0}, std::byte{0x56}, std::byte{0xEF},
                                std::byte{0}, std::byte{0x10}, std::byte{0x30}, std::byte{0xFF}, std::byte{0x80}},
                               {}, {}, 8U, 2U, 2U, 1U);
}

[[nodiscard]] std::vector<std::byte> MakePngAdam7Rgba16TwoByTwoUVE() {
    return MakePngOneByOneUVE(6U,
                               {std::byte{0}, std::byte{0x12}, std::byte{0x01}, std::byte{0x34}, std::byte{0x02},
                                std::byte{0x56}, std::byte{0x03}, std::byte{0x78}, std::byte{0x04},
                                std::byte{0}, std::byte{0xAB}, std::byte{0x05}, std::byte{0xCD}, std::byte{0x06},
                                std::byte{0xEF}, std::byte{0x07}, std::byte{0x01}, std::byte{0x08},
                                std::byte{0}, std::byte{0x10}, std::byte{0x09}, std::byte{0x20}, std::byte{0x0A},
                                std::byte{0x30}, std::byte{0x0B}, std::byte{0x40}, std::byte{0x0C},
                                std::byte{0xFF}, std::byte{0x0D}, std::byte{0xFF}, std::byte{0x0E}, std::byte{0xFF},
                                std::byte{0x0F}, std::byte{0xFF}, std::byte{0x10}},
                               {}, {}, 16U, 2U, 2U, 1U);
}

TEST(PngMetadataUVETest, ValidatePngRgba8PixelBudgetUVE_RejectsZeroAndAcceptsGrayRgbFacts) {
    PngMetadataUVE zeroWidth{.width = 0U, .height = 1080U, .bitDepth = 8U, .colorType = 6U};
    EXPECT_FALSE(ValidatePngRgba8PixelBudgetUVE(zeroWidth));

    PngMetadataUVE gray{.width = 1920U, .height = 1080U, .bitDepth = 8U, .colorType = 0U};
    EXPECT_TRUE(ValidatePngRgba8PixelBudgetUVE(gray));
    PngMetadataUVE rgb{.width = 1920U, .height = 1080U, .bitDepth = 8U, .colorType = 2U};
    EXPECT_TRUE(ValidatePngRgba8PixelBudgetUVE(rgb));
    PngMetadataUVE rgba16{.width = 1U, .height = 1U, .bitDepth = 16U, .colorType = 6U};
    EXPECT_TRUE(ValidatePngRgba8PixelBudgetUVE(rgba16));
    PngMetadataUVE gray16{.width = 1U, .height = 1U, .bitDepth = 16U, .colorType = 0U};
    EXPECT_TRUE(ValidatePngRgba8PixelBudgetUVE(gray16));
    PngMetadataUVE rgb16Accepted{.width = 1U, .height = 1U, .bitDepth = 16U, .colorType = 2U};
    EXPECT_TRUE(ValidatePngRgba8PixelBudgetUVE(rgb16Accepted));
    PngMetadataUVE indexed16{.width = 1U, .height = 1U, .bitDepth = 16U, .colorType = 3U};
    EXPECT_FALSE(ValidatePngRgba8PixelBudgetUVE(indexed16));
}

TEST(PngMetadataUVETest, ValidatePngRgba8PixelBudgetUVE_RejectsOverflowAndOversizedBudget) {
    PngMetadataUVE huge{.width = 0xFFFFFFFFU, .height = 0xFFFFFFFFU, .bitDepth = 8U, .colorType = 6U};
    EXPECT_FALSE(ValidatePngRgba8PixelBudgetUVE(huge, std::numeric_limits<std::uint64_t>::max()));

    PngMetadataUVE small{.width = 4U, .height = 4U, .bitDepth = 8U, .colorType = 6U};
    EXPECT_FALSE(ValidatePngRgba8PixelBudgetUVE(small, 63ULL));
    EXPECT_TRUE(ValidatePngRgba8PixelBudgetUVE(small, 64ULL));
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DownconvertsAdam7Gray16TwoByTwo) {
    const std::vector<std::byte> png = MakePngAdam7Gray16TwoByTwoUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.width, 2U);
    ASSERT_EQ(image.height, 2U);
    ASSERT_EQ(image.pixels.size(), 16U);
    EXPECT_EQ(image.pixels[0], std::byte{0x12});
    EXPECT_EQ(image.pixels[1], std::byte{0x12});
    EXPECT_EQ(image.pixels[2], std::byte{0x12});
    EXPECT_EQ(image.pixels[3], std::byte{0xFF});
    EXPECT_EQ(image.pixels[4], std::byte{0xAB});
    EXPECT_EQ(image.pixels[5], std::byte{0xAB});
    EXPECT_EQ(image.pixels[6], std::byte{0xAB});
    EXPECT_EQ(image.pixels[7], std::byte{0xFF});
    EXPECT_EQ(image.pixels[8], std::byte{0x10});
    EXPECT_EQ(image.pixels[9], std::byte{0x10});
    EXPECT_EQ(image.pixels[10], std::byte{0x10});
    EXPECT_EQ(image.pixels[11], std::byte{0xFF});
    EXPECT_EQ(image.pixels[12], std::byte{0xFF});
    EXPECT_EQ(image.pixels[13], std::byte{0xFF});
    EXPECT_EQ(image.pixels[14], std::byte{0xFF});
    EXPECT_EQ(image.pixels[15], std::byte{0xFF});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DownconvertsAdam7Rgb16TwoByTwo) {
    const std::vector<std::byte> png = MakePngAdam7Rgb16TwoByTwoUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.width, 2U);
    ASSERT_EQ(image.height, 2U);
    ASSERT_EQ(image.pixels.size(), 16U);
    EXPECT_EQ(image.pixels[0], std::byte{0x12});
    EXPECT_EQ(image.pixels[1], std::byte{0x34});
    EXPECT_EQ(image.pixels[2], std::byte{0x56});
    EXPECT_EQ(image.pixels[3], std::byte{0xFF});
    EXPECT_EQ(image.pixels[4], std::byte{0xAB});
    EXPECT_EQ(image.pixels[5], std::byte{0xCD});
    EXPECT_EQ(image.pixels[6], std::byte{0xEF});
    EXPECT_EQ(image.pixels[7], std::byte{0xFF});
    EXPECT_EQ(image.pixels[8], std::byte{0x10});
    EXPECT_EQ(image.pixels[9], std::byte{0x20});
    EXPECT_EQ(image.pixels[10], std::byte{0x30});
    EXPECT_EQ(image.pixels[11], std::byte{0xFF});
    EXPECT_EQ(image.pixels[12], std::byte{0xFF});
    EXPECT_EQ(image.pixels[13], std::byte{0xEE});
    EXPECT_EQ(image.pixels[14], std::byte{0xDD});
    EXPECT_EQ(image.pixels[15], std::byte{0xFF});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DownconvertsAdam7GrayAlpha16TwoByTwo) {
    const std::vector<std::byte> png = MakePngAdam7GrayAlpha16TwoByTwoUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.width, 2U);
    ASSERT_EQ(image.height, 2U);
    ASSERT_EQ(image.pixels.size(), 16U);
    EXPECT_EQ(image.pixels[0], std::byte{0x12});
    EXPECT_EQ(image.pixels[1], std::byte{0x12});
    EXPECT_EQ(image.pixels[2], std::byte{0x12});
    EXPECT_EQ(image.pixels[3], std::byte{0xAB});
    EXPECT_EQ(image.pixels[4], std::byte{0x56});
    EXPECT_EQ(image.pixels[5], std::byte{0x56});
    EXPECT_EQ(image.pixels[6], std::byte{0x56});
    EXPECT_EQ(image.pixels[7], std::byte{0xEF});
    EXPECT_EQ(image.pixels[8], std::byte{0x10});
    EXPECT_EQ(image.pixels[9], std::byte{0x10});
    EXPECT_EQ(image.pixels[10], std::byte{0x10});
    EXPECT_EQ(image.pixels[11], std::byte{0x30});
    EXPECT_EQ(image.pixels[12], std::byte{0xFF});
    EXPECT_EQ(image.pixels[13], std::byte{0xFF});
    EXPECT_EQ(image.pixels[14], std::byte{0xFF});
    EXPECT_EQ(image.pixels[15], std::byte{0x80});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DecodesAdam7GrayAlphaTwoByTwo) {
    const std::vector<std::byte> png = MakePngAdam7GrayAlphaTwoByTwoUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.width, 2U);
    ASSERT_EQ(image.height, 2U);
    ASSERT_EQ(image.pixels.size(), 16U);
    EXPECT_EQ(image.pixels[0], std::byte{0x12});
    EXPECT_EQ(image.pixels[1], std::byte{0x12});
    EXPECT_EQ(image.pixels[2], std::byte{0x12});
    EXPECT_EQ(image.pixels[3], std::byte{0xAB});
    EXPECT_EQ(image.pixels[4], std::byte{0x56});
    EXPECT_EQ(image.pixels[5], std::byte{0x56});
    EXPECT_EQ(image.pixels[6], std::byte{0x56});
    EXPECT_EQ(image.pixels[7], std::byte{0xEF});
    EXPECT_EQ(image.pixels[8], std::byte{0x10});
    EXPECT_EQ(image.pixels[9], std::byte{0x10});
    EXPECT_EQ(image.pixels[10], std::byte{0x10});
    EXPECT_EQ(image.pixels[11], std::byte{0x30});
    EXPECT_EQ(image.pixels[12], std::byte{0xFF});
    EXPECT_EQ(image.pixels[13], std::byte{0xFF});
    EXPECT_EQ(image.pixels[14], std::byte{0xFF});
    EXPECT_EQ(image.pixels[15], std::byte{0x80});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DownconvertsAdam7Rgba16TwoByTwo) {
    const std::vector<std::byte> png = MakePngAdam7Rgba16TwoByTwoUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.width, 2U);
    ASSERT_EQ(image.height, 2U);
    ASSERT_EQ(image.pixels.size(), 16U);
    EXPECT_EQ(image.pixels[0], std::byte{0x12});
    EXPECT_EQ(image.pixels[1], std::byte{0x34});
    EXPECT_EQ(image.pixels[2], std::byte{0x56});
    EXPECT_EQ(image.pixels[3], std::byte{0x78});
    EXPECT_EQ(image.pixels[4], std::byte{0xAB});
    EXPECT_EQ(image.pixels[5], std::byte{0xCD});
    EXPECT_EQ(image.pixels[6], std::byte{0xEF});
    EXPECT_EQ(image.pixels[7], std::byte{0x01});
    EXPECT_EQ(image.pixels[8], std::byte{0x10});
    EXPECT_EQ(image.pixels[9], std::byte{0x20});
    EXPECT_EQ(image.pixels[10], std::byte{0x30});
    EXPECT_EQ(image.pixels[11], std::byte{0x40});
    EXPECT_EQ(image.pixels[12], std::byte{0xFF});
    EXPECT_EQ(image.pixels[13], std::byte{0xFF});
    EXPECT_EQ(image.pixels[14], std::byte{0xFF});
    EXPECT_EQ(image.pixels[15], std::byte{0xFF});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DecodesAdam7IndexedTwoByTwo) {
    const std::vector<std::byte> png = MakePngAdam7IndexedTwoByTwoUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.width, 2U);
    ASSERT_EQ(image.height, 2U);
    ASSERT_EQ(image.pixels.size(), 16U);
    EXPECT_EQ(image.pixels[0], std::byte{0xFF});
    EXPECT_EQ(image.pixels[1], std::byte{0});
    EXPECT_EQ(image.pixels[2], std::byte{0});
    EXPECT_EQ(image.pixels[3], std::byte{0xFF});
    EXPECT_EQ(image.pixels[4], std::byte{0});
    EXPECT_EQ(image.pixels[5], std::byte{0xFF});
    EXPECT_EQ(image.pixels[6], std::byte{0});
    EXPECT_EQ(image.pixels[7], std::byte{0x80});
    EXPECT_EQ(image.pixels[8], std::byte{0});
    EXPECT_EQ(image.pixels[9], std::byte{0});
    EXPECT_EQ(image.pixels[10], std::byte{0xFF});
    EXPECT_EQ(image.pixels[11], std::byte{0x40});
    EXPECT_EQ(image.pixels[12], std::byte{0xFF});
    EXPECT_EQ(image.pixels[13], std::byte{0xFF});
    EXPECT_EQ(image.pixels[14], std::byte{0xFF});
    EXPECT_EQ(image.pixels[15], std::byte{0xFF});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DecodesAdam7GrayTwoByTwo) {
    const std::vector<std::byte> png = MakePngAdam7GrayTwoByTwoUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.width, 2U);
    ASSERT_EQ(image.height, 2U);
    ASSERT_EQ(image.pixels.size(), 16U);
    EXPECT_EQ(image.pixels[0], std::byte{0x10});
    EXPECT_EQ(image.pixels[1], std::byte{0x10});
    EXPECT_EQ(image.pixels[2], std::byte{0x10});
    EXPECT_EQ(image.pixels[3], std::byte{0xFF});
    EXPECT_EQ(image.pixels[4], std::byte{0x80});
    EXPECT_EQ(image.pixels[5], std::byte{0x80});
    EXPECT_EQ(image.pixels[6], std::byte{0x80});
    EXPECT_EQ(image.pixels[7], std::byte{0xFF});
    EXPECT_EQ(image.pixels[8], std::byte{0xC0});
    EXPECT_EQ(image.pixels[9], std::byte{0xC0});
    EXPECT_EQ(image.pixels[10], std::byte{0xC0});
    EXPECT_EQ(image.pixels[11], std::byte{0xFF});
    EXPECT_EQ(image.pixels[12], std::byte{0xFF});
    EXPECT_EQ(image.pixels[13], std::byte{0xFF});
    EXPECT_EQ(image.pixels[14], std::byte{0xFF});
    EXPECT_EQ(image.pixels[15], std::byte{0xFF});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DecodesAdam7RgbTwoByTwo) {
    const std::vector<std::byte> png = MakePngAdam7RgbTwoByTwoUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.width, 2U);
    ASSERT_EQ(image.height, 2U);
    ASSERT_EQ(image.pixels.size(), 16U);
    EXPECT_EQ(image.pixels[0], std::byte{0xFF});
    EXPECT_EQ(image.pixels[1], std::byte{0});
    EXPECT_EQ(image.pixels[2], std::byte{0});
    EXPECT_EQ(image.pixels[3], std::byte{0xFF});
    EXPECT_EQ(image.pixels[4], std::byte{0});
    EXPECT_EQ(image.pixels[5], std::byte{0xFF});
    EXPECT_EQ(image.pixels[6], std::byte{0});
    EXPECT_EQ(image.pixels[7], std::byte{0xFF});
    EXPECT_EQ(image.pixels[8], std::byte{0});
    EXPECT_EQ(image.pixels[9], std::byte{0});
    EXPECT_EQ(image.pixels[10], std::byte{0xFF});
    EXPECT_EQ(image.pixels[11], std::byte{0xFF});
    EXPECT_EQ(image.pixels[12], std::byte{0xFF});
    EXPECT_EQ(image.pixels[13], std::byte{0xFF});
    EXPECT_EQ(image.pixels[14], std::byte{0xFF});
    EXPECT_EQ(image.pixels[15], std::byte{0xFF});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DecodesAdam7PackedGray1BitTwoByTwo) {
    const std::vector<std::byte> png = MakePngAdam7PackedGray1BitTwoByTwoUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.width, 2U);
    ASSERT_EQ(image.height, 2U);
    const std::array<std::byte, 4> expected{std::byte{0}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0}};
    for (std::size_t pixel = 0U; pixel < expected.size(); ++pixel) {
        EXPECT_EQ(image.pixels[pixel * 4U], expected[pixel]);
        EXPECT_EQ(image.pixels[pixel * 4U + 1U], expected[pixel]);
        EXPECT_EQ(image.pixels[pixel * 4U + 2U], expected[pixel]);
        EXPECT_EQ(image.pixels[pixel * 4U + 3U], std::byte{0xFF});
    }
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DecodesAdam7PackedGray2BitTwoByTwo) {
    const std::vector<std::byte> png = MakePngAdam7PackedGray2BitTwoByTwoUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    const std::array<std::byte, 4> expected{std::byte{0}, std::byte{0xFF}, std::byte{0x55}, std::byte{0xAA}};
    for (std::size_t pixel = 0U; pixel < expected.size(); ++pixel) {
        EXPECT_EQ(image.pixels[pixel * 4U], expected[pixel]);
        EXPECT_EQ(image.pixels[pixel * 4U + 1U], expected[pixel]);
        EXPECT_EQ(image.pixels[pixel * 4U + 2U], expected[pixel]);
        EXPECT_EQ(image.pixels[pixel * 4U + 3U], std::byte{0xFF});
    }
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DecodesAdam7PackedGray4BitTwoByTwo) {
    const std::vector<std::byte> png = MakePngAdam7PackedGray4BitTwoByTwoUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    const std::array<std::byte, 4> expected{std::byte{0x11}, std::byte{0xEE}, std::byte{0x22}, std::byte{0xCC}};
    for (std::size_t pixel = 0U; pixel < expected.size(); ++pixel) {
        EXPECT_EQ(image.pixels[pixel * 4U], expected[pixel]);
        EXPECT_EQ(image.pixels[pixel * 4U + 1U], expected[pixel]);
        EXPECT_EQ(image.pixels[pixel * 4U + 2U], expected[pixel]);
        EXPECT_EQ(image.pixels[pixel * 4U + 3U], std::byte{0xFF});
    }
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DecodesAdam7PackedIndexed1BitTwoByTwo) {
    const std::vector<std::byte> png = MakePngAdam7PackedIndexed1BitTwoByTwoUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    const std::array<std::array<std::byte, 4>, 4> expected{{
        {std::byte{0xFF}, std::byte{0}, std::byte{0}, std::byte{0xFF}},
        {std::byte{0}, std::byte{0xFF}, std::byte{0}, std::byte{0x80}},
        {std::byte{0}, std::byte{0xFF}, std::byte{0}, std::byte{0x80}},
        {std::byte{0xFF}, std::byte{0}, std::byte{0}, std::byte{0xFF}},
    }};
    for (std::size_t pixel = 0U; pixel < expected.size(); ++pixel) {
        for (std::size_t channel = 0U; channel < 4U; ++channel) {
            EXPECT_EQ(image.pixels[pixel * 4U + channel], expected[pixel][channel]);
        }
    }
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DecodesAdam7PackedIndexed2BitTwoByTwo) {
    const std::vector<std::byte> png = MakePngAdam7PackedIndexed2BitTwoByTwoUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    const std::array<std::array<std::byte, 4>, 4> expected{{
        {std::byte{0xFF}, std::byte{0}, std::byte{0}, std::byte{0xFF}},
        {std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}},
        {std::byte{0}, std::byte{0xFF}, std::byte{0}, std::byte{0xFF}},
        {std::byte{0}, std::byte{0}, std::byte{0xFF}, std::byte{0xFF}},
    }};
    for (std::size_t pixel = 0U; pixel < expected.size(); ++pixel) {
        for (std::size_t channel = 0U; channel < 4U; ++channel) {
            EXPECT_EQ(image.pixels[pixel * 4U + channel], expected[pixel][channel]);
        }
    }
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DecodesAdam7PackedIndexed4BitTwoByTwo) {
    const std::vector<std::byte> png = MakePngAdam7PackedIndexed4BitTwoByTwoUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    const std::array<std::array<std::byte, 4>, 4> expected{{
        {std::byte{0xFF}, std::byte{0}, std::byte{0}, std::byte{0xFF}},
        {std::byte{0}, std::byte{0xFF}, std::byte{0}, std::byte{0xFF}},
        {std::byte{0}, std::byte{0}, std::byte{0xFF}, std::byte{0xFF}},
        {std::byte{0xFF}, std::byte{0}, std::byte{0}, std::byte{0xFF}},
    }};
    for (std::size_t pixel = 0U; pixel < expected.size(); ++pixel) {
        for (std::size_t channel = 0U; channel < 4U; ++channel) {
            EXPECT_EQ(image.pixels[pixel * 4U + channel], expected[pixel][channel]);
        }
    }
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_RejectsAdam7PackedIndexedOutOfRangeAtomically) {
    const std::vector<std::byte> png = MakePngOneByOneUVE(3U,
        {std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0x80}, std::byte{0}, std::byte{0x80}},
        {std::byte{0xFF}, std::byte{0}, std::byte{0}}, {}, 1U, 2U, 2U, 1U);
    PngRgba8ImageUVE image{7U, 8U, {std::byte{0xAA}}};
    EXPECT_FALSE(DecodePngRgba8ImageUVE(png, image));
    EXPECT_EQ(image.width, 7U);
    EXPECT_EQ(image.height, 8U);
    EXPECT_EQ(image.pixels, std::vector<std::byte>({std::byte{0xAA}}));
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DecodesAdam7RgbaTwoByTwo) {
    const std::vector<std::byte> png = MakePngAdam7RgbaTwoByTwoUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.width, 2U);
    ASSERT_EQ(image.height, 2U);
    ASSERT_EQ(image.pixels.size(), 16U);
    EXPECT_EQ(image.pixels[0], std::byte{0xFF});
    EXPECT_EQ(image.pixels[1], std::byte{0});
    EXPECT_EQ(image.pixels[2], std::byte{0});
    EXPECT_EQ(image.pixels[3], std::byte{0xFF});
    EXPECT_EQ(image.pixels[4], std::byte{0});
    EXPECT_EQ(image.pixels[5], std::byte{0xFF});
    EXPECT_EQ(image.pixels[6], std::byte{0});
    EXPECT_EQ(image.pixels[7], std::byte{0xFF});
    EXPECT_EQ(image.pixels[8], std::byte{0});
    EXPECT_EQ(image.pixels[9], std::byte{0});
    EXPECT_EQ(image.pixels[10], std::byte{0xFF});
    EXPECT_EQ(image.pixels[11], std::byte{0xFF});
    EXPECT_EQ(image.pixels[12], std::byte{0xFF});
    EXPECT_EQ(image.pixels[13], std::byte{0xFF});
    EXPECT_EQ(image.pixels[14], std::byte{0xFF});
    EXPECT_EQ(image.pixels[15], std::byte{0xFF});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DecodesKnownOneByOnePixel) {
    const std::vector<std::byte> png{
        std::byte{0x89}, std::byte{0x50}, std::byte{0x4E}, std::byte{0x47}, std::byte{0x0D}, std::byte{0x0A},
        std::byte{0x1A}, std::byte{0x0A}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x0D},
        std::byte{'I'}, std::byte{'H'}, std::byte{'D'}, std::byte{'R'}, std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x01}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x01},
        std::byte{0x08}, std::byte{0x06}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x1F},
        std::byte{0x15}, std::byte{0xC4}, std::byte{0x89}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
        std::byte{0x0D}, std::byte{'I'}, std::byte{'D'}, std::byte{'A'}, std::byte{'T'}, std::byte{0x78},
        std::byte{0x9C}, std::byte{0x63}, std::byte{0x60}, std::byte{0xF8}, std::byte{0xCF}, std::byte{0xF0},
        std::byte{0x1F}, std::byte{0x00}, std::byte{0x04}, std::byte{0x01}, std::byte{0x01}, std::byte{0xFF},
        std::byte{0x71}, std::byte{0xEB}, std::byte{0x47}, std::byte{0xE5}, std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{'I'}, std::byte{'E'}, std::byte{'N'}, std::byte{'D'}, std::byte{0xAE}, std::byte{0x42},
        std::byte{0x60}, std::byte{0x82}};
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    EXPECT_EQ(image.width, 1U);
    EXPECT_EQ(image.height, 1U);
    ASSERT_EQ(image.pixels.size(), 4U);
    EXPECT_EQ(image.pixels[0], std::byte{0x00});
    EXPECT_EQ(image.pixels[1], std::byte{0xFF});
    EXPECT_EQ(image.pixels[2], std::byte{0x00});
    EXPECT_EQ(image.pixels[3], std::byte{0xFF});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_AppliesTransparentGray8Key) {
    const std::vector<std::byte> png = MakePngTransparentGray8OneByOneUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.pixels.size(), 4U);
    EXPECT_EQ(image.pixels[0], std::byte{0x80});
    EXPECT_EQ(image.pixels[1], std::byte{0x80});
    EXPECT_EQ(image.pixels[2], std::byte{0x80});
    EXPECT_EQ(image.pixels[3], std::byte{0});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_AppliesTransparentRgb8Key) {
    const std::vector<std::byte> png = MakePngTransparentRgb8OneByOneUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.pixels.size(), 4U);
    EXPECT_EQ(image.pixels[0], std::byte{0xFF});
    EXPECT_EQ(image.pixels[1], std::byte{0});
    EXPECT_EQ(image.pixels[2], std::byte{0});
    EXPECT_EQ(image.pixels[3], std::byte{0});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_AppliesTransparentGray16Key) {
    const std::vector<std::byte> png = MakePngTransparentGray16OneByOneUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.pixels.size(), 4U);
    EXPECT_EQ(image.pixels[0], std::byte{0x34});
    EXPECT_EQ(image.pixels[1], std::byte{0x34});
    EXPECT_EQ(image.pixels[2], std::byte{0x34});
    EXPECT_EQ(image.pixels[3], std::byte{0});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_AppliesAdam7TransparentGrayKey) {
    const std::vector<std::byte> png = MakePngAdam7TransparentGrayTwoByTwoUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.pixels.size(), 16U);
    EXPECT_EQ(image.pixels[3], std::byte{0xFF});
    EXPECT_EQ(image.pixels[7], std::byte{0});
    EXPECT_EQ(image.pixels[11], std::byte{0xFF});
    EXPECT_EQ(image.pixels[15], std::byte{0xFF});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_RejectsMalformedTrnsKeyAtomically) {
    const std::vector<std::byte> png = MakePngOneByOneUVE(0U, {std::byte{0}, std::byte{0x80}}, {}, {}, 8U, 1U, 1U, 0U,
                                                               {std::byte{0x80}});
    PngRgba8ImageUVE image{7U, 8U, {std::byte{0xAA}}};
    EXPECT_FALSE(DecodePngRgba8ImageUVE(png, image));
    EXPECT_EQ(image.width, 7U);
    EXPECT_EQ(image.height, 8U);
    EXPECT_EQ(image.pixels, std::vector<std::byte>({std::byte{0xAA}}));
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_ExpandsRgbToRgba) {
    const std::vector<std::byte> png = MakePngRgbOneByOneUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    EXPECT_EQ(image.width, 1U);
    EXPECT_EQ(image.height, 1U);
    ASSERT_EQ(image.pixels.size(), 4U);
    EXPECT_EQ(image.pixels[0], std::byte{0xFF});
    EXPECT_EQ(image.pixels[1], std::byte{0x00});
    EXPECT_EQ(image.pixels[2], std::byte{0x00});
    EXPECT_EQ(image.pixels[3], std::byte{0xFF});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DecodesPackedGray1BitToRgba) {
    const std::vector<std::byte> png = MakePngGray1BitEightByOneUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.width, 8U);
    ASSERT_EQ(image.height, 1U);
    ASSERT_EQ(image.pixels.size(), 32U);
    const std::array<std::byte, 8> expected{std::byte{0x00}, std::byte{0xFF}, std::byte{0x00}, std::byte{0xFF},
                                             std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}};
    for (std::size_t pixel = 0U; pixel < expected.size(); ++pixel) {
        EXPECT_EQ(image.pixels[pixel * 4U], expected[pixel]);
        EXPECT_EQ(image.pixels[pixel * 4U + 1U], expected[pixel]);
        EXPECT_EQ(image.pixels[pixel * 4U + 2U], expected[pixel]);
        EXPECT_EQ(image.pixels[pixel * 4U + 3U], std::byte{0xFF});
    }
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DecodesPackedGray2BitToRgba) {
    const std::vector<std::byte> png = MakePngGray2BitFourByOneUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.width, 4U);
    ASSERT_EQ(image.height, 1U);
    ASSERT_EQ(image.pixels.size(), 16U);
    const std::array<std::byte, 4> expected{std::byte{0x00}, std::byte{0x55}, std::byte{0xAA}, std::byte{0xFF}};
    for (std::size_t pixel = 0U; pixel < expected.size(); ++pixel) {
        EXPECT_EQ(image.pixels[pixel * 4U], expected[pixel]);
        EXPECT_EQ(image.pixels[pixel * 4U + 1U], expected[pixel]);
        EXPECT_EQ(image.pixels[pixel * 4U + 2U], expected[pixel]);
        EXPECT_EQ(image.pixels[pixel * 4U + 3U], std::byte{0xFF});
    }
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DecodesPackedIndexed1BitToRgba) {
    const std::vector<std::byte> png = MakePngIndexed1BitEightByOneUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.width, 8U);
    ASSERT_EQ(image.height, 1U);
    ASSERT_EQ(image.pixels.size(), 32U);
    const std::array<std::byte, 8> expectedAlpha{std::byte{0xFF}, std::byte{0x80}, std::byte{0xFF}, std::byte{0x80},
                                                  std::byte{0x80}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0x80}};
    for (std::size_t pixel = 0U; pixel < expectedAlpha.size(); ++pixel) {
        const std::byte expectedRed = (pixel == 0U || pixel == 2U || pixel == 5U || pixel == 6U) ? std::byte{0xFF} : std::byte{0};
        const std::byte expectedGreen = expectedRed == std::byte{0xFF} ? std::byte{0} : std::byte{0xFF};
        EXPECT_EQ(image.pixels[pixel * 4U], expectedRed);
        EXPECT_EQ(image.pixels[pixel * 4U + 1U], expectedGreen);
        EXPECT_EQ(image.pixels[pixel * 4U + 2U], std::byte{0});
        EXPECT_EQ(image.pixels[pixel * 4U + 3U], expectedAlpha[pixel]);
    }
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DecodesPackedIndexed2BitToRgba) {
    const std::vector<std::byte> png = MakePngIndexed2BitFourByOneUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.width, 4U);
    ASSERT_EQ(image.height, 1U);
    ASSERT_EQ(image.pixels.size(), 16U);
    const std::array<std::array<std::byte, 4>, 4> expected{{
        {std::byte{0xFF}, std::byte{0}, std::byte{0}, std::byte{0xFF}},
        {std::byte{0}, std::byte{0xFF}, std::byte{0}, std::byte{0xFF}},
        {std::byte{0}, std::byte{0}, std::byte{0xFF}, std::byte{0xFF}},
        {std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}},
    }};
    for (std::size_t pixel = 0U; pixel < expected.size(); ++pixel) {
        for (std::size_t channel = 0U; channel < 4U; ++channel) {
            EXPECT_EQ(image.pixels[pixel * 4U + channel], expected[pixel][channel]);
        }
    }
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DecodesPackedIndexed4BitToRgba) {
    const std::vector<std::byte> png = MakePngIndexed4BitTwoByOneUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.width, 2U);
    ASSERT_EQ(image.height, 1U);
    ASSERT_EQ(image.pixels.size(), 8U);
    EXPECT_EQ(image.pixels[0], std::byte{0xFF});
    EXPECT_EQ(image.pixels[1], std::byte{0});
    EXPECT_EQ(image.pixels[2], std::byte{0});
    EXPECT_EQ(image.pixels[3], std::byte{0xFF});
    EXPECT_EQ(image.pixels[4], std::byte{0});
    EXPECT_EQ(image.pixels[5], std::byte{0xFF});
    EXPECT_EQ(image.pixels[6], std::byte{0});
    EXPECT_EQ(image.pixels[7], std::byte{0xFF});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_RejectsOutOfRangePackedIndexedPixelAtomically) {
    const std::vector<std::byte> png = MakePngOneByOneUVE(3U, {std::byte{0}, std::byte{0x80}},
                                                           {std::byte{0xFF}, std::byte{0}, std::byte{0}}, {}, 1U, 8U, 1U);
    PngRgba8ImageUVE image{7U, 8U, {std::byte{0xAA}}};
    EXPECT_FALSE(DecodePngRgba8ImageUVE(png, image));
    EXPECT_EQ(image.width, 7U);
    EXPECT_EQ(image.height, 8U);
    EXPECT_EQ(image.pixels, std::vector<std::byte>({std::byte{0xAA}}));
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DecodesPackedGray4BitToRgba) {
    const std::vector<std::byte> png = MakePngGray4BitTwoByOneUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.width, 2U);
    ASSERT_EQ(image.height, 1U);
    ASSERT_EQ(image.pixels.size(), 8U);
    EXPECT_EQ(image.pixels[0], std::byte{0x11});
    EXPECT_EQ(image.pixels[1], std::byte{0x11});
    EXPECT_EQ(image.pixels[2], std::byte{0x11});
    EXPECT_EQ(image.pixels[3], std::byte{0xFF});
    EXPECT_EQ(image.pixels[4], std::byte{0xEE});
    EXPECT_EQ(image.pixels[5], std::byte{0xEE});
    EXPECT_EQ(image.pixels[6], std::byte{0xEE});
    EXPECT_EQ(image.pixels[7], std::byte{0xFF});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_RejectsTruncatedPackedGrayRowAtomically) {
    const std::vector<std::byte> png = MakePngOneByOneUVE(0U, {std::byte{0}}, {}, {}, 4U, 2U, 1U);
    PngRgba8ImageUVE image{7U, 8U, {std::byte{0xAA}}};
    EXPECT_FALSE(DecodePngRgba8ImageUVE(png, image));
    EXPECT_EQ(image.width, 7U);
    EXPECT_EQ(image.height, 8U);
    EXPECT_EQ(image.pixels, std::vector<std::byte>({std::byte{0xAA}}));
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_ExpandsGrayscaleAlphaToRgba) {
    const std::vector<std::byte> png = MakePngGrayAlphaOneByOneUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.pixels.size(), 4U);
    EXPECT_EQ(image.pixels[0], std::byte{0x80});
    EXPECT_EQ(image.pixels[1], std::byte{0x80});
    EXPECT_EQ(image.pixels[2], std::byte{0x80});
    EXPECT_EQ(image.pixels[3], std::byte{0xC0});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_ExpandsGrayscaleToRgba) {
    const std::vector<std::byte> png = MakePngGrayOneByOneUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.pixels.size(), 4U);
    EXPECT_EQ(image.pixels[0], std::byte{0x80});
    EXPECT_EQ(image.pixels[1], std::byte{0x80});
    EXPECT_EQ(image.pixels[2], std::byte{0x80});
    EXPECT_EQ(image.pixels[3], std::byte{0xFF});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DownconvertsRgb16ToRgba8) {
    const std::vector<std::byte> png = MakePngRgb16OneByOneUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.pixels.size(), 4U);
    EXPECT_EQ(image.pixels[0], std::byte{0x12});
    EXPECT_EQ(image.pixels[1], std::byte{0xAB});
    EXPECT_EQ(image.pixels[2], std::byte{0xEF});
    EXPECT_EQ(image.pixels[3], std::byte{0xFF});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DownconvertsGray16ToRgba8) {
    const std::vector<std::byte> png = MakePngGray16OneByOneUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.pixels.size(), 4U);
    EXPECT_EQ(image.pixels[0], std::byte{0x34});
    EXPECT_EQ(image.pixels[1], std::byte{0x34});
    EXPECT_EQ(image.pixels[2], std::byte{0x34});
    EXPECT_EQ(image.pixels[3], std::byte{0xFF});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DownconvertsGrayAlpha16ToRgba8) {
    const std::vector<std::byte> png = MakePngGrayAlpha16OneByOneUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.pixels.size(), 4U);
    EXPECT_EQ(image.pixels[0], std::byte{0x34});
    EXPECT_EQ(image.pixels[1], std::byte{0x34});
    EXPECT_EQ(image.pixels[2], std::byte{0x34});
    EXPECT_EQ(image.pixels[3], std::byte{0xAB});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_DownconvertsRgba16ToRgba8) {
    const std::vector<std::byte> png = MakePngRgba16OneByOneUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.pixels.size(), 4U);
    EXPECT_EQ(image.pixels[0], std::byte{0x12});
    EXPECT_EQ(image.pixels[1], std::byte{0xAB});
    EXPECT_EQ(image.pixels[2], std::byte{0x56});
    EXPECT_EQ(image.pixels[3], std::byte{0x9A});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_ExpandsIndexedToRgba) {
    const std::vector<std::byte> png = MakePngIndexedOneByOneUVE();
    ASSERT_FALSE(png.empty());
    PngRgba8ImageUVE image;
    ASSERT_TRUE(DecodePngRgba8ImageUVE(png, image));
    ASSERT_EQ(image.pixels.size(), 4U);
    EXPECT_EQ(image.pixels[0], std::byte{0x00});
    EXPECT_EQ(image.pixels[1], std::byte{0xFF});
    EXPECT_EQ(image.pixels[2], std::byte{0x00});
    EXPECT_EQ(image.pixels[3], std::byte{0x80});
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_RejectsOutOfRangeIndexedPixelAtomically) {
    PngRgba8ImageUVE image{7U, 8U, {std::byte{0xAA}}};
    EXPECT_FALSE(DecodePngRgba8ImageUVE(MakePngIndexedOneByOneUVE(std::byte{2}), image));
    EXPECT_EQ(image.width, 7U);
    EXPECT_EQ(image.height, 8U);
    EXPECT_EQ(image.pixels, std::vector<std::byte>({std::byte{0xAA}}));
}

TEST(PngMetadataUVETest, DecodePngRgba8ImageUVE_RejectsCorruptOrUnsupportedInputAtomically) {
    const std::vector<std::byte> png{
        std::byte{0x89}, std::byte{0x50}, std::byte{0x4E}, std::byte{0x47}, std::byte{0x0D}, std::byte{0x0A},
        std::byte{0x1A}, std::byte{0x0A}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x0D},
        std::byte{'I'}, std::byte{'H'}, std::byte{'D'}, std::byte{'R'}, std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x01}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x01},
        std::byte{0x08}, std::byte{0x02}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x1F},
        std::byte{0x15}, std::byte{0xC4}, std::byte{0x89}};
    PngRgba8ImageUVE image{7U, 8U, {std::byte{0xAA}}};
    EXPECT_FALSE(DecodePngRgba8ImageUVE(png, image));
    EXPECT_EQ(image.width, 7U);
    EXPECT_EQ(image.height, 8U);
    EXPECT_EQ(image.pixels, std::vector<std::byte>({std::byte{0xAA}}));
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
