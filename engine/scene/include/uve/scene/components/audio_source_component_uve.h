// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

namespace UVE::Scene {

inline constexpr std::size_t kMaximumAudioAssetPathBytesUVE = 1024U;
inline constexpr std::size_t kMaximumAudioMixerGroupNameBytesUVE = 64U;

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
    /// Empty preserves the legacy/default runtime Master route; non-empty names are resolved by AudioSystemUVE.
    std::string mixerGroup;
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

[[nodiscard]] inline bool IsAudioAttenuationCurveValidUVE(const AudioAttenuationCurveUVE curve) noexcept {
    return curve == AudioAttenuationCurveUVE::Linear || curve == AudioAttenuationCurveUVE::InverseSquare;
}

/// Validates source parameters before persistence and audio-source synchronization. An empty path
/// remains valid as the existing default/no-clip source state; a future asset resolver owns path
/// existence. Distance ordering is required only for spatial sources because 2D sources ignore it.
[[nodiscard]] inline bool IsAudioSourceComponentValidUVE(const AudioSourceComponentUVE& source) noexcept {
    const bool spatialDistanceValid =
        !source.spatial || (std::isfinite(source.minDistance) && source.minDistance > 0.0F &&
                            std::isfinite(source.maxDistance) && source.maxDistance > source.minDistance);
    const bool assetPathValid = source.audioAssetPath.size() <= kMaximumAudioAssetPathBytesUVE &&
                                source.audioAssetPath.find('\0') == std::string::npos;
    const bool mixerGroupValid = source.mixerGroup.size() <= kMaximumAudioMixerGroupNameBytesUVE &&
                                 source.mixerGroup.find('\0') == std::string::npos;
    return assetPathValid && mixerGroupValid && std::isfinite(source.volume) && source.volume >= 0.0F &&
           std::isfinite(source.pitch) && source.pitch > 0.0F &&
           IsAudioAttenuationCurveValidUVE(source.attenuationCurve) && spatialDistanceValid;
}

} // namespace UVE::Scene
