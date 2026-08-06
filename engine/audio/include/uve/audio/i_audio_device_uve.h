// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <string_view>

#include "uve/audio/audio_voice_desc_uve.h"
#include "uve/audio/voice_handle_uve.h"
#include "uve/audio/voice_playback_state_uve.h"

namespace UVE::Audio {

/// IAudioDeviceUVE is the engine's backend-agnostic audio hardware interface (the spec's
/// AudioSystemUVE's "Built-in: OpenAL-soft backend"), mirroring Render::IRenderDeviceUVE's shape:
/// a small, explicit resource-handle RHI a real OpenAL-soft (or platform-native) backend would
/// implement directly. The only implementation this sandbox can build and test is
/// NullAudioDeviceUVE (engine/audio) — no audio SDK headers or sound hardware exist in this
/// environment (see docs/CODING_STANDARDS.md); a real backend is future work once it does. Every
/// future backend implements exactly this interface, so nothing above the RHI (AudioSystemUVE,
/// AudioSourceSystemUVE) needs to change when one arrives.
/// Thread-safety: implementation-defined; NullAudioDeviceUVE documents its own contract. Callers
/// should assume an audio device is only safe to use from the main engine/audio thread unless a
/// concrete implementation states otherwise.
class IAudioDeviceUVE {
public:
    virtual ~IAudioDeviceUVE() = default;

    /// Creates a voice per `desc`. Never returns kInvalidVoiceHandleUVE on success.
    [[nodiscard]] virtual VoiceHandleUVE CreateVoiceUVE(const AudioVoiceDescUVE& desc) = 0;

    /// Destroys `voice`. A handle already destroyed (or never valid) is a safe no-op (logged).
    virtual void DestroyVoiceUVE(VoiceHandleUVE voice) = 0;

    /// Starts (or restarts, if already playing) `voice`. Returns false (logging the reason) if
    /// `voice` is unknown.
    [[nodiscard]] virtual bool PlayUVE(VoiceHandleUVE voice) = 0;

    /// Stops `voice`. Returns false (logging the reason) if `voice` is unknown.
    [[nodiscard]] virtual bool StopUVE(VoiceHandleUVE voice) = 0;

    /// Overwrites `voice`'s runtime position/gain/pitch. Returns false (logging the reason) if
    /// `voice` is unknown.
    [[nodiscard]] virtual bool SetVoiceParamsUVE(VoiceHandleUVE voice, const AudioVoiceParamsUVE& params) = 0;

    /// `voice`'s current playback state. Returns VoicePlaybackStateUVE::Stopped (logging the
    /// reason) if `voice` is unknown — a query that may legitimately miss (the voice could have
    /// just been destroyed by another system this same frame), not a programmer-precondition
    /// violation, matching IConfigManagerUVE's "keyed lookup that may legitimately miss" precedent.
    [[nodiscard]] virtual VoicePlaybackStateUVE GetVoiceStateUVE(VoiceHandleUVE voice) const = 0;

    /// A short human-readable backend name (e.g. `"Null"`), for logging/diagnostics.
    [[nodiscard]] virtual std::string_view GetBackendNameUVE() const noexcept = 0;
};

} // namespace UVE::Audio
