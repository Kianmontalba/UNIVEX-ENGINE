//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <variant>

#include "uve/audio/audio_voice_desc_uve.h"
#include "uve/audio/voice_handle_uve.h"

namespace UVE::Audio {

/// One order-sensitive call recorded by NullAudioDeviceUVE — deliberately only the three calls a
/// real backend would need to receive in exact sequence to reproduce this voice's behavior
/// (Play/Stop/SetVoiceParams). CreateVoiceUVE/DestroyVoiceUVE are pure resource-lifecycle
/// bookkeeping (tested via NullAudioDeviceUVE::GetLiveVoiceCountUVE(), like
/// Render::NullRenderDeviceUVE's own Create/Destroy calls) and are deliberately NOT part of this
/// sequence — audio has no command-buffer/submission concept at all (a real backend issues
/// alSourcePlay/alSourcefv-style calls directly, not via a batched command list), so calls
/// accumulate for the device's whole lifetime instead of being grouped by a "submit" boundary.
struct PlayVoiceCallUVE {
    VoiceHandleUVE voice;
};
struct StopVoiceCallUVE {
    VoiceHandleUVE voice;
};
struct SetVoiceParamsCallUVE {
    VoiceHandleUVE voice;
    AudioVoiceParamsUVE params;
};

/// A single recorded IAudioDeviceUVE call, tagged by which method it came from.
using RecordedAudioCallUVE = std::variant<PlayVoiceCallUVE, StopVoiceCallUVE, SetVoiceParamsCallUVE>;

} // namespace UVE::Audio
