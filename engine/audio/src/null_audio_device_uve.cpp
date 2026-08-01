//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/audio/null_audio_device_uve.h"

#include <unordered_map>
#include <utility>

#include "uve/debug/logging_macros_uve.h"

namespace UVE::Audio {

struct NullAudioDeviceUVE::ImplUVE {
    std::unordered_map<std::uint32_t, VoicePlaybackStateUVE> voices;
    std::uint32_t nextVoiceHandle = 1;
    std::vector<RecordedAudioCallUVE> recordedCalls;
};

NullAudioDeviceUVE::NullAudioDeviceUVE() : m_impl(std::make_unique<ImplUVE>()) {}

NullAudioDeviceUVE::~NullAudioDeviceUVE() = default;

VoiceHandleUVE NullAudioDeviceUVE::CreateVoiceUVE(const AudioVoiceDescUVE& desc) {
    static_cast<void>(desc); // NullAudioDeviceUVE performs no real audio output, bookkeeping only.
    const std::uint32_t handleValue = m_impl->nextVoiceHandle++;
    m_impl->voices.emplace(handleValue, VoicePlaybackStateUVE::Stopped);
    return VoiceHandleUVE{handleValue};
}

void NullAudioDeviceUVE::DestroyVoiceUVE(VoiceHandleUVE voice) {
    if (m_impl->voices.erase(voice.value) == 0) {
        UVE_ERROR("NullAudioDeviceUVE: DestroyVoiceUVE called with an unknown or already-destroyed handle ({})",
                   voice.value);
    }
}

bool NullAudioDeviceUVE::PlayUVE(VoiceHandleUVE voice) {
    const auto iterator = m_impl->voices.find(voice.value);
    if (iterator == m_impl->voices.end()) {
        UVE_ERROR("NullAudioDeviceUVE: PlayUVE called with an unknown handle ({})", voice.value);
        return false;
    }
    iterator->second = VoicePlaybackStateUVE::Playing;
    m_impl->recordedCalls.emplace_back(PlayVoiceCallUVE{voice});
    return true;
}

bool NullAudioDeviceUVE::StopUVE(VoiceHandleUVE voice) {
    const auto iterator = m_impl->voices.find(voice.value);
    if (iterator == m_impl->voices.end()) {
        UVE_ERROR("NullAudioDeviceUVE: StopUVE called with an unknown handle ({})", voice.value);
        return false;
    }
    iterator->second = VoicePlaybackStateUVE::Stopped;
    m_impl->recordedCalls.emplace_back(StopVoiceCallUVE{voice});
    return true;
}

bool NullAudioDeviceUVE::SetVoiceParamsUVE(VoiceHandleUVE voice, const AudioVoiceParamsUVE& params) {
    if (!m_impl->voices.contains(voice.value)) {
        UVE_ERROR("NullAudioDeviceUVE: SetVoiceParamsUVE called with an unknown handle ({})", voice.value);
        return false;
    }
    m_impl->recordedCalls.emplace_back(SetVoiceParamsCallUVE{voice, params});
    return true;
}

VoicePlaybackStateUVE NullAudioDeviceUVE::GetVoiceStateUVE(VoiceHandleUVE voice) const {
    const auto iterator = m_impl->voices.find(voice.value);
    if (iterator == m_impl->voices.end()) {
        UVE_ERROR("NullAudioDeviceUVE: GetVoiceStateUVE called with an unknown handle ({})", voice.value);
        return VoicePlaybackStateUVE::Stopped;
    }
    return iterator->second;
}

std::string_view NullAudioDeviceUVE::GetBackendNameUVE() const noexcept {
    return "Null";
}

const std::vector<RecordedAudioCallUVE>& NullAudioDeviceUVE::GetRecordedCallsUVE() const noexcept {
    return m_impl->recordedCalls;
}

void NullAudioDeviceUVE::ClearRecordedCallsUVE() noexcept {
    m_impl->recordedCalls.clear();
}

std::size_t NullAudioDeviceUVE::GetLiveVoiceCountUVE() const noexcept {
    return m_impl->voices.size();
}

} // namespace UVE::Audio
