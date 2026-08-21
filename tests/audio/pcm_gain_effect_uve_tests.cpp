// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/audio/pcm_gain_effect_uve.h"
#include <gtest/gtest.h>
#include <limits>
namespace UVE::Audio::Tests {
namespace {
TEST(PcmGainEffectUVETest, AppliesOrderedGainChainAndClampsEachStage) {
    const std::vector<float> input{-0.75F, 0.25F, 0.9F};
    std::vector<float> output;
    ASSERT_TRUE(ApplyPcmGainEffectChainUVE(input, {2.0F, 0.5F}, output));
    EXPECT_FLOAT_EQ(output[0], -0.5F);
    EXPECT_FLOAT_EQ(output[1], 0.25F);
    EXPECT_FLOAT_EQ(output[2], 0.5F);
}

TEST(PcmGainEffectUVETest, EmptyGainChainCopiesFiniteInput) {
    const std::vector<float> input{-0.5F, 0.25F};
    std::vector<float> output{0.75F};
    ASSERT_TRUE(ApplyPcmGainEffectChainUVE(input, {}, output));
    EXPECT_EQ(output, input);
}

TEST(PcmGainEffectUVETest, InvalidChainStageOrCountLeavesOutputUnchanged) {
    const std::vector<float> input{0.5F};
    std::vector<float> output{0.25F};
    EXPECT_FALSE(ApplyPcmGainEffectChainUVE(input, {-1.0F}, output));
    EXPECT_EQ(output, std::vector<float>{0.25F});
    EXPECT_FALSE(ApplyPcmGainEffectChainUVE(input, {std::numeric_limits<float>::quiet_NaN()}, output));
    EXPECT_EQ(output, std::vector<float>{0.25F});
    EXPECT_FALSE(ApplyPcmGainEffectChainUVE(input, std::vector<float>(kMaximumPcmGainChainEffectsUVE + 1U, 1.0F), output));
    EXPECT_EQ(output, std::vector<float>{0.25F});
}

TEST(PcmGainEffectUVETest, ApplyScheduledPcmGainEffectsUVE_AppliesOrderedWindows) {
    const std::vector<float> input{-0.5F, 0.25F, 0.75F, -0.25F};
    std::vector<float> output;
    ASSERT_TRUE(ApplyScheduledPcmGainEffectsUVE(
        input, {{1U, 2U, 2.0F}, {2U, 1U, 0.5F}}, output));
    EXPECT_EQ(output, (std::vector<float>{-0.5F, 0.5F, 0.5F, -0.25F}));
    ASSERT_TRUE(ApplyScheduledPcmGainEffectsUVE(
        input, {{0U, 3U, 3.0F}}, output));
    EXPECT_EQ(output, (std::vector<float>{-1.0F, 0.75F, 1.0F, -0.25F}));
}

TEST(PcmGainEffectUVETest, ApplyScheduledPcmGainEffectsUVE_RejectsInvalidWindowsAtomically) {
    const std::vector<float> input{0.5F, -0.25F};
    const std::vector<float> original{0.75F};
    std::vector<float> output = original;
    EXPECT_FALSE(ApplyScheduledPcmGainEffectsUVE(input, {{0U, 0U, 1.0F}}, output));
    EXPECT_EQ(output, original);
    EXPECT_FALSE(ApplyScheduledPcmGainEffectsUVE(input, {{1U, 2U, 1.0F}}, output));
    EXPECT_EQ(output, original);
    EXPECT_FALSE(ApplyScheduledPcmGainEffectsUVE(input, {{0U, 1U, -1.0F}}, output));
    EXPECT_EQ(output, original);
    EXPECT_FALSE(ApplyScheduledPcmGainEffectsUVE(
        input, std::vector<PcmGainEffectWindowUVE>(kMaximumPcmGainChainEffectsUVE + 1U, {0U, 1U, 1.0F}), output));
    EXPECT_EQ(output, original);
}

TEST(PcmGainEffectUVETest, AppliesGainAndClampsNormalizedSamples) {
    const std::vector<float> input{-0.75F, 0.25F, 0.9F};
    std::vector<float> output;
    ASSERT_TRUE(ApplyPcmGainEffectUVE(input, 2.0F, output));
    EXPECT_FLOAT_EQ(output[0], -1.0F);
    EXPECT_FLOAT_EQ(output[1], 0.5F);
    EXPECT_FLOAT_EQ(output[2], 1.0F);
}
TEST(PcmGainEffectUVETest, InvalidGainOrSampleLeavesOutputUnchanged) {
    const std::vector<float> input{0.5F};
    std::vector<float> output{0.25F};
    EXPECT_FALSE(ApplyPcmGainEffectUVE(input, -1.0F, output));
    EXPECT_EQ(output, std::vector<float>{0.25F});
    EXPECT_FALSE(ApplyPcmGainEffectUVE(input, std::numeric_limits<float>::quiet_NaN(), output));
    EXPECT_EQ(output, std::vector<float>{0.25F});
    const std::vector<float> nonFinite{0.5F, std::numeric_limits<float>::infinity()};
    EXPECT_FALSE(ApplyPcmGainEffectUVE(nonFinite, 1.0F, output));
    EXPECT_EQ(output, std::vector<float>{0.25F});
}
TEST(PcmGainEffectUVETest, SampleCountCapFailsClosed) {
    const std::vector<float> input(kMaximumPcmGainSamplesUVE + 1U, 0.0F);
    std::vector<float> output{0.5F};
    EXPECT_FALSE(ApplyPcmGainEffectUVE(input, 1.0F, output));
    EXPECT_EQ(output, std::vector<float>{0.5F});
}
} // namespace
} // namespace UVE::Audio::Tests
