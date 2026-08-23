// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/audio/audio_source_system_uve.h"

#include <algorithm>
#include <limits>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include "uve/audio/audio_system_uve.h"
#include "uve/audio/i_audio_clip_resolver_uve.h"
#include "uve/audio/null_audio_device_uve.h"
#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/scene/components/audio_source_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"
#include "uve/scene/entity_manager_uve.h"
#include "uve/scene/scene_graph_uve.h"

namespace UVE::Audio::Tests {
namespace {

class AudioSourceSystemUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    Scene::EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    Scene::SceneGraphUVE sceneGraph;
    NullAudioDeviceUVE device;
    AudioSystemUVE audioSystem{device};
    AudioSourceSystemUVE audioSourceSystem;

    Scene::EntityUVE MakeAudioEntityUVE(Scene::AudioSourceComponentUVE audioSource) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        sceneGraph.AttachTransformUVE(entityManager, entity, Scene::TransformComponentUVE{});
        sceneGraph.UpdateUVE(entityManager);
        entityManager.AddComponentUVE<Scene::AudioSourceComponentUVE>(entity, std::move(audioSource));
        return entity;
    }

    [[nodiscard]] bool AnyRecordedPlayCallUVE() const {
        const std::vector<RecordedAudioCallUVE>& recorded = device.GetRecordedCallsUVE();
        return std::any_of(recorded.begin(), recorded.end(),
                            [](const RecordedAudioCallUVE& call) { return std::holds_alternative<PlayVoiceCallUVE>(call); });
    }
};

class RejectingClipResolverUVE final : public IAudioClipResolverUVE {
public:
    [[nodiscard]] AudioClipResolutionUVE ResolveAudioClipUVE(const std::string_view authoredPath) const override {
        if (authoredPath == "rejected.wav") {
            return AudioClipResolutionUVE{false, {}, "test rejection"};
        }
        return AudioClipResolutionUVE{true, std::string(authoredPath), {}};
    }
};

TEST(AudioSourceComponentUVETest, IsAudioSourceComponentValidUVE_RejectsUnsafeParameters) {
    EXPECT_TRUE(Scene::IsAudioSourceComponentValidUVE(Scene::AudioSourceComponentUVE{}));

    Scene::AudioSourceComponentUVE invalid = {};
    invalid.volume = -0.1F;
    EXPECT_FALSE(Scene::IsAudioSourceComponentValidUVE(invalid));
    invalid = {};
    invalid.pitch = 0.0F;
    EXPECT_FALSE(Scene::IsAudioSourceComponentValidUVE(invalid));
    invalid = {};
    invalid.pitch = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(Scene::IsAudioSourceComponentValidUVE(invalid));
    invalid = {};
    invalid.minDistance = 0.0F;
    EXPECT_FALSE(Scene::IsAudioSourceComponentValidUVE(invalid));
    invalid = {};
    invalid.maxDistance = invalid.minDistance;
    EXPECT_FALSE(Scene::IsAudioSourceComponentValidUVE(invalid));
    invalid = {};
    invalid.spatial = false;
    invalid.minDistance = 0.0F;
    invalid.maxDistance = 0.0F;
    EXPECT_TRUE(Scene::IsAudioSourceComponentValidUVE(invalid));
    invalid = {};
    invalid.attenuationCurve = static_cast<Scene::AudioAttenuationCurveUVE>(99U);
    EXPECT_FALSE(Scene::IsAudioSourceComponentValidUVE(invalid));
    invalid = {};
    invalid.mixerGroup.assign(65U, 'X');
    EXPECT_FALSE(Scene::IsAudioSourceComponentValidUVE(invalid));
    invalid = {};
    invalid.mixerGroup = std::string("SFX\0invalid", 11U);
    EXPECT_FALSE(Scene::IsAudioSourceComponentValidUVE(invalid));
}

