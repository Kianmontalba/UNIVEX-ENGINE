// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <vector>

#include "uve/audio/audio_mixer_group_uve.h"
#include "uve/audio/pcm_gain_effect_uve.h"
#include "uve/audio/wav_pcm16_decoder_uve.h"
#include "uve/audio/audio_source_desc_uve.h"
#include "uve/audio/voice_handle_uve.h"
#include "uve/audio/voice_playback_state_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Audio {

/// IAudioSystemUVE is the engine's top-level 3D positional audio orchestrator (the spec's
/// `AudioSystemUVE`, Part 7.6): owns the live source registry and the listener transform, and
/// drives IAudioDeviceUVE via computed (never raw) gain values. AudioSourceSystemUVE is the
/// ECS-facing caller; nothing below IAudioSystemUVE knows about entities or components.
/// Thread-safety: not thread-safe. Every method is intended to be called only from the main engine
/// thread, matching RenderSystemUVE's/InputSystemUVE's own single-threaded frame contract.
class IAudioSystemUVE {
public:
    virtual ~IAudioSystemUVE() = default;

    // --- Listener (the spec's AudioListenerUVE — "Attached to Camera3D by default"; see
    // EngineCoreUVE::LateUpdate() for how the active camera drives these each frame). ---
    virtual void SetListenerPositionUVE(Math::Vector3UVE position) = 0;
    virtual void SetListenerOrientationUVE(Math::Vector3UVE forward, Math::Vector3UVE up) = 0;
    [[nodiscard]] virtual Math::Vector3UVE GetListenerPositionUVE() const noexcept = 0;

    // --- Source lifecycle. ---
    /// Creates a source per `desc` (forwarding an AudioVoiceDescUVE subset to the underlying
    /// IAudioDeviceUVE). Never returns kInvalidVoiceHandleUVE on success.
    [[nodiscard]] virtual VoiceHandleUVE CreateSourceUVE(const AudioSourceDescUVE& desc) = 0;
    /// Destroys `source`. A handle already destroyed (or never valid) is a safe no-op (logged).
    virtual void DestroySourceUVE(VoiceHandleUVE source) = 0;

    /// Starts (or restarts) `source`. Returns false (logging the reason) if `source` is unknown.
    [[nodiscard]] virtual bool PlayUVE(VoiceHandleUVE source) = 0;
    /// Stops `source`. Returns false (logging the reason) if `source` is unknown.
    [[nodiscard]] virtual bool StopUVE(VoiceHandleUVE source) = 0;
    /// `source`'s current playback state; Stopped (logged) if `source` is unknown.
    [[nodiscard]] virtual VoicePlaybackStateUVE GetSourceStateUVE(VoiceHandleUVE source) const = 0;

    // --- Per-source runtime params: void + logged on an unknown handle (not [[nodiscard]] bool) —
    // these are expected to be called once per entity per frame from AudioSourceSystemUVE's ECS
    // sync, where forcing a bool check at every call site would be excessive ceremony. ---
    virtual void SetSourcePositionUVE(VoiceHandleUVE source, Math::Vector3UVE position) = 0;
    virtual void SetSourceVolumeUVE(VoiceHandleUVE source, float volume) = 0;
    virtual void SetSourcePitchUVE(VoiceHandleUVE source, float pitch) = 0;

    // --- Bounded mixer-group routing. ---
    virtual bool RegisterMixerGroupUVE(std::string_view name, float volumeMultiplier = 1.0F,
                                       float pitchMultiplier = 1.0F) = 0;
    virtual bool RemoveMixerGroupUVE(std::string_view name) = 0;
    virtual bool SetMixerGroupVolumeUVE(std::string_view name, float volumeMultiplier) = 0;
    virtual bool SetMixerGroupPitchUVE(std::string_view name, float pitchMultiplier) = 0;
    virtual bool SetSourceMixerGroupUVE(VoiceHandleUVE source, std::string_view name) = 0;
    [[nodiscard]] virtual AudioMixerDiagnosticsUVE GetMixerDiagnosticsUVE(
        std::size_t maximumGroups = kMaximumAudioMixerGroupsUVE) const = 0;

    // --- Caller-owned bounded stream/effect preparation. ---
    /// Resets one source's bounded PCM16 refill cursor. The source retains only cursor/scheduler
    /// state; decoded samples remain caller-owned and are never copied into AudioSystemUVE.
    [[nodiscard]] virtual bool ResetSourceStreamUVE(
        VoiceHandleUVE source, std::size_t totalSamples, bool loop, std::size_t cursorSample = 0U,
        std::size_t maximumSamples = kMaximumWavPcm16SamplesUVE) = 0;
    /// Plans and queues one bounded contiguous refill window, advancing only the source-owned cursor.
    [[nodiscard]] virtual bool ScheduleSourceStreamWindowUVE(VoiceHandleUVE source,
                                                               std::size_t requestedSamples) = 0;
    /// Pops the oldest queued refill plan for caller-owned decoding/publication.
    [[nodiscard]] virtual bool PopSourceStreamWindowUVE(VoiceHandleUVE source,
                                                         Pcm16StreamWindowPlanUVE& outPlan) = 0;
    /// Queues one bounded PCM gain window for the source's next caller-owned sample-buffer pass.
    [[nodiscard]] virtual bool ScheduleSourcePcmGainWindowUVE(
        VoiceHandleUVE source, const PcmGainEffectWindowUVE& window) = 0;
    /// Applies queued gain windows atomically to copied caller-owned samples and clears them only
    /// after successful output publication. No device submission is performed by this method.
    [[nodiscard]] virtual bool ApplySourcePcmGainEffectsUVE(
        VoiceHandleUVE source, const std::vector<float>& inputSamples,
        std::vector<float>& outputSamples) = 0;

    // --- Frame advance. ---
    /// Recomputes ComputeDistanceAttenuationUVE() for every live spatial source against the
    /// current listener position (non-spatial sources use `volume` directly) and pushes the
    /// resulting AudioVoiceParamsUVE to the underlying IAudioDeviceUVE via SetVoiceParamsUVE().
    /// Must be called once per frame, main-thread-only, after this frame's listener/source
    /// positions are settled.
    virtual void UpdateUVE() = 0;
};

} // namespace UVE::Audio
