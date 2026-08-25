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

TEST(MotionQueryAssetSamplingUVETest, BuildDatabaseFromClipsUVE_SamplesCandidatesAndFutureTrajectory) {
    UVE::Core::AnimationClipUVE clip;
    clip.clipId = "walk";
    clip.durationSeconds = 1.0;
    clip.samples = {
        UVE::Core::PoseSampleUVE{0.0, UVE::Core::TransformPoseUVE{{0.0F, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}}},
        UVE::Core::PoseSampleUVE{1.0, UVE::Core::TransformPoseUVE{{1.0F, 0.0F, 0.0F}, {}, {1.0F, 1.0F, 1.0F}}},
    };
    MotionQueryClipSamplingRequestUVE request;
    request.key.source.guid.value = 51U;
    request.key.source.generation = 2U;
    request.featureSchema.channels = {
        UVE::Core::MotionQueryFeatureChannelUVE{"root_velocity",
                                                UVE::Core::MotionQueryFeatureChannelKindUVE::RootVelocity,
                                                0U, 1.0F}};
    request.trajectoryOffsets = {0.0, 0.25};
    request.clips = {clip};
    request.samplePeriodSeconds = 0.5;
    request.looping = false;

    UVE::Core::MotionMatchingDatabaseUVE database;
    const MotionQueryClipSamplingResultUVE result =
        BuildMotionQueryDatabaseFromClipsUVE(request, database);
    ASSERT_TRUE(result.IsAcceptedUVE()) << result.message;
    ASSERT_EQ(database.candidates.size(), 3U);
    EXPECT_EQ(database.candidates[0].candidateId, "walk@0");
    EXPECT_EQ(database.candidates[1].candidateId, "walk@1");
    EXPECT_EQ(database.candidates[2].candidateId, "walk@2");
    EXPECT_FLOAT_EQ(database.candidates[1].feature.rootVelocity.x, 1.0F);
    ASSERT_EQ(database.candidates[1].feature.trajectory.size(), 2U);
    EXPECT_DOUBLE_EQ(database.candidates[1].feature.trajectory[0].offsetSeconds, 0.0);
    EXPECT_FLOAT_EQ(database.candidates[1].feature.trajectory[1].relativePosition.x, 0.25F);
    EXPECT_TRUE(UVE::Core::ValidateMotionMatchingDatabaseUVE(database).IsValidUVE());
}

TEST(MotionQueryAssetSamplingUVETest, BuildDatabaseFromClipsUVE_RejectsInvalidPeriodWithoutPublishing) {
    MotionQueryClipSamplingRequestUVE request;
    request.key.source.guid.value = 51U;
    request.key.source.generation = 2U;
    request.featureSchema.channels = {
        UVE::Core::MotionQueryFeatureChannelUVE{"root_velocity",
                                                UVE::Core::MotionQueryFeatureChannelKindUVE::RootVelocity,
                                                0U, 1.0F}};
    request.trajectoryOffsets = {0.0};
    request.samplePeriodSeconds = 0.0;
    UVE::Core::AnimationClipUVE clip;
    clip.clipId = "walk";
    clip.durationSeconds = 1.0;
    clip.samples = {UVE::Core::PoseSampleUVE{0.0, UVE::Core::TransformPoseUVE{}}};
    request.clips = {clip};

    UVE::Core::MotionMatchingDatabaseUVE database;
    UVE::Core::MotionMatchingCandidateUVE sentinel;
    sentinel.candidateId = "sentinel";
    database.candidates.push_back(std::move(sentinel));
    const MotionQueryClipSamplingResultUVE result =
        BuildMotionQueryDatabaseFromClipsUVE(request, database);
    EXPECT_EQ(result.code, MotionQueryClipSamplingResultUVE::Code::InvalidSamplingPeriod);
    ASSERT_EQ(database.candidates.size(), 1U);
    EXPECT_EQ(database.candidates.front().candidateId, "sentinel");
}

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
