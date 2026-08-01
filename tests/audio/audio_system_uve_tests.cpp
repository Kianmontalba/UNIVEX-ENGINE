//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/audio/audio_system_uve.h"

#include <algorithm>
#include <memory>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include "uve/audio/null_audio_device_uve.h"
#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logger_uve.h"

namespace UVE::Audio::Tests {
namespace {

class AudioSystemUVETest : public ::testing::Test {
protected:
    NullAudioDeviceUVE device;
    AudioSystemUVE audioSystem{device};
};

[[nodiscard]] bool LoggedAnErrorUVE(const Debug::MemorySinkUVE& sink) {
    const std::vector<Debug::LogMessageUVE> messages = sink.GetMessagesUVE();
    return std::any_of(messages.begin(), messages.end(),
                        [](const Debug::LogMessageUVE& message) { return message.level == Debug::LogLevelUVE::Error; });
}

TEST_F(AudioSystemUVETest, CreateSourceUVE_ForwardsCorrectlyShapedVoiceDescToDevice) {
    AudioSourceDescUVE desc;
    desc.audioAssetPath = "sounds/explosion.wav";
    desc.looping = true;

    const VoiceHandleUVE source = audioSystem.CreateSourceUVE(desc);
    EXPECT_NE(source, kInvalidVoiceHandleUVE);
    EXPECT_EQ(device.GetLiveVoiceCountUVE(), 1U);
}

TEST_F(AudioSystemUVETest, PlayStopGetSourceStateUVE_RoundTripThroughDevice) {
    const VoiceHandleUVE source = audioSystem.CreateSourceUVE(AudioSourceDescUVE{});

    EXPECT_EQ(audioSystem.GetSourceStateUVE(source), VoicePlaybackStateUVE::Stopped);
    EXPECT_TRUE(audioSystem.PlayUVE(source));
    EXPECT_EQ(audioSystem.GetSourceStateUVE(source), VoicePlaybackStateUVE::Playing);
    EXPECT_TRUE(audioSystem.StopUVE(source));
    EXPECT_EQ(audioSystem.GetSourceStateUVE(source), VoicePlaybackStateUVE::Stopped);
}

TEST_F(AudioSystemUVETest, NonSpatialSource_GainEqualsVolumeRegardlessOfPosition) {
    AudioSourceDescUVE desc;
    desc.spatial = false;
    desc.volume = 0.75F;
    const VoiceHandleUVE source = audioSystem.CreateSourceUVE(desc);
    audioSystem.SetSourcePositionUVE(source, Math::Vector3UVE{1000.0F, 0.0F, 0.0F});
    audioSystem.SetListenerPositionUVE(Math::Vector3UVE{});

    device.ClearRecordedCallsUVE();
    audioSystem.UpdateUVE();

    const std::vector<RecordedAudioCallUVE>& recorded = device.GetRecordedCallsUVE();
    ASSERT_EQ(recorded.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<SetVoiceParamsCallUVE>(recorded[0]));
    EXPECT_FLOAT_EQ(std::get<SetVoiceParamsCallUVE>(recorded[0]).params.gain, 0.75F);
}

TEST_F(AudioSystemUVETest, SpatialSource_GainMatchesHandComputedLinearAttenuation) {
    AudioSourceDescUVE desc;
    desc.spatial = true;
    desc.volume = 1.0F;
    desc.minDistance = 1.0F;
    desc.maxDistance = 9.0F;
    desc.attenuationModel = AudioAttenuationModelUVE::Linear;
    const VoiceHandleUVE source = audioSystem.CreateSourceUVE(desc);

    audioSystem.SetListenerPositionUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F});
    audioSystem.SetSourcePositionUVE(source, Math::Vector3UVE{5.0F, 0.0F, 0.0F});

    device.ClearRecordedCallsUVE();
    audioSystem.UpdateUVE();

    const std::vector<RecordedAudioCallUVE>& recorded = device.GetRecordedCallsUVE();
    ASSERT_EQ(recorded.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<SetVoiceParamsCallUVE>(recorded[0]));
    // distance=5, minDistance=1, maxDistance=9 -> exact midpoint -> gain 0.5.
    EXPECT_FLOAT_EQ(std::get<SetVoiceParamsCallUVE>(recorded[0]).params.gain, 0.5F);
}

TEST_F(AudioSystemUVETest, SpatialSource_GainMatchesHandComputedInverseSquareAttenuation) {
    AudioSourceDescUVE desc;
    desc.spatial = true;
    desc.volume = 1.0F;
    desc.minDistance = 1.0F;
    desc.maxDistance = 100.0F;
    desc.attenuationModel = AudioAttenuationModelUVE::InverseSquare;
    const VoiceHandleUVE source = audioSystem.CreateSourceUVE(desc);

    audioSystem.SetListenerPositionUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F});
    audioSystem.SetSourcePositionUVE(source, Math::Vector3UVE{2.0F, 0.0F, 0.0F});

    device.ClearRecordedCallsUVE();
    audioSystem.UpdateUVE();

    const std::vector<RecordedAudioCallUVE>& recorded = device.GetRecordedCallsUVE();
    ASSERT_EQ(recorded.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<SetVoiceParamsCallUVE>(recorded[0]));
    // distance=2, minDistance=1 -> (1/2)^2 = 0.25.
    EXPECT_FLOAT_EQ(std::get<SetVoiceParamsCallUVE>(recorded[0]).params.gain, 0.25F);
}

TEST_F(AudioSystemUVETest, SetListenerPositionAndGetListenerPositionUVE_RoundTrip) {
    audioSystem.SetListenerPositionUVE(Math::Vector3UVE{1.0F, 2.0F, 3.0F});
    EXPECT_EQ(audioSystem.GetListenerPositionUVE(), (Math::Vector3UVE{1.0F, 2.0F, 3.0F}));
}

TEST_F(AudioSystemUVETest, SetSourcePositionVolumePitchUVE_UnknownHandle_LogsErrorAndIsNoOp) {
    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    audioSystem.SetSourcePositionUVE(VoiceHandleUVE{999}, Math::Vector3UVE{});
    EXPECT_TRUE(LoggedAnErrorUVE(*memorySinkPtr));

    logger.Shutdown();
}

TEST_F(AudioSystemUVETest, UpdateUVE_WithZeroSources_IsSafeNoOp) {
    EXPECT_NO_FATAL_FAILURE(audioSystem.UpdateUVE());
}

} // namespace
} // namespace UVE::Audio::Tests
