// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <memory>

#include "uve/audio/i_audio_device_uve.h"
#include "uve/audio/i_audio_system_uve.h"

namespace UVE::Audio {

/// AudioSystemUVE is the concrete, engine-standard implementation of IAudioSystemUVE. Takes its
/// IAudioDeviceUVE& by dependency injection (same shape as Render::RenderSystemUVE's
/// IRenderDeviceUVE&, Input::InputSystemUVE's IEventSystemUVE&) — it never constructs or owns an
/// audio device itself, so tests and future backend swaps can pass in whichever IAudioDeviceUVE&
/// they need.
class AudioSystemUVE final : public IAudioSystemUVE {
public:
    explicit AudioSystemUVE(IAudioDeviceUVE& audioDevice);
    ~AudioSystemUVE() override;

    AudioSystemUVE(const AudioSystemUVE&) = delete;
    AudioSystemUVE& operator=(const AudioSystemUVE&) = delete;

    void SetListenerPositionUVE(Math::Vector3UVE position) override;
    void SetListenerOrientationUVE(Math::Vector3UVE forward, Math::Vector3UVE up) override;
    [[nodiscard]] Math::Vector3UVE GetListenerPositionUVE() const noexcept override;

    [[nodiscard]] VoiceHandleUVE CreateSourceUVE(const AudioSourceDescUVE& desc) override;
    void DestroySourceUVE(VoiceHandleUVE source) override;
    [[nodiscard]] bool PlayUVE(VoiceHandleUVE source) override;
    [[nodiscard]] bool StopUVE(VoiceHandleUVE source) override;
    [[nodiscard]] VoicePlaybackStateUVE GetSourceStateUVE(VoiceHandleUVE source) const override;

    void SetSourcePositionUVE(VoiceHandleUVE source, Math::Vector3UVE position) override;
    void SetSourceVolumeUVE(VoiceHandleUVE source, float volume) override;
    void SetSourcePitchUVE(VoiceHandleUVE source, float pitch) override;

    bool RegisterMixerGroupUVE(std::string_view name, float volumeMultiplier = 1.0F,
                               float pitchMultiplier = 1.0F) override;
    bool RemoveMixerGroupUVE(std::string_view name) override;
    bool SetMixerGroupVolumeUVE(std::string_view name, float volumeMultiplier) override;
    bool SetMixerGroupPitchUVE(std::string_view name, float pitchMultiplier) override;
    bool SetSourceMixerGroupUVE(VoiceHandleUVE source, std::string_view name) override;
    [[nodiscard]] AudioMixerDiagnosticsUVE GetMixerDiagnosticsUVE(
        std::size_t maximumGroups = kMaximumAudioMixerGroupsUVE) const override;

    void UpdateUVE() override;

private:
    struct ImplUVE;
    std::unique_ptr<ImplUVE> m_impl;
};

} // namespace UVE::Audio