TEST_F(AudioSourceSystemUVETest, PlayOnAwakeEntity_CreatesAndPlaysSourceOnFirstSync) {
    Scene::AudioSourceComponentUVE audioSource;
    audioSource.playOnAwake = true;
    MakeAudioEntityUVE(audioSource);

    audioSourceSystem.SyncUVE(entityManager, audioSystem);

    EXPECT_EQ(device.GetLiveVoiceCountUVE(), 1U);
    EXPECT_TRUE(AnyRecordedPlayCallUVE());
}

TEST_F(AudioSourceSystemUVETest, AuthoredMixerGroup_RoutesAndReroutesWithoutReplacingVoice) {
    ASSERT_TRUE(audioSystem.RegisterMixerGroupUVE("SFX"));
    ASSERT_TRUE(audioSystem.RegisterMixerGroupUVE("Music"));
    Scene::AudioSourceComponentUVE audioSource;
    audioSource.mixerGroup = "SFX";
    const Scene::EntityUVE entity = MakeAudioEntityUVE(audioSource);

    audioSourceSystem.SyncUVE(entityManager, audioSystem);
    ASSERT_EQ(device.GetLiveVoiceCountUVE(), 1U);
    auto diagnostics = audioSystem.GetMixerDiagnosticsUVE();
    ASSERT_EQ(diagnostics.groups.size(), 3U);
    const auto sourceCountFor = [](const AudioMixerDiagnosticsUVE& snapshot, const std::string_view name) {
        const auto iterator = std::find_if(snapshot.groups.begin(), snapshot.groups.end(),
                                           [name](const AudioMixerGroupSnapshotUVE& group) { return group.name == name; });
        return iterator == snapshot.groups.end() ? 0U : iterator->sourceCount;
    };
    EXPECT_EQ(sourceCountFor(diagnostics, "SFX"), 1U);

    device.ClearRecordedCallsUVE();
    entityManager.GetComponentUVE<Scene::AudioSourceComponentUVE>(entity).mixerGroup = "Music";
    audioSourceSystem.SyncUVE(entityManager, audioSystem);
    EXPECT_EQ(device.GetLiveVoiceCountUVE(), 1U);
    diagnostics = audioSystem.GetMixerDiagnosticsUVE();
    EXPECT_EQ(sourceCountFor(diagnostics, "SFX"), 0U);
    EXPECT_EQ(sourceCountFor(diagnostics, "Music"), 1U);
}

TEST_F(AudioSourceSystemUVETest, PlayOnAwakeFalse_CreatesButDoesNotAutoPlay) {
    Scene::AudioSourceComponentUVE audioSource;
    audioSource.playOnAwake = false;
    MakeAudioEntityUVE(audioSource);

    audioSourceSystem.SyncUVE(entityManager, audioSystem);

    EXPECT_EQ(device.GetLiveVoiceCountUVE(), 1U);
    EXPECT_FALSE(AnyRecordedPlayCallUVE());
}

TEST_F(AudioSourceSystemUVETest, MovingEntity_UpdatesPushedPositionAndGain) {
    Scene::AudioSourceComponentUVE audioSource;
    audioSource.spatial = true;
    audioSource.minDistance = 1.0F;
    audioSource.maxDistance = 9.0F;
    const Scene::EntityUVE entity = MakeAudioEntityUVE(audioSource);

    audioSourceSystem.SyncUVE(entityManager, audioSystem);
    audioSystem.UpdateUVE();

    Scene::WorldTransformComponentUVE& worldTransform =
        entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(entity);
    worldTransform.worldPosition = Math::Vector3UVE{5.0F, 0.0F, 0.0F};

    device.ClearRecordedCallsUVE();
    audioSourceSystem.SyncUVE(entityManager, audioSystem);
    audioSystem.UpdateUVE();

    const std::vector<RecordedAudioCallUVE>& recorded = device.GetRecordedCallsUVE();
    const auto iterator = std::find_if(recorded.begin(), recorded.end(), [](const RecordedAudioCallUVE& call) {
        return std::holds_alternative<SetVoiceParamsCallUVE>(call);
    });
    ASSERT_NE(iterator, recorded.end());
    const auto& params = std::get<SetVoiceParamsCallUVE>(*iterator).params;
    EXPECT_EQ(params.position, (Math::Vector3UVE{5.0F, 0.0F, 0.0F}));
    // distance=5, minDistance=1, maxDistance=9 -> exact midpoint -> gain 0.5.
    EXPECT_FLOAT_EQ(params.gain, 0.5F);
}

