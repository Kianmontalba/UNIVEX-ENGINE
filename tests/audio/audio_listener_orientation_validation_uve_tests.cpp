// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/audio/audio_listener_orientation_validation_uve.h"

#include <limits>

#include <gtest/gtest.h>

namespace UVE::Audio::Tests {
namespace {

TEST(AudioListenerOrientationValidationUVETest, AcceptsFiniteNonZeroBasisVectors) {
    EXPECT_TRUE(IsAudioListenerOrientationValidUVE(Math::Vector3UVE{0.0F, 0.0F, -1.0F},
                                                    Math::Vector3UVE{0.0F, 1.0F, 0.0F}));
}

TEST(AudioListenerOrientationValidationUVETest, RejectsZeroAndNonFiniteBasisVectors) {
    EXPECT_FALSE(IsAudioListenerOrientationValidUVE(Math::Vector3UVE{},
                                                     Math::Vector3UVE{0.0F, 1.0F, 0.0F}));
    EXPECT_FALSE(IsAudioListenerOrientationValidUVE(
        Math::Vector3UVE{std::numeric_limits<float>::quiet_NaN(), 0.0F, -1.0F},
        Math::Vector3UVE{0.0F, 1.0F, 0.0F}));
    EXPECT_FALSE(IsAudioListenerOrientationValidUVE(
        Math::Vector3UVE{0.0F, 0.0F, -1.0F},
        Math::Vector3UVE{std::numeric_limits<float>::infinity(), 1.0F, 0.0F}));
}

TEST(AudioListenerOrientationValidationUVETest, RejectsFiniteVectorsWhoseSquaredLengthOverflows) {
    constexpr float kLargeFiniteComponent = std::numeric_limits<float>::max();
    EXPECT_FALSE(IsAudioListenerOrientationValidUVE(
        Math::Vector3UVE{kLargeFiniteComponent, kLargeFiniteComponent, 0.0F},
        Math::Vector3UVE{0.0F, 1.0F, 0.0F}));
}

} // namespace
} // namespace UVE::Audio::Tests
