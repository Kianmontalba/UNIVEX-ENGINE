//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include "uve/asset/asset_guid_uve.h"

namespace UVE::Scene {

/// Records which prefab asset an entity was instantiated from — internal infrastructure
/// PrefabSystemUVE owns, not one of the master spec's named built-in components (like
/// HierarchyComponentUVE/WorldTransformComponentUVE are SceneGraphUVE-owned infrastructure).
/// Added by PrefabSystemUVE::InstantiateUVE() to the instantiated root entity; preserved
/// through ordinary save/load like any other component (it never drives special re-
/// instantiation behavior during a plain SceneSerializerUVE load — only an explicit
/// InstantiateUVE() call actually instantiates a prefab).
struct PrefabInstanceComponentUVE final {
    Asset::AssetGuidUVE sourcePrefabGuid;
};

} // namespace UVE::Scene
