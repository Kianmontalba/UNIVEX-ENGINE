// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/plugins/motion_query_asset_sampling_uve.h"

#include <gtest/gtest.h>

#include <limits>

namespace UVE::Plugins {
namespace {

MotionQueryAssetSamplingRequestUVE MakeRequestUVE() {
    MotionQueryAssetSamplingRequestUVE request;
    request.key.source.guid.value = 42U;
    request.key.source.generation = 7U;
    request.samples = {
        UVE::Core::MotionQueryFeatureVectorUVE{{0.0F, 1.0F}, 2.0F},
        UVE::Core::MotionQueryFeatureVectorUVE{{2.0F, 3.0F}, 3.0F},
    };
    request.normalizationRanges = {
        MotionQueryNormalizationRangeUVE{0.0F, 2.0F},
        MotionQueryNormalizationRangeUVE{1.0F, 3.0F},
    };
    return request;
}

} // namespace

TEST(MotionQueryAssetSamplingUVETest, BuildDerivedDataUVE_NormalizesCopiedSamplesDeterministically) {
    const MotionQueryAssetSamplingRequestUVE request = MakeRequestUVE();
    MotionQueryDerivedDataUVE derived;

    const MotionQueryAssetSamplingResultUVE result =
        BuildMotionQueryDerivedDataUVE(request, derived);
    ASSERT_TRUE(result.IsAcceptedUVE()) << result.message;
    ASSERT_EQ(derived.normalizedSamples.size(), 2U);
    EXPECT_FLOAT_EQ(derived.normalizedSamples[0].values[0], 0.0F);
    EXPECT_FLOAT_EQ(derived.normalizedSamples[0].values[1], 0.0F);
    EXPECT_FLOAT_EQ(derived.normalizedSamples[1].values[0], 1.0F);
    EXPECT_FLOAT_EQ(derived.normalizedSamples[1].values[1], 1.0F);
    EXPECT_EQ(derived.key, request.key);
}

TEST(MotionQueryAssetSamplingUVETest, BuildDerivedDataUVE_PreservesFiniteExtremeRanges) {
    MotionQueryAssetSamplingRequestUVE request = MakeRequestUVE();
    const float maximumValue = std::numeric_limits<float>::max();
    request.samples[0].values = {-maximumValue, maximumValue};
    request.normalizationRanges[0] =
        MotionQueryNormalizationRangeUVE{-maximumValue, maximumValue};
    request.normalizationRanges[1] =
        MotionQueryNormalizationRangeUVE{-maximumValue, maximumValue};

    MotionQueryDerivedDataUVE derived;
    const MotionQueryAssetSamplingResultUVE result =
        BuildMotionQueryDerivedDataUVE(request, derived);
    ASSERT_TRUE(result.IsAcceptedUVE()) << result.message;
    ASSERT_EQ(derived.normalizedSamples.size(), 2U);
    EXPECT_FLOAT_EQ(derived.normalizedSamples[0].values[0], 0.0F);
    EXPECT_FLOAT_EQ(derived.normalizedSamples[0].values[1], 1.0F);
}

TEST(MotionQueryAssetSamplingUVETest, BuildDerivedDataUVE_ClampsOutOfRangeValues) {
    MotionQueryAssetSamplingRequestUVE request = MakeRequestUVE();
    request.samples[0].values = {-4.0F, 9.0F};
    MotionQueryDerivedDataUVE derived;

    ASSERT_TRUE(BuildMotionQueryDerivedDataUVE(request, derived).IsAcceptedUVE());
    EXPECT_FLOAT_EQ(derived.normalizedSamples[0].values[0], 0.0F);
    EXPECT_FLOAT_EQ(derived.normalizedSamples[0].values[1], 1.0F);
}

TEST(MotionQueryAssetSamplingUVETest, BuildDerivedDataUVE_RejectsInvalidHandleAndDimensions) {
    MotionQueryAssetSamplingRequestUVE request = MakeRequestUVE();
    request.key.source.generation = 0U;
    MotionQueryDerivedDataUVE derived;
    EXPECT_EQ(BuildMotionQueryDerivedDataUVE(request, derived).code,
              MotionQueryAssetSamplingCodeUVE::InvalidSourceHandle);

    request = MakeRequestUVE();
    request.samples[1].values.push_back(4.0F);
    EXPECT_EQ(BuildMotionQueryDerivedDataUVE(request, derived).code,
              MotionQueryAssetSamplingCodeUVE::InconsistentFeatureDimensions);
}

TEST(MotionQueryAssetSamplingUVETest, IsDerivedDataCurrentUVE_UsesExactResourceGeneration) {
    MotionQueryDerivedDataUVE derived;
    ASSERT_TRUE(BuildMotionQueryDerivedDataUVE(MakeRequestUVE(), derived).IsAcceptedUVE());

    Asset::ResourceDependencySnapshotUVE current;
    current.entries.push_back(Asset::ResourceDependencyEntryUVE{derived.key.source, {}});
    EXPECT_TRUE(IsMotionQueryDerivedDataCurrentUVE(derived, current));

    current.entries.front().handle.generation++;
    EXPECT_FALSE(IsMotionQueryDerivedDataCurrentUVE(derived, current));
}

} // namespace UVE::Plugins
