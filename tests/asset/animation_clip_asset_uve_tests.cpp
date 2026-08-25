// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/animation_clip_asset_uve.h"

#include <filesystem>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/uve_file_envelope_uve.h"

namespace UVE::Asset::Tests {
namespace {

std::filesystem::path TestPathUVE(const char* const name) {
    return std::filesystem::temp_directory_path() / name;
}

AnimationClipAssetUVE MakeValidClipUVE() {
    AnimationClipAssetUVE clip;
    clip.clipId = "walk";
    clip.durationSeconds = 1.0;
    clip.samples = {
        AnimationAssetSampleUVE{0.0, AnimationAssetPoseUVE{Math::Vector3UVE{0.0F, 0.0F, 0.0F},
                                                            Math::QuaternionUVE{},
                                                            Math::Vector3UVE{1.0F, 1.0F, 1.0F}}},
        AnimationAssetSampleUVE{1.0, AnimationAssetPoseUVE{Math::Vector3UVE{1.0F, 0.0F, 0.0F},
                                                            Math::QuaternionUVE{},
                                                            Math::Vector3UVE{1.0F, 1.0F, 1.0F}}},
    };
    clip.events = {AnimationAssetEventUVE{0.5, "footstep"}};
    return clip;
}

} // namespace

TEST(AnimationClipAssetUVETest, SaveThenLoad_RoundTripsBoundedPoseSamplesAndEvents) {
    const std::filesystem::path path = TestPathUVE("uve_animation_clip_asset_round_trip.uveanim");
    std::filesystem::remove(path);
    const AnimationClipAssetUVE original = MakeValidClipUVE();
    ASSERT_TRUE(SaveAnimationClipAssetUVE(original, path));

    AnimationClipAssetUVE loaded;
    ASSERT_TRUE(LoadAnimationClipAssetUVE(path, loaded));
    EXPECT_EQ(loaded.clipId, original.clipId);
    EXPECT_EQ(loaded.durationSeconds, original.durationSeconds);
    ASSERT_EQ(loaded.samples.size(), original.samples.size());
    EXPECT_EQ(loaded.samples[0], original.samples[0]);
    EXPECT_EQ(loaded.samples[1], original.samples[1]);
    ASSERT_EQ(loaded.events.size(), 1U);
    EXPECT_EQ(loaded.events.front(), original.events.front());
    std::filesystem::remove(path);
}

TEST(AnimationClipAssetUVETest, SaveRejectsInvalidClipWithoutPublishingDestination) {
    const std::filesystem::path path = TestPathUVE("uve_animation_clip_asset_invalid.uveanim");
    std::filesystem::remove(path);
    AnimationClipAssetUVE invalid = MakeValidClipUVE();
    invalid.samples[1].timeSeconds = 1.25;
    EXPECT_FALSE(SaveAnimationClipAssetUVE(invalid, path));
    EXPECT_FALSE(std::filesystem::exists(path));
}

TEST(AnimationClipAssetUVETest, LoadWrongEnvelopeKindPreservesExistingOutput) {
    const std::filesystem::path path = TestPathUVE("uve_animation_clip_asset_wrong_kind.uveanim");
    std::filesystem::remove(path);
    const std::vector<std::byte> payload{std::byte{'{'}, std::byte{'}'}};
    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Mesh, payload));
    AnimationClipAssetUVE retained = MakeValidClipUVE();
    const AnimationClipAssetUVE original = retained;
    EXPECT_FALSE(LoadAnimationClipAssetUVE(path, retained));
    EXPECT_EQ(retained.clipId, original.clipId);
    EXPECT_EQ(retained.samples, original.samples);
    EXPECT_EQ(retained.events, original.events);
    std::filesystem::remove(path);
}

} // namespace UVE::Asset::Tests
