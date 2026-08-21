// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/audio/audio_system_uve.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "uve/debug/logging_macros_uve.h"
#include "uve/audio/audio_listener_orientation_validation_uve.h"

namespace UVE::Audio {

namespace {

struct SourceStateUVE {
    AudioSourceDescUVE desc;
    std::string mixerGroup = std::string(kMasterAudioMixerGroupNameUVE);
    Math::Vector3UVE position{};
    float volume = 1.0F;
    float pitch = 1.0F;
};

} // namespace

struct AudioSystemUVE::ImplUVE {
    IAudioDeviceUVE& audioDevice;
    IAudioClipResolverUVE* clipResolver = nullptr;
    Math::Vector3UVE listenerPosition{};
    Math::Vector3UVE listenerForward{0.0F, 0.0F, -1.0F};
    Math::Vector3UVE listenerUp{0.0F, 1.0F, 0.0F};
    std::unordered_map<std::uint32_t, SourceStateUVE> sources;
    AudioMixerGroupUVE mixerGroups;

    explicit ImplUVE(IAudioDeviceUVE& device, IAudioClipResolverUVE* resolver)
        : audioDevice(device), clipResolver(resolver) {}
};

AudioSystemUVE::AudioSystemUVE(IAudioDeviceUVE& audioDevice, IAudioClipResolverUVE* clipResolver)
    : m_impl(std::make_unique<ImplUVE>(audioDevice, clipResolver)) {}

AudioSystemUVE::~AudioSystemUVE() = default;

void AudioSystemUVE::SetListenerPositionUVE(Math::Vector3UVE position) {
    if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z)) {
        UVE_ERROR("AudioSystemUVE: SetListenerPositionUVE rejected non-finite position");
        return;
    }
    m_impl->listenerPosition = position;
}

void AudioSystemUVE::SetListenerOrientationUVE(Math::Vector3UVE forward, Math::Vector3UVE up) {
    if (!IsAudioListenerOrientationValidUVE(forward, up)) {
        UVE_ERROR("AudioSystemUVE: SetListenerOrientationUVE rejected invalid orientation vectors");
        return;
    }
    m_impl->listenerForward = forward;
    m_impl->listenerUp = up;
}

Math::Vector3UVE AudioSystemUVE::GetListenerPositionUVE() const noexcept {
    return m_impl->listenerPosition;
}

VoiceHandleUVE AudioSystemUVE::CreateSourceUVE(const AudioSourceDescUVE& desc) {
    if (!IsAudioSourceDescValidUVE(desc)) {
        UVE_ERROR("AudioSystemUVE: CreateSourceUVE rejected invalid source descriptor");
        return kInvalidVoiceHandleUVE;
    }
    std::string resolvedAudioAssetPath = desc.audioAssetPath;
    if (m_impl->clipResolver != nullptr) {
        const AudioClipResolutionUVE resolution = m_impl->clipResolver->ResolveAudioClipUVE(desc.audioAssetPath);
        if (!resolution.accepted) {
            UVE_ERROR("AudioSystemUVE: audio clip resolution rejected '{}': {}", desc.audioAssetPath,
                      resolution.diagnostic);
            return kInvalidVoiceHandleUVE;
        }
        resolvedAudioAssetPath = resolution.resolvedPath;
    }

    const VoiceHandleUVE voice =
        m_impl->audioDevice.CreateVoiceUVE(AudioVoiceDescUVE{resolvedAudioAssetPath, desc.looping});
    if (voice == kInvalidVoiceHandleUVE) {
        UVE_ERROR("AudioSystemUVE: audio device returned an invalid voice handle for '{}'",
                  resolvedAudioAssetPath);
        return kInvalidVoiceHandleUVE;
    }
    if (m_impl->sources.contains(voice.value)) {
        UVE_ERROR("AudioSystemUVE: audio device returned a duplicate voice handle ({})", voice.value);
        return kInvalidVoiceHandleUVE;
    }
    SourceStateUVE state;
    state.desc = desc;
    state.desc.audioAssetPath = resolvedAudioAssetPath;
    state.mixerGroup = m_impl->mixerGroups.ResolveGroupNameUVE(desc.mixerGroup);
    state.desc.mixerGroup = state.mixerGroup;
    state.volume = desc.volume;
    state.pitch = desc.pitch;
    static_cast<void>(m_impl->mixerGroups.AttachSourceUVE(state.mixerGroup));
    m_impl->sources.emplace(voice.value, std::move(state));
    return voice;
}

void AudioSystemUVE::DestroySourceUVE(VoiceHandleUVE source) {
    const auto iterator = m_impl->sources.find(source.value);
    if (iterator != m_impl->sources.end()) {
        static_cast<void>(m_impl->mixerGroups.DetachSourceUVE(iterator->second.mixerGroup));
        m_impl->sources.erase(iterator);
    }
    m_impl->audioDevice.DestroyVoiceUVE(source);
}

bool AudioSystemUVE::PlayUVE(VoiceHandleUVE source) {
    return m_impl->audioDevice.PlayUVE(source);
}

bool AudioSystemUVE::StopUVE(VoiceHandleUVE source) {
    return m_impl->audioDevice.StopUVE(source);
}

VoicePlaybackStateUVE AudioSystemUVE::GetSourceStateUVE(VoiceHandleUVE source) const {
    return m_impl->audioDevice.GetVoiceStateUVE(source);
}

