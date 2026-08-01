//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

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

    void UpdateUVE() override;

private:
    struct ImplUVE;
    std::unique_ptr<ImplUVE> m_impl;
};

} // namespace UVE::Audio
