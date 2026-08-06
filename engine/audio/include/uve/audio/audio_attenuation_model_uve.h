// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <algorithm>
#include <cstdint>

namespace UVE::Audio {

/// The distance-attenuation curve shape a spatial audio source uses (the spec's AudioSourceUVE —
/// "Spatial audio with distance attenuation curves"). Deliberately two curves this increment — the
/// two universally-understood basics (a straight linear falloff, and physically-motivated
/// inverse-square falloff); a full custom-curve/animation-curve system is future work, not
/// invented ahead of a real consumer needing it.
enum class AudioAttenuationModelUVE : std::uint8_t { Linear, InverseSquare };

/// Returns the linear gain multiplier in [0, 1] for a sound at `distance` from the listener.
/// `distance <= minDistance` always returns 1.0F (full volume). `distance >= maxDistance` always
/// returns 0.0F (inaudible) regardless of `model` — a hard cutoff, not a smoothed fade-out; note an
/// InverseSquare curve mathematically never reaches exactly zero on its own, so this cutoff is what
/// guarantees a source is fully silent past maxDistance for either model (an accepted, documented
/// simplification, not a bug).
/// - Linear:        `1 - (distance - minDistance) / (maxDistance - minDistance)`
/// - InverseSquare: `(minDistance / distance)^2`, clamped into [0, 1]
/// Precondition: `minDistance` must be > 0 (a real consumer must not construct a spatial source
/// with `minDistance == 0`, or InverseSquare divides by zero) — callers' responsibility to ensure,
/// matching Math::NormalizeUVE's "caller must already guarantee this" precedent; not itself
/// runtime-checked here.
[[nodiscard]] constexpr float ComputeDistanceAttenuationUVE(float distance, float minDistance, float maxDistance,
                                                              AudioAttenuationModelUVE model) noexcept {
    if (distance <= minDistance) {
        return 1.0F;
    }
    if (distance >= maxDistance) {
        return 0.0F;
    }
    if (model == AudioAttenuationModelUVE::Linear) {
        return 1.0F - (distance - minDistance) / (maxDistance - minDistance);
    }
    const float ratio = minDistance / distance;
    return std::clamp(ratio * ratio, 0.0F, 1.0F);
}

} // namespace UVE::Audio
