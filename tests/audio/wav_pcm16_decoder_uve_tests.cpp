// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/audio/wav_pcm16_decoder_uve.h"
#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
namespace UVE::Audio::Tests {
namespace {
void AppendU16(std::vector<std::byte>& bytes, const std::uint16_t value) {
    bytes.push_back(static_cast<std::byte>(value & 0xFFU));
    bytes.push_back(static_cast<std::byte>((value >> 8U) & 0xFFU));
}
void AppendU32(std::vector<std::byte>& bytes, const std::uint32_t value) {
    for (unsigned shift = 0U; shift < 32U; shift += 8U) bytes.push_back(static_cast<std::byte>((value >> shift) & 0xFFU));
}
std::vector<std::byte> BuildWav16(const std::vector<std::int16_t>& samples, const std::uint16_t bits = 16U) {
    std::vector<std::byte> bytes;
    const std::uint32_t dataSize = static_cast<std::uint32_t>(samples.size() * 2U);
    bytes.insert(bytes.end(), {std::byte{'R'}, std::byte{'I'}, std::byte{'F'}, std::byte{'F'}});
    AppendU32(bytes, 36U + dataSize);
    bytes.insert(bytes.end(), {std::byte{'W'}, std::byte{'A'}, std::byte{'V'}, std::byte{'E'}});
    bytes.insert(bytes.end(), {std::byte{'f'}, std::byte{'m'}, std::byte{'t'}, std::byte{' '}});
    AppendU32(bytes, 16U); AppendU16(bytes, 1U); AppendU16(bytes, 1U); AppendU32(bytes, 48000U);
    AppendU32(bytes, 48000U * 2U); AppendU16(bytes, 2U); AppendU16(bytes, bits);
    bytes.insert(bytes.end(), {std::byte{'d'}, std::byte{'a'}, std::byte{'t'}, std::byte{'a'}});
    AppendU32(bytes, dataSize);
    for (const std::int16_t sample : samples) AppendU16(bytes, static_cast<std::uint16_t>(sample));
    return bytes;
}
TEST(WavPcm16DecoderUVETest, ValidateWavPcm16SampleWindowUVE_AcceptsBoundedChunks) {
    EXPECT_TRUE(ValidateWavPcm16SampleWindowUVE(100U, 0U, 32U));
    EXPECT_TRUE(ValidateWavPcm16SampleWindowUVE(100U, 40U, 60U));
    EXPECT_TRUE(ValidateWavPcm16SampleWindowUVE(100U, 40U, 8U, 8U));
}

TEST(WavPcm16DecoderUVETest, ValidateWavPcm16SampleWindowUVE_RejectsInvalidWindows) {
    EXPECT_FALSE(ValidateWavPcm16SampleWindowUVE(100U, 0U, 0U));
    EXPECT_FALSE(ValidateWavPcm16SampleWindowUVE(100U, 101U, 1U));
    EXPECT_FALSE(ValidateWavPcm16SampleWindowUVE(100U, 90U, 11U));
    EXPECT_FALSE(ValidateWavPcm16SampleWindowUVE(100U, 0U, kMaximumWavPcm16SamplesUVE + 1U));
    EXPECT_FALSE(ValidateWavPcm16SampleWindowUVE(100U, 0U, 1U, 0U));
}

TEST(WavPcm16DecoderUVETest, DecodeWavPcm16SampleWindowUVE_DecodesBoundedSlices) {
    const auto wav = BuildWav16({-32768, 0, 16384, 32767, -16384});
    std::vector<float> output;
    ASSERT_TRUE(DecodeWavPcm16SampleWindowUVE(wav, 1U, 3U, output));
    ASSERT_EQ(output.size(), 3U);
    EXPECT_FLOAT_EQ(output[0], 0.0F);
    EXPECT_FLOAT_EQ(output[1], 0.5F);
    EXPECT_NEAR(output[2], 0.9999695F, 1.0e-6F);
    ASSERT_TRUE(DecodeWavPcm16SampleWindowUVE(wav, 4U, 1U, output, 1U));
    ASSERT_EQ(output.size(), 1U);
    EXPECT_FLOAT_EQ(output[0], -0.5F);
}

TEST(WavPcm16DecoderUVETest, DecodeWavPcm16SampleWindowUVE_RejectsInvalidAndMalformedInputsAtomically) {
    const std::vector<float> original{0.25F};
    std::vector<float> output = original;
    const auto wav = BuildWav16({1, 2, 3});
    EXPECT_FALSE(DecodeWavPcm16SampleWindowUVE(wav, 3U, 1U, output));
    EXPECT_EQ(output, original);
    EXPECT_FALSE(DecodeWavPcm16SampleWindowUVE(wav, 0U, 2U, output, 1U));
    EXPECT_EQ(output, original);
    auto truncated = wav;
    truncated.pop_back();
    EXPECT_FALSE(DecodeWavPcm16SampleWindowUVE(truncated, 0U, 1U, output));
    EXPECT_EQ(output, original);
    EXPECT_FALSE(DecodeWavPcm16SampleWindowUVE(BuildWav16({1, 2}, 8U), 0U, 1U, output));
    EXPECT_EQ(output, original);
}

TEST(WavPcm16DecoderUVETest, DecodesNormalizedSamples) {
    std::vector<float> output;
    ASSERT_TRUE(DecodeWavPcm16SamplesUVE(BuildWav16({-32768, 0, 16384, 32767}), output));
    ASSERT_EQ(output.size(), 4U);
    EXPECT_FLOAT_EQ(output[0], -1.0F);
    EXPECT_FLOAT_EQ(output[1], 0.0F);
    EXPECT_FLOAT_EQ(output[2], 0.5F);
    EXPECT_NEAR(output[3], 0.9999695F, 1.0e-6F);
}
TEST(WavPcm16DecoderUVETest, RejectsUnsupportedOrTruncatedDataAtomically) {
    std::vector<float> output{0.25F};
    EXPECT_FALSE(DecodeWavPcm16SamplesUVE(BuildWav16({1, 2}, 8U), output));
    EXPECT_EQ(output, std::vector<float>{0.25F});
    auto truncated = BuildWav16({1, 2});
    truncated.pop_back();
    EXPECT_FALSE(DecodeWavPcm16SamplesUVE(truncated, output));
    EXPECT_EQ(output, std::vector<float>{0.25F});
}
} // namespace
} // namespace UVE::Audio::Tests
