// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <string>

#include "uve/audio/audio_attenuation_model_uve.h"
#include "uve/audio/audio_mixer_group_uve.h"

namespace UVE::Audio {

/// Describes a source to create via IAudioSystemUVE::CreateSourceUVE(). The system-level
/// counterpart to AudioVoiceDescUVE (which only carries what the RHI layer needs) — bundles
/// everything AudioSystemUVE's own attenuation/gain computation needs, matching this codebase's
/// Desc-struct convention (Render::BufferDescUVE, Physics::RaycastQueryUVE).
struct AudioSourceDescUVE {
    std::string audioAssetPath;
    std::string mixerGroup = std::string(kMasterAudioMixerGroupNameUVE);
    bool looping = false;
    float volume = 1.0F;
    float pitch = 1.0F;
    /// true = 3D positional (attenuated by distance from the listener via minDistance/
    /// maxDistance/attenuationModel); false = 2D (played at `volume` directly, no attenuation).
    bool spatial = true;
    float minDistance = 1.0F;
    float maxDistance = 25.0F;
    AudioAttenuationModelUVE attenuationModel = AudioAttenuationModelUVE::Linear;
};

} // namespace UVE::Audio
