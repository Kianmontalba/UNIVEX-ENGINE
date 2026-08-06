// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/audio/audio_source_system_uve.h"

#include <algorithm>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include "uve/audio/audio_system_uve.h"
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

TEST_F(AudioSourceSystemUVETest, PlayOnAwakeEntity_CreatesAndPlaysSourceOnFirstSync) {
    Scene::AudioSourceComponentUVE audioSource;
    audioSource.playOnAwake = true;
    MakeAudioEntityUVE(audioSource);

    audioSourceSystem.SyncUVE(entityManager, audioSystem);

    EXPECT_EQ(device.GetLiveVoiceCountUVE(), 1U);
    EXPECT_TRUE(AnyRecordedPlayCallUVE());
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

} // namespace
} // namespace UVE::Audio::Tests
