//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/audio/null_audio_device_uve.h"

#include <algorithm>
#include <memory>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logger_uve.h"

namespace UVE::Audio::Tests {
namespace {

[[nodiscard]] bool LoggedAnErrorUVE(const Debug::MemorySinkUVE& sink) {
    const std::vector<Debug::LogMessageUVE> messages = sink.GetMessagesUVE();
    return std::any_of(messages.begin(), messages.end(),
                        [](const Debug::LogMessageUVE& message) { return message.level == Debug::LogLevelUVE::Error; });
}

TEST(NullAudioDeviceUVETest, CreateVoiceUVE_ReturnsUniqueHandles) {
    NullAudioDeviceUVE device;
    const VoiceHandleUVE first = device.CreateVoiceUVE(AudioVoiceDescUVE{});
    const VoiceHandleUVE second = device.CreateVoiceUVE(AudioVoiceDescUVE{});

    EXPECT_NE(first, kInvalidVoiceHandleUVE);
    EXPECT_NE(second, kInvalidVoiceHandleUVE);
    EXPECT_NE(first, second);
}

TEST(NullAudioDeviceUVETest, DestroyVoiceUVE_UnknownHandle_LogsErrorSafely) {
    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    NullAudioDeviceUVE device;
    device.DestroyVoiceUVE(VoiceHandleUVE{999});

    EXPECT_TRUE(LoggedAnErrorUVE(*memorySinkPtr));
    logger.Shutdown();
}

TEST(NullAudioDeviceUVETest, PlayUVE_UnknownHandle_ReturnsFalseAndLogsError) {
    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    NullAudioDeviceUVE device;
    EXPECT_FALSE(device.PlayUVE(VoiceHandleUVE{999}));

    EXPECT_TRUE(LoggedAnErrorUVE(*memorySinkPtr));
    logger.Shutdown();
}

TEST(NullAudioDeviceUVETest, StopUVE_UnknownHandle_ReturnsFalseAndLogsError) {
    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    NullAudioDeviceUVE device;
    EXPECT_FALSE(device.StopUVE(VoiceHandleUVE{999}));

    EXPECT_TRUE(LoggedAnErrorUVE(*memorySinkPtr));
    logger.Shutdown();
}

TEST(NullAudioDeviceUVETest, SetVoiceParamsUVE_UnknownHandle_ReturnsFalseAndLogsError) {
    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    NullAudioDeviceUVE device;
    EXPECT_FALSE(device.SetVoiceParamsUVE(VoiceHandleUVE{999}, AudioVoiceParamsUVE{}));

    EXPECT_TRUE(LoggedAnErrorUVE(*memorySinkPtr));
    logger.Shutdown();
}

TEST(NullAudioDeviceUVETest, GetVoiceStateUVE_UnknownHandle_ReturnsStoppedAndLogsError) {
    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    NullAudioDeviceUVE device;
    EXPECT_EQ(device.GetVoiceStateUVE(VoiceHandleUVE{999}), VoicePlaybackStateUVE::Stopped);

    EXPECT_TRUE(LoggedAnErrorUVE(*memorySinkPtr));
    logger.Shutdown();
}

TEST(NullAudioDeviceUVETest, GetVoiceStateUVE_TracksPlayAndStop) {
    NullAudioDeviceUVE device;
    const VoiceHandleUVE voice = device.CreateVoiceUVE(AudioVoiceDescUVE{});

    EXPECT_EQ(device.GetVoiceStateUVE(voice), VoicePlaybackStateUVE::Stopped);
    EXPECT_TRUE(device.PlayUVE(voice));
    EXPECT_EQ(device.GetVoiceStateUVE(voice), VoicePlaybackStateUVE::Playing);
    EXPECT_TRUE(device.StopUVE(voice));
    EXPECT_EQ(device.GetVoiceStateUVE(voice), VoicePlaybackStateUVE::Stopped);
}

TEST(NullAudioDeviceUVETest, GetLiveVoiceCountUVE_TracksCreateAndDestroy) {
    NullAudioDeviceUVE device;
    EXPECT_EQ(device.GetLiveVoiceCountUVE(), 0U);

    const VoiceHandleUVE voice = device.CreateVoiceUVE(AudioVoiceDescUVE{});
    EXPECT_EQ(device.GetLiveVoiceCountUVE(), 1U);

    device.DestroyVoiceUVE(voice);
    EXPECT_EQ(device.GetLiveVoiceCountUVE(), 0U);
}

TEST(NullAudioDeviceUVETest, GetBackendNameUVE_ReturnsNull) {
    NullAudioDeviceUVE device;
    EXPECT_EQ(device.GetBackendNameUVE(), "Null");
}

TEST(NullAudioDeviceUVETest, GetRecordedCallsUVE_RecordsPlaySetParamsStopInOrder) {
    NullAudioDeviceUVE device;
    const VoiceHandleUVE voice = device.CreateVoiceUVE(AudioVoiceDescUVE{});

    EXPECT_TRUE(device.PlayUVE(voice));
    const AudioVoiceParamsUVE params{Math::Vector3UVE{1.0F, 2.0F, 3.0F}, 0.5F, 1.0F};
    EXPECT_TRUE(device.SetVoiceParamsUVE(voice, params));
    EXPECT_TRUE(device.StopUVE(voice));

    const std::vector<RecordedAudioCallUVE>& recorded = device.GetRecordedCallsUVE();
    ASSERT_EQ(recorded.size(), 3U);
    EXPECT_TRUE(std::holds_alternative<PlayVoiceCallUVE>(recorded[0]));
    EXPECT_TRUE(std::holds_alternative<SetVoiceParamsCallUVE>(recorded[1]));
    EXPECT_TRUE(std::holds_alternative<StopVoiceCallUVE>(recorded[2]));

    EXPECT_EQ(std::get<PlayVoiceCallUVE>(recorded[0]).voice, voice);
    EXPECT_EQ(std::get<SetVoiceParamsCallUVE>(recorded[1]).voice, voice);
    EXPECT_EQ(std::get<SetVoiceParamsCallUVE>(recorded[1]).params.gain, 0.5F);
    EXPECT_EQ(std::get<StopVoiceCallUVE>(recorded[2]).voice, voice);
}

TEST(NullAudioDeviceUVETest, ClearRecordedCallsUVE_EmptiesLogWithoutAffectingLiveVoiceCount) {
    NullAudioDeviceUVE device;
    const VoiceHandleUVE voice = device.CreateVoiceUVE(AudioVoiceDescUVE{});
    EXPECT_TRUE(device.PlayUVE(voice));
    ASSERT_FALSE(device.GetRecordedCallsUVE().empty());

    device.ClearRecordedCallsUVE();

    EXPECT_TRUE(device.GetRecordedCallsUVE().empty());
    EXPECT_EQ(device.GetLiveVoiceCountUVE(), 1U);
}

} // namespace
} // namespace UVE::Audio::Tests
