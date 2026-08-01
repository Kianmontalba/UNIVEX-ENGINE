//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "uve/audio/i_audio_device_uve.h"
#include "uve/audio/recorded_audio_call_uve.h"

namespace UVE::Audio {

/// NullAudioDeviceUVE is the only IAudioDeviceUVE backend this sandbox can build and test: it
/// performs zero real audio output — there is no OpenAL-soft/platform audio SDK or sound hardware
/// available here — and instead validates and bookkeeps every call, recording the exact sequence
/// of order-sensitive calls (Play/Stop/SetVoiceParams) a real backend would have received.
/// Existing solely so AudioSystemUVE/AudioSourceSystemUVE can be built and unit-tested against a
/// real IAudioDeviceUVE& today; a genuine OpenAL-soft backend is future work once this environment
/// has the SDK and audio hardware it currently lacks.
/// Thread-safety: not thread-safe. Every method is intended to be called only from the main
/// engine/audio thread, matching AudioSystemUVE's own single-threaded frame contract.
class NullAudioDeviceUVE final : public IAudioDeviceUVE {
public:
    NullAudioDeviceUVE();
    ~NullAudioDeviceUVE() override;

    NullAudioDeviceUVE(const NullAudioDeviceUVE&) = delete;
    NullAudioDeviceUVE& operator=(const NullAudioDeviceUVE&) = delete;

    [[nodiscard]] VoiceHandleUVE CreateVoiceUVE(const AudioVoiceDescUVE& desc) override;
    void DestroyVoiceUVE(VoiceHandleUVE voice) override;
    [[nodiscard]] bool PlayUVE(VoiceHandleUVE voice) override;
    [[nodiscard]] bool StopUVE(VoiceHandleUVE voice) override;
    [[nodiscard]] bool SetVoiceParamsUVE(VoiceHandleUVE voice, const AudioVoiceParamsUVE& params) override;
    [[nodiscard]] VoicePlaybackStateUVE GetVoiceStateUVE(VoiceHandleUVE voice) const override;
    [[nodiscard]] std::string_view GetBackendNameUVE() const noexcept override;

    /// Test-only hook (not part of IAudioDeviceUVE): every Play/Stop/SetVoiceParams call this
    /// device has recorded, in call order, since construction (or the last
    /// ClearRecordedCallsUVE()). There is no command-buffer-style "submit" boundary for audio —
    /// calls accumulate for the device's whole lifetime.
    [[nodiscard]] const std::vector<RecordedAudioCallUVE>& GetRecordedCallsUVE() const noexcept;

    /// Test-only hook: clears the recorded-call log without touching live-voice bookkeeping — lets
    /// a test isolate assertions to calls made after a known point.
    void ClearRecordedCallsUVE() noexcept;

    /// Test-only hook: how many voices are currently alive (created but not yet destroyed).
    [[nodiscard]] std::size_t GetLiveVoiceCountUVE() const noexcept;

private:
    struct ImplUVE;
    std::unique_ptr<ImplUVE> m_impl;
};

} // namespace UVE::Audio
