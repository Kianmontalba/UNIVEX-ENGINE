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

} // namespace UVE::Scene
