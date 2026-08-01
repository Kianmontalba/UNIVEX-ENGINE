//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <string>

namespace UVE::Scene {

/// Distance-attenuation curve shape a spatial AudioSourceComponentUVE uses, consumed by
/// Audio::AudioSourceSystemUVE (which converts it to Audio::AudioAttenuationModelUVE) — kept as a
/// Scene-local enum, not the UVE::Audio type itself, so engine/scene never depends on
/// engine/audio. Mirrors Physics::PhysicsMaterialUVE/MaterialOfUVE's precedent
/// (docs/CODING_STANDARDS.md, Physics section, which names engine/audio explicitly as a future
/// user of this exact pattern).
enum class AudioAttenuationCurveUVE : std::uint8_t { Linear, InverseSquare };

/// One of the master spec's named built-in components (Part 7.3), extended in Part 7.6's
/// AudioSystemUVE (Increment 18) exactly as this component's own original doc comment predicted.
/// `audioAssetPath` remains a path-based identity only — no AudioClipUVE/decoded-audio asset type
/// exists yet (a future Part 7.4 extension); AudioSourceSystemUVE only needs to know *which*
/// sound this entity refers to, not its decoded samples, to drive IAudioSystemUVE's
/// position/gain bookkeeping this increment. No runtime "isPlaying" field is stored here —
/// AudioSourceSystemUVE tracks the live entity->voice mapping itself, and playback state is
/// queryable via IAudioSystemUVE::GetSourceStateUVE(), matching WorldTransformComponentUVE's own
/// precedent of keeping derived/runtime state out of the authored component.
struct AudioSourceComponentUVE final {
    std::string audioAssetPath;
    float volume = 1.0F;
    bool looping = false;
    float pitch = 1.0F;
    /// true = 3D positional; false = 2D (played at `volume` directly, no attenuation) — a binary
    /// choice this increment, not a continuous 0..1 spatial blend.
    bool spatial = true;
    float minDistance = 1.0F;
    float maxDistance = 25.0F;
    AudioAttenuationCurveUVE attenuationCurve = AudioAttenuationCurveUVE::Linear;
    /// Whether AudioSourceSystemUVE calls IAudioSystemUVE::PlayUVE() the first time it sees this
    /// entity. No scripting/event trigger consumes AudioSourceComponentUVE yet — playOnAwake is
    /// the only way an ECS-authored sound can start without one.
    bool playOnAwake = true;
};

} // namespace UVE::Scene
