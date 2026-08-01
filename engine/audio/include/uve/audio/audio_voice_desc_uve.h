//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <string>

#include "uve/math/vector3_uve.h"

namespace UVE::Audio {

/// Describes a voice to create via IAudioDeviceUVE::CreateVoiceUVE(). `audioAssetPath` is an
/// identity only — no decoded PCM/streaming data exists this increment (see
/// Scene::AudioSourceComponentUVE's own doc comment); a real backend would resolve this path to
/// real audio data itself.
struct AudioVoiceDescUVE {
    std::string audioAssetPath;
    bool looping = false;
};

/// Per-voice runtime parameters pushed via IAudioDeviceUVE::SetVoiceParamsUVE(). `gain` is a
/// final, already-attenuated linear multiplier in [0, 1] — AudioSystemUVE, not the device,
/// computes it; the device only ever receives the finished number.
struct AudioVoiceParamsUVE {
    Math::Vector3UVE position{};
    float gain = 1.0F;
    float pitch = 1.0F;
};

} // namespace UVE::Audio
