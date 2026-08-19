// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/asset/animation_clip_metadata_uve.h"
#include <gtest/gtest.h>
namespace UVE::Asset::Tests {
TEST(AnimationClipMetadataUVETest, ValidateAnimationClipMetadataUVE_CopiesValidFacts) {
    const AnimationClipMetadataInputUVE input{2.5F, 60.0F, 128U, 42U};
    const auto metadata = ValidateAnimationClipMetadataUVE(input); ASSERT_TRUE(metadata.has_value());
    EXPECT_FLOAT_EQ(metadata->durationSeconds, 2.5F); EXPECT_FLOAT_EQ(metadata->sampleRateHz, 60.0F);
    EXPECT_EQ(metadata->trackCount, 128U); EXPECT_EQ(metadata->skeletonIdentity, 42U);
}
TEST(AnimationClipMetadataUVETest, ValidateAnimationClipMetadataUVE_AcceptsDocumentedUpperBounds) {
    const auto metadata = ValidateAnimationClipMetadataUVE(AnimationClipMetadataInputUVE{0.001F, 120.0F, 65'536U, 1U});
    EXPECT_TRUE(metadata.has_value());
}
TEST(AnimationClipMetadataUVETest, ValidateAnimationClipMetadataUVE_RejectsInvalidFacts) {
    EXPECT_FALSE(ValidateAnimationClipMetadataUVE(AnimationClipMetadataInputUVE{0.0F, 60.0F, 1U, 1U}).has_value());
    EXPECT_FALSE(ValidateAnimationClipMetadataUVE(AnimationClipMetadataInputUVE{1.0F, 121.0F, 1U, 1U}).has_value());
    EXPECT_FALSE(ValidateAnimationClipMetadataUVE(AnimationClipMetadataInputUVE{1.0F, 60.0F, 0U, 1U}).has_value());
    EXPECT_FALSE(ValidateAnimationClipMetadataUVE(AnimationClipMetadataInputUVE{1.0F, 60.0F, 1U, 0U}).has_value());
}
}