void AudioSystemUVE::SetSourcePositionUVE(VoiceHandleUVE source, Math::Vector3UVE position) {
    const auto iterator = m_impl->sources.find(source.value);
    if (iterator == m_impl->sources.end()) {
        UVE_ERROR("AudioSystemUVE: SetSourcePositionUVE called with an unknown handle ({})", source.value);
        return;
    }
    if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z)) {
        UVE_ERROR("AudioSystemUVE: SetSourcePositionUVE rejected non-finite position for handle ({})", source.value);
        return;
    }
    iterator->second.position = position;
}

void AudioSystemUVE::SetSourceVolumeUVE(VoiceHandleUVE source, float volume) {
    const auto iterator = m_impl->sources.find(source.value);
    if (iterator == m_impl->sources.end()) {
        UVE_ERROR("AudioSystemUVE: SetSourceVolumeUVE called with an unknown handle ({})", source.value);
        return;
    }
    if (!std::isfinite(volume) || volume < 0.0F) {
        UVE_ERROR("AudioSystemUVE: SetSourceVolumeUVE rejected invalid volume for handle ({})", source.value);
        return;
    }
    iterator->second.volume = volume;
}

void AudioSystemUVE::SetSourcePitchUVE(VoiceHandleUVE source, float pitch) {
    const auto iterator = m_impl->sources.find(source.value);
    if (iterator == m_impl->sources.end()) {
        UVE_ERROR("AudioSystemUVE: SetSourcePitchUVE called with an unknown handle ({})", source.value);
        return;
    }
    if (!std::isfinite(pitch) || pitch < 0.0F) {
        UVE_ERROR("AudioSystemUVE: SetSourcePitchUVE rejected invalid pitch for handle ({})", source.value);
        return;
    }
    iterator->second.pitch = pitch;
}

bool AudioSystemUVE::RegisterMixerGroupUVE(const std::string_view name, const float volumeMultiplier,
                                            const float pitchMultiplier) {
    return m_impl->mixerGroups.RegisterGroupUVE(name, volumeMultiplier, pitchMultiplier);
}

bool AudioSystemUVE::RemoveMixerGroupUVE(const std::string_view name) {
    return m_impl->mixerGroups.RemoveGroupUVE(name);
}

bool AudioSystemUVE::SetMixerGroupVolumeUVE(const std::string_view name, const float volumeMultiplier) {
    return m_impl->mixerGroups.SetGroupVolumeUVE(name, volumeMultiplier);
}

bool AudioSystemUVE::SetMixerGroupPitchUVE(const std::string_view name, const float pitchMultiplier) {
    return m_impl->mixerGroups.SetGroupPitchUVE(name, pitchMultiplier);
}

bool AudioSystemUVE::SetSourceMixerGroupUVE(const VoiceHandleUVE source, const std::string_view name) {
    const auto iterator = m_impl->sources.find(source.value);
    if (iterator == m_impl->sources.end() || !m_impl->mixerGroups.HasGroupUVE(name)) {
        return false;
    }
    const std::string resolvedName(name);
    if (iterator->second.mixerGroup == resolvedName) {
        return true;
    }
    if (!m_impl->mixerGroups.AttachSourceUVE(resolvedName)) {
        return false;
    }
    static_cast<void>(m_impl->mixerGroups.DetachSourceUVE(iterator->second.mixerGroup));
    iterator->second.mixerGroup = resolvedName;
    iterator->second.desc.mixerGroup = resolvedName;
    return true;
}

AudioMixerDiagnosticsUVE AudioSystemUVE::GetMixerDiagnosticsUVE(const std::size_t maximumGroups) const {
    return m_impl->mixerGroups.GetDiagnosticsUVE(maximumGroups);
}

void AudioSystemUVE::UpdateUVE() {
    for (auto& [handleValue, state] : m_impl->sources) {
        const float groupVolume = m_impl->mixerGroups.GetGroupVolumeUVE(state.mixerGroup);
        const float groupPitch = m_impl->mixerGroups.GetGroupPitchUVE(state.mixerGroup);
        float attenuation = 1.0F;
        if (state.desc.spatial) {
            const Math::Vector3UVE listenerDelta = state.position - m_impl->listenerPosition;
            const float distanceSquared = Math::LengthSquaredUVE(listenerDelta);
            if (!std::isfinite(distanceSquared) || distanceSquared < 0.0F) {
                attenuation = 0.0F;
            } else {
                const float distance = std::sqrt(distanceSquared);
                attenuation = std::isfinite(distance)
                    ? ComputeDistanceAttenuationUVE(distance, state.desc.minDistance,
                                                     state.desc.maxDistance, state.desc.attenuationModel)
                    : 0.0F;
            }
        }
        AudioMixParametersUVE parameters;
        if (!EvaluateAudioMixParametersUVE(state.volume, state.pitch, groupVolume, groupPitch, attenuation, parameters)) {
            UVE_ERROR("AudioSystemUVE: invalid mix parameters for source {}", handleValue);
            parameters = AudioMixParametersUVE{0.0F, 1.0F};
        }
        static_cast<void>(m_impl->audioDevice.SetVoiceParamsUVE(
            VoiceHandleUVE{handleValue}, AudioVoiceParamsUVE{state.position, parameters.gain, parameters.pitch}));
    }
}

} // namespace UVE::Audio
