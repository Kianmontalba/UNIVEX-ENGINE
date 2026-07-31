//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include "uve/math/vector3_uve.h"

namespace UVE::Scene {

/// One of the master spec's named built-in components (Part 7.3). Deliberately minimal
/// placeholder data — only the two fields universally implied by "a light" (color, intensity).
/// Will be extended once LightSystemUVE (Part 7.2) exists to define light types, shadow
/// settings, culling, etc.
struct LightComponentUVE final {
    Math::Vector3UVE color{1.0F, 1.0F, 1.0F};
    float intensity = 1.0F;
};

} // namespace UVE::Scene
