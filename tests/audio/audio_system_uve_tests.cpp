// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/audio/audio_system_uve.h"

#include <algorithm>
#include <limits>
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

class RecordingAudioDeviceUVE final : public IAudioDeviceUVE {
public:
    [[nodiscard]] VoiceHandleUVE CreateVoiceUVE(const AudioVoiceDescUVE& desc) override {
        lastDescription = desc;
        ++createCount;
        return nextHandle;
    }
    void DestroyVoiceUVE(VoiceHandleUVE) override {}
    [[nodiscard]] bool PlayUVE(VoiceHandleUVE) override { return true; }
    [[nodiscard]] bool StopUVE(VoiceHandleUVE) override { return true; }
    [[nodiscard]] bool SetVoiceParamsUVE(VoiceHandleUVE, const AudioVoiceParamsUVE&) override { return true; }
    [[nodiscard]] VoicePlaybackStateUVE GetVoiceStateUVE(VoiceHandleUVE) const override {
        return VoicePlaybackStateUVE::Stopped;
    }
    [[nodiscard]] std::string_view GetBackendNameUVE() const noexcept override { return "Recording"; }

    AudioVoiceDescUVE lastDescription{};
    VoiceHandleUVE nextHandle{1U};
    int createCount = 0;
};

class TestAudioClipResolverUVE final : public IAudioClipResolverUVE {
public:
    [[nodiscard]] AudioClipResolutionUVE ResolveAudioClipUVE(std::string_view) const override {
        ++resolveCount;
        return AudioClipResolutionUVE{accepted, resolvedPath, diagnostic};
    }

    bool accepted = true;
    std::string resolvedPath;
    std::string diagnostic;
    mutable int resolveCount = 0;
};

TEST(AudioClipResolutionUVETest, AcceptedResolverPathReachesDeviceAndCopiesIntoSourceState) {
    RecordingAudioDeviceUVE device;
    TestAudioClipResolverUVE resolver;
    resolver.resolvedPath = "audio/derived/explosion.clip";
    AudioSystemUVE audioSystem{device, &resolver};

    AudioSourceDescUVE desc;
    desc.audioAssetPath = "sounds/explosion.wav";
    const VoiceHandleUVE source = audioSystem.CreateSourceUVE(desc);

    EXPECT_NE(source, kInvalidVoiceHandleUVE);
    EXPECT_EQ(resolver.resolveCount, 1);
    EXPECT_EQ(device.createCount, 1);
    EXPECT_EQ(device.lastDescription.audioAssetPath, resolver.resolvedPath);
}

TEST(AudioSystemDeviceFailureUVETest, InvalidDeviceVoiceHandleFailsBeforeSourcePublication) {
    RecordingAudioDeviceUVE device;
    device.nextHandle = kInvalidVoiceHandleUVE;
    AudioSystemUVE audioSystem{device};

    const VoiceHandleUVE source = audioSystem.CreateSourceUVE(AudioSourceDescUVE{});

    EXPECT_EQ(source, kInvalidVoiceHandleUVE);
    EXPECT_EQ(device.createCount, 1);
    EXPECT_EQ(audioSystem.GetMixerDiagnosticsUVE().routedSourceCount, 0U);
}

TEST(AudioSystemDeviceFailureUVETest, DuplicateDeviceVoiceHandleFailsBeforeSecondSourcePublication) {
    RecordingAudioDeviceUVE device;
    AudioSystemUVE audioSystem{device};

    const VoiceHandleUVE first = audioSystem.CreateSourceUVE(AudioSourceDescUVE{});
    const VoiceHandleUVE duplicate = audioSystem.CreateSourceUVE(AudioSourceDescUVE{});

    EXPECT_EQ(first, VoiceHandleUVE{1U});
    EXPECT_EQ(duplicate, kInvalidVoiceHandleUVE);
    EXPECT_EQ(device.createCount, 2);
    EXPECT_EQ(audioSystem.GetMixerDiagnosticsUVE().routedSourceCount, 1U);
}

TEST(AudioClipResolutionUVETest, RejectedResolverPathFailsAtomicallyBeforeDeviceCreation) {
    RecordingAudioDeviceUVE device;
    TestAudioClipResolverUVE resolver;
    resolver.accepted = false;
    resolver.diagnostic = "clip is missing";
    AudioSystemUVE audioSystem{device, &resolver};

    AudioSourceDescUVE desc;
    desc.audioAssetPath = "sounds/missing.wav";
    const VoiceHandleUVE source = audioSystem.CreateSourceUVE(desc);

    EXPECT_EQ(source, kInvalidVoiceHandleUVE);
    EXPECT_EQ(resolver.resolveCount, 1);
    EXPECT_EQ(device.createCount, 0);
}