TEST_F(AudioSourceSystemUVETest, InvalidAudioSource_SkipsCreationAndPreservesExistingVoice) {
    Scene::AudioSourceComponentUVE invalid = {};
    invalid.spatial = true;
    invalid.minDistance = 2.0F;
    invalid.maxDistance = 2.0F;
    const Scene::EntityUVE entity = MakeAudioEntityUVE(invalid);

    audioSourceSystem.SyncUVE(entityManager, audioSystem);
    EXPECT_EQ(device.GetLiveVoiceCountUVE(), 0U);
    EXPECT_TRUE(device.GetRecordedCallsUVE().empty());

    entityManager.GetComponentUVE<Scene::AudioSourceComponentUVE>(entity).maxDistance = 4.0F;
    audioSourceSystem.SyncUVE(entityManager, audioSystem);
    ASSERT_EQ(device.GetLiveVoiceCountUVE(), 1U);
    device.ClearRecordedCallsUVE();

    entityManager.GetComponentUVE<Scene::AudioSourceComponentUVE>(entity).maxDistance = 2.0F;
    audioSourceSystem.SyncUVE(entityManager, audioSystem);

    EXPECT_EQ(device.GetLiveVoiceCountUVE(), 1U);
    EXPECT_TRUE(device.GetRecordedCallsUVE().empty());
}

TEST_F(AudioSourceSystemUVETest, RemovingAudioSourceComponent_DestroysVoice) {
    Scene::AudioSourceComponentUVE audioSource;
    const Scene::EntityUVE entity = MakeAudioEntityUVE(audioSource);

    audioSourceSystem.SyncUVE(entityManager, audioSystem);
    ASSERT_EQ(device.GetLiveVoiceCountUVE(), 1U);

    entityManager.RemoveComponentUVE<Scene::AudioSourceComponentUVE>(entity);
    audioSourceSystem.SyncUVE(entityManager, audioSystem);

    EXPECT_EQ(device.GetLiveVoiceCountUVE(), 0U);
}

TEST_F(AudioSourceSystemUVETest, DestroyingEntity_DestroysVoice) {
    Scene::AudioSourceComponentUVE audioSource;
    const Scene::EntityUVE entity = MakeAudioEntityUVE(audioSource);

    audioSourceSystem.SyncUVE(entityManager, audioSystem);
    ASSERT_EQ(device.GetLiveVoiceCountUVE(), 1U);

    entityManager.DestroyEntityUVE(entity);
    audioSourceSystem.SyncUVE(entityManager, audioSystem);

    EXPECT_EQ(device.GetLiveVoiceCountUVE(), 0U);
}

TEST_F(AudioSourceSystemUVETest, RepeatedSyncUVE_UnchangedScene_DoesNotCreateASecondVoice) {
    Scene::AudioSourceComponentUVE audioSource;
    MakeAudioEntityUVE(audioSource);

    audioSourceSystem.SyncUVE(entityManager, audioSystem);
    ASSERT_EQ(device.GetLiveVoiceCountUVE(), 1U);

    audioSourceSystem.SyncUVE(entityManager, audioSystem);
    audioSourceSystem.SyncUVE(entityManager, audioSystem);

    EXPECT_EQ(device.GetLiveVoiceCountUVE(), 1U);
}

TEST_F(AudioSourceSystemUVETest, ChangingAudioAssetPath_ReplacesVoiceAndPreservesCount) {
    Scene::AudioSourceComponentUVE audioSource;
    audioSource.audioAssetPath = "a.wav";
    audioSource.playOnAwake = true;
    const Scene::EntityUVE entity = MakeAudioEntityUVE(audioSource);

    audioSourceSystem.SyncUVE(entityManager, audioSystem);
    ASSERT_EQ(device.GetLiveVoiceCountUVE(), 1U);
    device.ClearRecordedCallsUVE();

    entityManager.GetComponentUVE<Scene::AudioSourceComponentUVE>(entity).audioAssetPath = "b.wav";
    audioSourceSystem.SyncUVE(entityManager, audioSystem);

    EXPECT_EQ(device.GetLiveVoiceCountUVE(), 1U);
    EXPECT_TRUE(AnyRecordedPlayCallUVE());
}

