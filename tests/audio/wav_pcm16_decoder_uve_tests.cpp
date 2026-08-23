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
TEST(WavPcm16DecoderUVETest, Pcm16StreamCursorUVE_ConsumesNonLoopingWindowsAndClampsAtEnd) {
    Pcm16StreamCursorUVE cursor;
    ASSERT_TRUE(cursor.ResetUVE(10U, false));
    Pcm16StreamWindowPlanUVE plan;
    ASSERT_TRUE(cursor.ConsumeWindowUVE(4U, plan));
    EXPECT_EQ(plan, (Pcm16StreamWindowPlanUVE{0U, 4U, 4U, false, false}));
    EXPECT_EQ(cursor.GetCursorSampleUVE(), 4U);
    ASSERT_TRUE(cursor.ConsumeWindowUVE(16U, plan));
    EXPECT_EQ(plan, (Pcm16StreamWindowPlanUVE{4U, 6U, 10U, true, false}));
    EXPECT_EQ(cursor.GetCursorSampleUVE(), 10U);
    ASSERT_TRUE(cursor.ConsumeWindowUVE(1U, plan));
    EXPECT_EQ(plan, (Pcm16StreamWindowPlanUVE{10U, 0U, 10U, true, false}));
}

TEST(WavPcm16DecoderUVETest, Pcm16StreamCursorUVE_WrapsLoopingWindowsAndAdvancesPersistently) {
    Pcm16StreamCursorUVE cursor;
    ASSERT_TRUE(cursor.ResetUVE(10U, true, 8U));
    Pcm16StreamWindowPlanUVE plan;
    ASSERT_TRUE(cursor.ConsumeWindowUVE(4U, plan));
    EXPECT_EQ(plan, (Pcm16StreamWindowPlanUVE{8U, 2U, 0U, true, true}));
    ASSERT_TRUE(cursor.ConsumeWindowUVE(3U, plan));
    EXPECT_EQ(plan, (Pcm16StreamWindowPlanUVE{0U, 3U, 3U, false, false}));
    EXPECT_EQ(cursor.GetCursorSampleUVE(), 3U);
    bool reachedEnd = false;
    bool wrapped = false;
    ASSERT_TRUE(cursor.AdvanceUVE(7U, reachedEnd, wrapped));
    EXPECT_EQ(cursor.GetCursorSampleUVE(), 0U);
    EXPECT_TRUE(reachedEnd);
    EXPECT_TRUE(wrapped);
}

TEST(WavPcm16DecoderUVETest, Pcm16StreamCursorUVE_RejectsInvalidResetAndConsumptionAtomically) {
    Pcm16StreamCursorUVE cursor;
    ASSERT_TRUE(cursor.ResetUVE(10U, false, 3U));
    Pcm16StreamWindowPlanUVE original{7U, 2U, 9U, false, false};
    Pcm16StreamWindowPlanUVE plan = original;
    EXPECT_FALSE(cursor.ResetUVE(0U, false));
    EXPECT_EQ(cursor.GetTotalSamplesUVE(), 10U);
    EXPECT_EQ(cursor.GetCursorSampleUVE(), 3U);
    EXPECT_FALSE(cursor.ConsumeWindowUVE(0U, plan));
    EXPECT_EQ(plan, original);
    bool reachedEnd = true;
    bool wrapped = true;
    Pcm16StreamCursorUVE unconfigured;
    EXPECT_FALSE(unconfigured.AdvanceUVE(1U, reachedEnd, wrapped));
    EXPECT_TRUE(reachedEnd);
    EXPECT_TRUE(wrapped);
    EXPECT_EQ(cursor.GetCursorSampleUVE(), 3U);
}

TEST(WavPcm16DecoderUVETest, PlanPcm16StreamWindowUVE_PlansContiguousAndLoopingWindows) {
    Pcm16StreamWindowPlanUVE plan;
    ASSERT_TRUE(PlanPcm16StreamWindowUVE(100U, 20U, 16U, false, plan));
    EXPECT_EQ(plan, (Pcm16StreamWindowPlanUVE{20U, 16U, 36U, false, false}));
    ASSERT_TRUE(PlanPcm16StreamWindowUVE(100U, 96U, 16U, false, plan));
    EXPECT_EQ(plan, (Pcm16StreamWindowPlanUVE{96U, 4U, 100U, true, false}));
    ASSERT_TRUE(PlanPcm16StreamWindowUVE(100U, 96U, 16U, true, plan));
    EXPECT_EQ(plan, (Pcm16StreamWindowPlanUVE{96U, 4U, 0U, true, true}));
    ASSERT_TRUE(PlanPcm16StreamWindowUVE(100U, 100U, 8U, true, plan));
    EXPECT_EQ(plan, (Pcm16StreamWindowPlanUVE{0U, 8U, 8U, false, true}));
    ASSERT_TRUE(PlanPcm16StreamWindowUVE(100U, 100U, 8U, false, plan));
    EXPECT_EQ(plan, (Pcm16StreamWindowPlanUVE{100U, 0U, 100U, true, false}));
}

