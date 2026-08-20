// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/audio/audio_source_system_uve.h"

#include <unordered_map>
#include <unordered_set>

#include "uve/debug/assert_uve.h"
#include "uve/scene/components/audio_source_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Audio {

namespace {

/// The Audio-namespaced counterpart of a Scene-namespaced AudioSourceComponentUVE's attenuation
/// curve field — mirrors Physics::MaterialOfUVE()'s "component holds plain data, system converts
/// on demand" precedent, keeping engine/scene free of any engine/audio dependency.
[[nodiscard]] AudioAttenuationModelUVE AttenuationModelOfUVE(Scene::AudioAttenuationCurveUVE curve) noexcept {
    switch (curve) {
        case Scene::AudioAttenuationCurveUVE::InverseSquare:
            return AudioAttenuationModelUVE::InverseSquare;
        case Scene::AudioAttenuationCurveUVE::Linear:
        default:
            return AudioAttenuationModelUVE::Linear;
    }
}

[[nodiscard]] AudioSourceDescUVE MakeAudioSourceDescUVE(
    const Scene::AudioSourceComponentUVE& audioSource) {
    AudioSourceDescUVE desc;
    desc.audioAssetPath = audioSource.audioAssetPath;
    desc.looping = audioSource.looping;
    desc.volume = audioSource.volume;
    desc.pitch = audioSource.pitch;
    desc.spatial = audioSource.spatial;
    desc.minDistance = audioSource.minDistance;
    desc.maxDistance = audioSource.maxDistance;
    desc.attenuationModel = AttenuationModelOfUVE(audioSource.attenuationCurve);
    return desc;
}

[[nodiscard]] bool RequiresVoiceReplacementUVE(const AudioSourceDescUVE& previous,
                                                const AudioSourceDescUVE& current) noexcept {
    return previous.audioAssetPath != current.audioAssetPath || previous.looping != current.looping ||
           previous.spatial != current.spatial || previous.minDistance != current.minDistance ||
           previous.maxDistance != current.maxDistance || previous.attenuationModel != current.attenuationModel;
}

} // namespace

struct AudioSourceSystemUVE::ImplUVE {
    struct SourceStateUVE final {
        VoiceHandleUVE voice;
        AudioSourceDescUVE descriptor;
    };

    std::unordered_map<Scene::EntityUVE, SourceStateUVE> entityToVoice;
};

AudioSourceSystemUVE::AudioSourceSystemUVE() : m_impl(std::make_unique<ImplUVE>()) {}

AudioSourceSystemUVE::~AudioSourceSystemUVE() = default;

void AudioSourceSystemUVE::SyncUVE(Scene::IEntityManagerUVE& entityManager, IAudioSystemUVE& audioSystem) {
    std::unordered_set<Scene::EntityUVE> seen;

    entityManager.ForEachUVE<Scene::WorldTransformComponentUVE, Scene::AudioSourceComponentUVE>(
        [&](Scene::EntityUVE entity, const Scene::WorldTransformComponentUVE& worldTransform,
            const Scene::AudioSourceComponentUVE& audioSource) {
            UVE_ASSERT(Scene::IsAudioSourceComponentValidUVE(audioSource));
            seen.insert(entity);

            const AudioSourceDescUVE desiredDesc = MakeAudioSourceDescUVE(audioSource);
            auto iterator = m_impl->entityToVoice.find(entity);
            if (iterator == m_impl->entityToVoice.end()) {
                const VoiceHandleUVE voice = audioSystem.CreateSourceUVE(desiredDesc);
                if (voice == kInvalidVoiceHandleUVE) {
                    return;
                }
                iterator = m_impl->entityToVoice.emplace(
                    entity, ImplUVE::SourceStateUVE{voice, desiredDesc}).first;
                if (audioSource.playOnAwake) {
                    static_cast<void>(audioSystem.PlayUVE(voice));
                }
            } else {
                ImplUVE::SourceStateUVE& state = iterator->second;
                if (RequiresVoiceReplacementUVE(state.descriptor, desiredDesc)) {
                    const VoicePlaybackStateUVE previousPlaybackState = audioSystem.GetSourceStateUVE(state.voice);
                    const VoiceHandleUVE replacement = audioSystem.CreateSourceUVE(desiredDesc);
                    if (replacement != kInvalidVoiceHandleUVE) {
                        const bool wasPlaying = previousPlaybackState == VoicePlaybackStateUVE::Playing;
                        if (!wasPlaying || audioSystem.PlayUVE(replacement)) {
                            const VoiceHandleUVE previousVoice = state.voice;
                            state.voice = replacement;
                            state.descriptor = desiredDesc;
                            audioSystem.DestroySourceUVE(previousVoice);
                        } else {
                            audioSystem.DestroySourceUVE(replacement);
                        }
                    }
                }
            }

            audioSystem.SetSourcePositionUVE(iterator->second.voice, worldTransform.worldPosition);
            audioSystem.SetSourceVolumeUVE(iterator->second.voice, audioSource.volume);
            audioSystem.SetSourcePitchUVE(iterator->second.voice, audioSource.pitch);
        });

    for (auto iterator = m_impl->entityToVoice.begin(); iterator != m_impl->entityToVoice.end();) {
        if (!seen.contains(iterator->first)) {
            audioSystem.DestroySourceUVE(iterator->second.voice);
            iterator = m_impl->entityToVoice.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

} // namespace UVE::Audio
