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
/// placeholder data: no AssetManagerUVE/asset-handle type exists yet (that's Part 7.4, Asset
/// Pipeline), so a path string is the smallest honest reference to a mesh asset. Will be
/// extended (or migrated to a real asset handle) once RenderSystemUVE/MeshRendererUVE (Part
/// 7.2) exist to consume it.
struct MeshComponentUVE final {
    std::string meshAssetPath;
};

} // namespace UVE::Scene
