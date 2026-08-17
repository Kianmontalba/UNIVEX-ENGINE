// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cmath>
#include <cstdint>

#include "uve/math/vector3_uve.h"

namespace UVE::Scene {

/// Which lighting behavior a LightComponentUVE-bearing entity represents (Increment 25).
/// `range`/`spotAngleDegrees` are only meaningful for Point/Spot respectively — ignored
/// otherwise, following the same "always a full record, never branch on which fields are
/// meaningful" philosophy Render::LightDataUVE uses on the extraction side.
enum class LightTypeUVE : std::uint8_t { Directional = 0, Point = 1, Spot = 2 };

/// One of the master spec's named built-in components (Part 7.3). Originally a deliberately
/// minimal placeholder (Increment 5) — only color/intensity. Extended in Increment 25 with
/// `type`/`range`/`spotAngleDegrees` to support Point and Spot lights alongside Directional,
/// per Render::LightSystemUVE (Part 7.2). A light's *position*/*direction* are still not stored
/// here — they're derived from the entity's WorldTransformComponentUVE
/// (`worldPosition`/`worldRotation`), exactly as Increment 23 established for direction.
struct LightComponentUVE final {
    // color/intensity stay first (their original Increment-5 position) so every existing 2-arg
    // aggregate-init call site (LightComponentUVE{color, intensity}) keeps compiling unmodified,
    // matching the established backward-compatible-field-addition convention (see
    // ColliderComponentUVE's own later-added fields for the same trailing-default pattern).
    Math::Vector3UVE color{1.0F, 1.0F, 1.0F};
    float intensity = 1.0F;

    LightTypeUVE type = LightTypeUVE::Directional;

    /// Point/Spot only: the distance at which this light's contribution falls to (effectively)
    /// zero. Ignored for Directional (infinitely far away, no falloff).
    float range = 10.0F;

    /// Spot only: the cone's half-angle, in degrees, measured from the light's forward direction.
    /// Ignored for Directional/Point.
    float spotAngleDegrees = 45.0F;
};

[[nodiscard]] inline bool IsLightTypeValidUVE(const LightTypeUVE type) noexcept {
    return type == LightTypeUVE::Directional || type == LightTypeUVE::Point || type == LightTypeUVE::Spot;
}

/// Validates the value-only lighting component before persistence or renderer extraction. Color and
/// intensity permit HDR values but reject negatives/non-finite data; range and cone angle remain
/// finite positive policy values even when a given light type ignores one of them.
[[nodiscard]] inline bool IsLightComponentValidUVE(const LightComponentUVE& light) noexcept {
    return std::isfinite(light.color.x) && std::isfinite(light.color.y) && std::isfinite(light.color.z) &&
           light.color.x >= 0.0F && light.color.y >= 0.0F && light.color.z >= 0.0F &&
           std::isfinite(light.intensity) && light.intensity >= 0.0F && IsLightTypeValidUVE(light.type) &&
           std::isfinite(light.range) && light.range > 0.0F && std::isfinite(light.spotAngleDegrees) &&
           light.spotAngleDegrees > 0.0F && light.spotAngleDegrees < 180.0F;
}

} // namespace UVE::Scene