TEST(WavPcm16DecoderUVETest, PlanPcm16StreamWindowUVE_RejectsInvalidInputsAtomically) {
    const Pcm16StreamWindowPlanUVE original{4U, 3U, 7U, false, false};
    Pcm16StreamWindowPlanUVE plan = original;
    EXPECT_FALSE(PlanPcm16StreamWindowUVE(0U, 0U, 1U, false, plan));
    EXPECT_EQ(plan, original);
    EXPECT_FALSE(PlanPcm16StreamWindowUVE(10U, 11U, 1U, false, plan));
    EXPECT_EQ(plan, original);
    EXPECT_FALSE(PlanPcm16StreamWindowUVE(10U, 0U, 0U, false, plan));
    EXPECT_EQ(plan, original);
    EXPECT_FALSE(PlanPcm16StreamWindowUVE(10U, 0U, 9U, false, plan, 8U));
    EXPECT_EQ(plan, original);
    EXPECT_FALSE(PlanPcm16StreamWindowUVE(kMaximumWavPcm16SamplesUVE + 1U, 0U, 1U, false, plan));
    EXPECT_EQ(plan, original);
}

TEST(WavPcm16DecoderUVETest, AdvancePcm16StreamCursorUVE_ClampsNonLoopingCursorAtEnd) {
    std::size_t cursor = 99U;
    bool reachedEnd = false;
    bool wrapped = true;
    ASSERT_TRUE(AdvancePcm16StreamCursorUVE(100U, 20U, 16U, false, cursor, reachedEnd, wrapped));
    EXPECT_EQ(cursor, 36U);
    EXPECT_FALSE(reachedEnd);
    EXPECT_FALSE(wrapped);
    ASSERT_TRUE(AdvancePcm16StreamCursorUVE(100U, cursor, 100U, false, cursor, reachedEnd, wrapped));
    EXPECT_EQ(cursor, 100U);
    EXPECT_TRUE(reachedEnd);
    EXPECT_FALSE(wrapped);
    ASSERT_TRUE(AdvancePcm16StreamCursorUVE(100U, cursor, 0U, false, cursor, reachedEnd, wrapped));
    EXPECT_EQ(cursor, 100U);
    EXPECT_TRUE(reachedEnd);
}

TEST(WavPcm16DecoderUVETest, AdvancePcm16StreamCursorUVE_WrapsLoopingCursor) {
    std::size_t cursor = 0U;
    bool reachedEnd = false;
    bool wrapped = false;
    ASSERT_TRUE(AdvancePcm16StreamCursorUVE(100U, 96U, 16U, true, cursor, reachedEnd, wrapped));
    EXPECT_EQ(cursor, 12U);
    EXPECT_TRUE(reachedEnd);
    EXPECT_TRUE(wrapped);
    ASSERT_TRUE(AdvancePcm16StreamCursorUVE(100U, 100U, 0U, true, cursor, reachedEnd, wrapped));
    EXPECT_EQ(cursor, 0U);
    EXPECT_FALSE(reachedEnd);
    EXPECT_TRUE(wrapped);
}

TEST(WavPcm16DecoderUVETest, AdvancePcm16StreamCursorUVE_RejectsInvalidInputsAtomically) {
    std::size_t cursor = 7U;
    bool reachedEnd = true;
    bool wrapped = true;
    EXPECT_FALSE(AdvancePcm16StreamCursorUVE(0U, 0U, 1U, false, cursor, reachedEnd, wrapped));
    EXPECT_EQ(cursor, 7U);
    EXPECT_TRUE(reachedEnd);
    EXPECT_TRUE(wrapped);
    EXPECT_FALSE(AdvancePcm16StreamCursorUVE(10U, 11U, 1U, false, cursor, reachedEnd, wrapped));
    EXPECT_FALSE(AdvancePcm16StreamCursorUVE(10U, 0U, 1U, false, cursor, reachedEnd, wrapped, 0U));
    EXPECT_FALSE(AdvancePcm16StreamCursorUVE(11U, 0U, 1U, false, cursor, reachedEnd, wrapped, 10U));
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
