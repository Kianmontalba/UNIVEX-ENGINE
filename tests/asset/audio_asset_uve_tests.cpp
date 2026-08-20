// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/audio_asset_uve.h"

#include <filesystem>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/texture_asset_uve.h"

namespace UVE::Asset::Tests {
namespace {

TEST(AudioAssetUVETest, SaveThenLoad_RoundTripsValidatedInterleavedSamples) {
    const std::filesystem::path path = "uve_audio_asset_tests_roundtrip.uveaudio";
    std::filesystem::remove(path);
    const AudioAssetUVE original{2U, 48000U, {-1.0F, -0.5F, 0.0F, 0.5F, 0.75F, 1.0F}};

    ASSERT_TRUE(SaveAudioAssetUVE(original, path));
    AudioAssetUVE loaded;
    ASSERT_TRUE(LoadAudioAssetUVE(path, loaded));
    EXPECT_EQ(loaded.channels, original.channels);
    EXPECT_EQ(loaded.sampleRate, original.sampleRate);
    EXPECT_EQ(loaded.samples, original.samples);
    std::filesystem::remove(path);
}

TEST(AudioAssetUVETest, SaveAudioAssetUVE_RejectsInvalidMetadataAndSamples) {
    const std::filesystem::path path = "uve_audio_asset_tests_invalid.uveaudio";
    std::filesystem::remove(path);
    EXPECT_FALSE(SaveAudioAssetUVE(AudioAssetUVE{0U, 48000U, {0.0F}}, path));
    EXPECT_FALSE(SaveAudioAssetUVE(AudioAssetUVE{3U, 48000U, {0.0F, 0.5F}}, path));
    EXPECT_FALSE(SaveAudioAssetUVE(AudioAssetUVE{2U, 48000U, {0.0F, 0.5F, 1.5F, 0.0F}}, path));
    EXPECT_FALSE(SaveAudioAssetUVE(AudioAssetUVE{2U, 48000U, {0.0F, std::numeric_limits<float>::quiet_NaN()}}, path));
    EXPECT_FALSE(std::filesystem::exists(path));
}

TEST(AudioAssetUVETest, LoadAudioAssetUVE_RejectsWrongKindAndPreservesOutput) {
    const std::filesystem::path path = "uve_audio_asset_tests_wrong_kind.uvetex";
    std::filesystem::remove(path);
    AudioAssetUVE original{1U, 44100U, {0.25F}};
    TextureAssetUVE texture;
    texture.width = 1U;
    texture.height = 1U;
    texture.format = TextureFormatUVE::RGBA8Unorm;
    texture.pixels = {std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}};
    ASSERT_TRUE(SaveTextureAssetUVE(texture, path));

    EXPECT_FALSE(LoadAudioAssetUVE(path, original));
    EXPECT_EQ(original.channels, 1U);
    EXPECT_EQ(original.sampleRate, 44100U);
    EXPECT_EQ(original.samples, std::vector<float>({0.25F}));
    std::filesystem::remove(path);
}

} // namespace
} // namespace UVE::Asset::Tests