TEST(AudioSystemSourceValidationUVETest, CreateSourceUVE_RejectsInvalidDescriptorBeforeResolverOrDevice) {
    RecordingAudioDeviceUVE device;
    TestAudioClipResolverUVE resolver;
    AudioSystemUVE audioSystem{device, &resolver};

    AudioSourceDescUVE descriptor;
    descriptor.audioAssetPath = "sounds/invalid.wav";
    descriptor.minDistance = 5.0F;
    descriptor.maxDistance = 5.0F;
    EXPECT_EQ(audioSystem.CreateSourceUVE(descriptor), kInvalidVoiceHandleUVE);

    descriptor = AudioSourceDescUVE{};
    descriptor.pitch = std::numeric_limits<float>::quiet_NaN();
    EXPECT_EQ(audioSystem.CreateSourceUVE(descriptor), kInvalidVoiceHandleUVE);
    EXPECT_EQ(resolver.resolveCount, 0);
    EXPECT_EQ(device.createCount, 0);
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

TEST_F(AudioSystemUVETest, SpatialSource_OverflowedListenerDistanceFailsClosedToFiniteSilence) {
    AudioSourceDescUVE desc;
    desc.spatial = true;
    desc.minDistance = 1.0F;
    desc.maxDistance = std::numeric_limits<float>::max();
    const VoiceHandleUVE source = audioSystem.CreateSourceUVE(desc);
    const float maximumFloat = std::numeric_limits<float>::max();

    audioSystem.SetListenerPositionUVE(Math::Vector3UVE{-maximumFloat, 0.0F, 0.0F});
    audioSystem.SetSourcePositionUVE(source, Math::Vector3UVE{maximumFloat, 0.0F, 0.0F});

    device.ClearRecordedCallsUVE();
    audioSystem.UpdateUVE();

    const std::vector<RecordedAudioCallUVE>& recorded = device.GetRecordedCallsUVE();
    ASSERT_EQ(recorded.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<SetVoiceParamsCallUVE>(recorded[0]));
    const AudioVoiceParamsUVE& params = std::get<SetVoiceParamsCallUVE>(recorded[0]).params;
    EXPECT_TRUE(ValidateAudioVoiceParamsUVE(params));
    EXPECT_FLOAT_EQ(params.gain, 0.0F);
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

TEST_F(AudioSystemUVETest, MixerGroup_ScalesFinalGainAndPitchBeforeDeviceSubmission) {
    ASSERT_TRUE(audioSystem.RegisterMixerGroupUVE("SFX", 0.5F, 1.5F));

    AudioSourceDescUVE desc;
    desc.spatial = false;
    desc.volume = 0.8F;
    desc.pitch = 1.2F;
    desc.mixerGroup = "SFX";
    const VoiceHandleUVE source = audioSystem.CreateSourceUVE(desc);

    device.ClearRecordedCallsUVE();
    audioSystem.UpdateUVE();

    const std::vector<RecordedAudioCallUVE>& recorded = device.GetRecordedCallsUVE();
    ASSERT_EQ(recorded.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<SetVoiceParamsCallUVE>(recorded[0]));
    EXPECT_FLOAT_EQ(std::get<SetVoiceParamsCallUVE>(recorded[0]).params.gain, 0.4F);
    EXPECT_FLOAT_EQ(std::get<SetVoiceParamsCallUVE>(recorded[0]).params.pitch, 1.8F);

    const AudioMixerDiagnosticsUVE diagnostics = audioSystem.GetMixerDiagnosticsUVE();
    ASSERT_EQ(diagnostics.routedSourceCount, 1U);
    ASSERT_EQ(diagnostics.groups.back().name, "SFX");
    EXPECT_EQ(diagnostics.groups.back().sourceCount, 1U);
    EXPECT_EQ(source, VoiceHandleUVE{1U});
}

TEST_F(AudioSystemUVETest, MixerGroup_RerouteAndDestroyKeepsCopiedCountsConsistent) {
    ASSERT_TRUE(audioSystem.RegisterMixerGroupUVE("Music"));
    const VoiceHandleUVE source = audioSystem.CreateSourceUVE(AudioSourceDescUVE{});

    ASSERT_TRUE(audioSystem.SetSourceMixerGroupUVE(source, "Music"));
    AudioMixerDiagnosticsUVE diagnostics = audioSystem.GetMixerDiagnosticsUVE();
    ASSERT_EQ(diagnostics.routedSourceCount, 1U);
    ASSERT_EQ(diagnostics.groups[0].name, kMasterAudioMixerGroupNameUVE);
    EXPECT_EQ(diagnostics.groups[0].sourceCount, 0U);
    EXPECT_EQ(diagnostics.groups[1].name, "Music");
    EXPECT_EQ(diagnostics.groups[1].sourceCount, 1U);
    EXPECT_FALSE(audioSystem.RemoveMixerGroupUVE("Music"));

    audioSystem.DestroySourceUVE(source);
    diagnostics = audioSystem.GetMixerDiagnosticsUVE();
    EXPECT_EQ(diagnostics.routedSourceCount, 0U);
    EXPECT_TRUE(audioSystem.RemoveMixerGroupUVE("Music"));
}

TEST_F(AudioSystemUVETest, MixerGroup_UnknownSourceGroupFallsBackToMaster) {
    AudioSourceDescUVE desc;
    desc.mixerGroup = "Missing";
    const VoiceHandleUVE source = audioSystem.CreateSourceUVE(desc);

    const AudioMixerDiagnosticsUVE diagnostics = audioSystem.GetMixerDiagnosticsUVE();
    ASSERT_EQ(diagnostics.routedSourceCount, 1U);
    EXPECT_EQ(diagnostics.groups[0].name, kMasterAudioMixerGroupNameUVE);
    EXPECT_EQ(diagnostics.groups[0].sourceCount, 1U);
    EXPECT_FALSE(audioSystem.SetSourceMixerGroupUVE(source, "Missing"));
}

TEST_F(AudioSystemUVETest, SetListenerPositionAndGetListenerPositionUVE_RoundTrip) {
    audioSystem.SetListenerPositionUVE(Math::Vector3UVE{1.0F, 2.0F, 3.0F});
    EXPECT_EQ(audioSystem.GetListenerPositionUVE(), (Math::Vector3UVE{1.0F, 2.0F, 3.0F}));
}

TEST_F(AudioSystemUVETest, SetListenerPositionUVE_RejectsNonFiniteAndPreservesLastValidState) {
    const Math::Vector3UVE validPosition{1.0F, 2.0F, 3.0F};
    audioSystem.SetListenerPositionUVE(validPosition);
    audioSystem.SetListenerPositionUVE(
        Math::Vector3UVE{std::numeric_limits<float>::infinity(), 0.0F, 0.0F});

    EXPECT_EQ(audioSystem.GetListenerPositionUVE(), validPosition);
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

TEST_F(AudioSystemUVETest, InvalidSourceRuntimeParameters_PreserveLastValidState) {
    AudioSourceDescUVE desc;
    desc.spatial = false;
    const VoiceHandleUVE source = audioSystem.CreateSourceUVE(desc);
    audioSystem.SetSourcePositionUVE(source, Math::Vector3UVE{2.0F, 3.0F, 4.0F});
    audioSystem.SetSourceVolumeUVE(source, 0.5F);
    audioSystem.SetSourcePitchUVE(source, 1.5F);

    audioSystem.SetSourcePositionUVE(
        source, Math::Vector3UVE{std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F});
    audioSystem.SetSourceVolumeUVE(source, std::numeric_limits<float>::infinity());
    audioSystem.SetSourcePitchUVE(source, -1.0F);

    device.ClearRecordedCallsUVE();
    audioSystem.UpdateUVE();
    const std::vector<RecordedAudioCallUVE>& recorded = device.GetRecordedCallsUVE();
    ASSERT_EQ(recorded.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<SetVoiceParamsCallUVE>(recorded[0]));
    const AudioVoiceParamsUVE& params = std::get<SetVoiceParamsCallUVE>(recorded[0]).params;
    EXPECT_EQ(params.position, (Math::Vector3UVE{2.0F, 3.0F, 4.0F}));
    EXPECT_FLOAT_EQ(params.gain, 0.5F);
    EXPECT_FLOAT_EQ(params.pitch, 1.5F);
}

TEST_F(AudioSystemUVETest, UpdateUVE_WithZeroSources_IsSafeNoOp) {
    EXPECT_NO_FATAL_FAILURE(audioSystem.UpdateUVE());
}

} // namespace
} // namespace UVE::Audio::Tests