TEST_F(AudioSourceSystemUVETest, ChangingLooping_ReplacesVoiceAtomically) {
    Scene::AudioSourceComponentUVE audioSource;
    audioSource.looping = false;
    audioSource.playOnAwake = true;
    const Scene::EntityUVE entity = MakeAudioEntityUVE(audioSource);

    audioSourceSystem.SyncUVE(entityManager, audioSystem);
    ASSERT_EQ(device.GetLiveVoiceCountUVE(), 1U);
    device.ClearRecordedCallsUVE();

    entityManager.GetComponentUVE<Scene::AudioSourceComponentUVE>(entity).looping = true;
    audioSourceSystem.SyncUVE(entityManager, audioSystem);

    EXPECT_EQ(device.GetLiveVoiceCountUVE(), 1U);
    EXPECT_TRUE(AnyRecordedPlayCallUVE());
}

TEST_F(AudioSourceSystemUVETest, UnchangedDescriptor_DoesNotReplaceVoice) {
    Scene::AudioSourceComponentUVE audioSource;
    audioSource.playOnAwake = false;
    MakeAudioEntityUVE(audioSource);

    audioSourceSystem.SyncUVE(entityManager, audioSystem);
    ASSERT_EQ(device.GetLiveVoiceCountUVE(), 1U);
    device.ClearRecordedCallsUVE();

    audioSourceSystem.SyncUVE(entityManager, audioSystem);

    EXPECT_EQ(device.GetLiveVoiceCountUVE(), 1U);
    EXPECT_TRUE(device.GetRecordedCallsUVE().empty());
}

TEST_F(AudioSourceSystemUVETest, ReplacementPreservesPlayingState) {
    Scene::AudioSourceComponentUVE audioSource;
    audioSource.audioAssetPath = "playing-a.wav";
    audioSource.playOnAwake = true;
    const Scene::EntityUVE entity = MakeAudioEntityUVE(audioSource);

    audioSourceSystem.SyncUVE(entityManager, audioSystem);
    ASSERT_EQ(device.GetLiveVoiceCountUVE(), 1U);
    device.ClearRecordedCallsUVE();

    entityManager.GetComponentUVE<Scene::AudioSourceComponentUVE>(entity).audioAssetPath = "playing-b.wav";
    audioSourceSystem.SyncUVE(entityManager, audioSystem);

    EXPECT_EQ(device.GetLiveVoiceCountUVE(), 1U);
    EXPECT_TRUE(AnyRecordedPlayCallUVE());
}

TEST_F(AudioSourceSystemUVETest, FailedReplacement_PreservesOldVoice) {
    RejectingClipResolverUVE resolver;
    AudioSystemUVE resolvingAudioSystem{device, &resolver};
    AudioSourceSystemUVE resolvingAudioSourceSystem;
    Scene::AudioSourceComponentUVE audioSource;
    audioSource.audioAssetPath = "accepted.wav";
    audioSource.playOnAwake = false;
    const Scene::EntityUVE entity = MakeAudioEntityUVE(audioSource);

    resolvingAudioSourceSystem.SyncUVE(entityManager, resolvingAudioSystem);
    ASSERT_EQ(device.GetLiveVoiceCountUVE(), 1U);
    device.ClearRecordedCallsUVE();

    entityManager.GetComponentUVE<Scene::AudioSourceComponentUVE>(entity).audioAssetPath = "rejected.wav";
    resolvingAudioSourceSystem.SyncUVE(entityManager, resolvingAudioSystem);

    EXPECT_EQ(device.GetLiveVoiceCountUVE(), 1U);
    EXPECT_TRUE(device.GetRecordedCallsUVE().empty());
}


} // namespace
} // namespace UVE::Audio::Tests
