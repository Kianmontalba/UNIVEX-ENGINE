// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/audio/audio_attenuation_model_uve.h"

#include <gtest/gtest.h>

namespace UVE::Audio::Tests {
namespace {

TEST(AudioAttenuationModelUVETest, DistanceAtOrBelowMinDistance_ReturnsFullGain_BothModels) {
    EXPECT_FLOAT_EQ(ComputeDistanceAttenuationUVE(1.0F, 1.0F, 10.0F, AudioAttenuationModelUVE::Linear), 1.0F);
    EXPECT_FLOAT_EQ(ComputeDistanceAttenuationUVE(0.5F, 1.0F, 10.0F, AudioAttenuationModelUVE::Linear), 1.0F);
    EXPECT_FLOAT_EQ(ComputeDistanceAttenuationUVE(1.0F, 1.0F, 10.0F, AudioAttenuationModelUVE::InverseSquare), 1.0F);
    EXPECT_FLOAT_EQ(ComputeDistanceAttenuationUVE(0.5F, 1.0F, 10.0F, AudioAttenuationModelUVE::InverseSquare), 1.0F);
}

TEST(AudioAttenuationModelUVETest, DistanceAtOrBeyondMaxDistance_ReturnsZeroGain_BothModels) {
    EXPECT_FLOAT_EQ(ComputeDistanceAttenuationUVE(10.0F, 1.0F, 10.0F, AudioAttenuationModelUVE::Linear), 0.0F);
    EXPECT_FLOAT_EQ(ComputeDistanceAttenuationUVE(20.0F, 1.0F, 10.0F, AudioAttenuationModelUVE::Linear), 0.0F);
    EXPECT_FLOAT_EQ(ComputeDistanceAttenuationUVE(10.0F, 1.0F, 10.0F, AudioAttenuationModelUVE::InverseSquare), 0.0F);
    EXPECT_FLOAT_EQ(ComputeDistanceAttenuationUVE(20.0F, 1.0F, 10.0F, AudioAttenuationModelUVE::InverseSquare), 0.0F);
}

TEST(AudioAttenuationModelUVETest, Linear_AtMidpoint_ReturnsHalfGain) {
    // minDistance=1, maxDistance=9 -> midpoint distance=5.
    EXPECT_FLOAT_EQ(ComputeDistanceAttenuationUVE(5.0F, 1.0F, 9.0F, AudioAttenuationModelUVE::Linear), 0.5F);
}

TEST(AudioAttenuationModelUVETest, InverseSquare_AtTwiceMinDistance_ReturnsQuarterGain) {
    // (minDistance / distance)^2 = (1/2)^2 = 0.25.
    EXPECT_FLOAT_EQ(ComputeDistanceAttenuationUVE(2.0F, 1.0F, 100.0F, AudioAttenuationModelUVE::InverseSquare), 0.25F);
}

TEST(AudioAttenuationModelUVETest, Linear_StrictlyDecreasesWithDistance) {
    const float near = ComputeDistanceAttenuationUVE(2.0F, 1.0F, 10.0F, AudioAttenuationModelUVE::Linear);
    const float mid = ComputeDistanceAttenuationUVE(5.0F, 1.0F, 10.0F, AudioAttenuationModelUVE::Linear);
    const float far = ComputeDistanceAttenuationUVE(8.0F, 1.0F, 10.0F, AudioAttenuationModelUVE::Linear);
    EXPECT_GT(near, mid);
    EXPECT_GT(mid, far);
}

TEST(AudioAttenuationModelUVETest, InverseSquare_StrictlyDecreasesWithDistance) {
    const float near = ComputeDistanceAttenuationUVE(2.0F, 1.0F, 100.0F, AudioAttenuationModelUVE::InverseSquare);
    const float mid = ComputeDistanceAttenuationUVE(10.0F, 1.0F, 100.0F, AudioAttenuationModelUVE::InverseSquare);
    const float far = ComputeDistanceAttenuationUVE(50.0F, 1.0F, 100.0F, AudioAttenuationModelUVE::InverseSquare);
    EXPECT_GT(near, mid);
    EXPECT_GT(mid, far);
}

} // namespace
} // namespace UVE::Audio::Tests
