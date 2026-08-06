// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/audio/audio_system_uve.h"

#include <algorithm>
#include <unordered_map>

#include "uve/debug/logging_macros_uve.h"

namespace UVE::Audio {

namespace {

struct SourceStateUVE {
    AudioSourceDescUVE desc;
    Math::Vector3UVE position{};
    float volume = 1.0F;
    float pitch = 1.0F;
};

} // namespace

struct AudioSystemUVE::ImplUVE {
    IAudioDeviceUVE& audioDevice;
    Math::Vector3UVE listenerPosition{};
    Math::Vector3UVE listenerForward{0.0F, 0.0F, -1.0F};
    Math::Vector3UVE listenerUp{0.0F, 1.0F, 0.0F};
    std::unordered_map<std::uint32_t, SourceStateUVE> sources;

    explicit ImplUVE(IAudioDeviceUVE& device) : audioDevice(device) {}
};

AudioSystemUVE::AudioSystemUVE(IAudioDeviceUVE& audioDevice) : m_impl(std::make_unique<ImplUVE>(audioDevice)) {}

AudioSystemUVE::~AudioSystemUVE() = default;

void AudioSystemUVE::SetListenerPositionUVE(Math::Vector3UVE position) {
    m_impl->listenerPosition = position;
}

void AudioSystemUVE::SetListenerOrientationUVE(Math::Vector3UVE forward, Math::Vector3UVE up) {
    m_impl->listenerForward = forward;
    m_impl->listenerUp = up;
}

Math::Vector3UVE AudioSystemUVE::GetListenerPositionUVE() const noexcept {
    return m_impl->listenerPosition;
}

VoiceHandleUVE AudioSystemUVE::CreateSourceUVE(const AudioSourceDescUVE& desc) {
    const VoiceHandleUVE voice = m_impl->audioDevice.CreateVoiceUVE(AudioVoiceDescUVE{desc.audioAssetPath, desc.looping});
    SourceStateUVE state;
    state.desc = desc;
    state.volume = desc.volume;
    state.pitch = desc.pitch;
    m_impl->sources.emplace(voice.value, std::move(state));
    return voice;
}

void AudioSystemUVE::DestroySourceUVE(VoiceHandleUVE source) {
    m_impl->sources.erase(source.value);
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
    iterator->second.position = position;
}

void AudioSystemUVE::SetSourceVolumeUVE(VoiceHandleUVE source, float volume) {
    const auto iterator = m_impl->sources.find(source.value);
    if (iterator == m_impl->sources.end()) {
        UVE_ERROR("AudioSystemUVE: SetSourceVolumeUVE called with an unknown handle ({})", source.value);
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
    iterator->second.pitch = pitch;
}

void AudioSystemUVE::UpdateUVE() {
    for (auto& [handleValue, state] : m_impl->sources) {
        float gain = 0.0F;
        if (!state.desc.spatial) {
            gain = std::clamp(state.volume, 0.0F, 1.0F);
        } else {
            const float distance = Math::LengthUVE(state.position - m_impl->listenerPosition);
            const float attenuation = ComputeDistanceAttenuationUVE(distance, state.desc.minDistance,
                                                                      state.desc.maxDistance, state.desc.attenuationModel);
            gain = std::clamp(state.volume * attenuation, 0.0F, 1.0F);
        }
        static_cast<void>(m_impl->audioDevice.SetVoiceParamsUVE(
            VoiceHandleUVE{handleValue}, AudioVoiceParamsUVE{state.position, gain, state.pitch}));
    }
}

} // namespace UVE::Audio
