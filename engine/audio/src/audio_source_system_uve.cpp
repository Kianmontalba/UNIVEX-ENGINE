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

} // namespace

struct AudioSourceSystemUVE::ImplUVE {
    std::unordered_map<Scene::EntityUVE, VoiceHandleUVE> entityToVoice;
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

            auto iterator = m_impl->entityToVoice.find(entity);
            if (iterator == m_impl->entityToVoice.end()) {
                AudioSourceDescUVE desc;
                desc.audioAssetPath = audioSource.audioAssetPath;
                desc.looping = audioSource.looping;
                desc.volume = audioSource.volume;
                desc.pitch = audioSource.pitch;
                desc.spatial = audioSource.spatial;
                desc.minDistance = audioSource.minDistance;
                desc.maxDistance = audioSource.maxDistance;
                desc.attenuationModel = AttenuationModelOfUVE(audioSource.attenuationCurve);

                const VoiceHandleUVE voice = audioSystem.CreateSourceUVE(desc);
                iterator = m_impl->entityToVoice.emplace(entity, voice).first;
                if (audioSource.playOnAwake) {
                    static_cast<void>(audioSystem.PlayUVE(voice));
                }
            }

            audioSystem.SetSourcePositionUVE(iterator->second, worldTransform.worldPosition);
            audioSystem.SetSourceVolumeUVE(iterator->second, audioSource.volume);
            audioSystem.SetSourcePitchUVE(iterator->second, audioSource.pitch);
        });

    for (auto iterator = m_impl->entityToVoice.begin(); iterator != m_impl->entityToVoice.end();) {
        if (!seen.contains(iterator->first)) {
            audioSystem.DestroySourceUVE(iterator->second);
            iterator = m_impl->entityToVoice.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

} // namespace UVE::Audio
