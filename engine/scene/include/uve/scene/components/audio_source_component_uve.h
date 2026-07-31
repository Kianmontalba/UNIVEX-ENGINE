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

namespace UVE::Scene {

/// One of the master spec's named built-in components (Part 7.3). Deliberately minimal
/// placeholder data: a path-based asset reference (no AssetManagerUVE/asset-handle type exists
/// yet — Part 7.4) plus volume. Will be extended once an audio system (Part 7.6) exists to
/// consume it.
struct AudioSourceComponentUVE final {
    std::string audioAssetPath;
    float volume = 1.0F;
};

} // namespace UVE::Scene
