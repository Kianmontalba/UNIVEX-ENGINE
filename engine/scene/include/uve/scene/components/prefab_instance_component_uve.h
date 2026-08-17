// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


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

/// Validates the persisted source reference without resolving the prefab or reading the asset
/// database. A prefab-instance tag must identify a real source asset; nested instances preserve
/// this value and are still loaded as data rather than recursively re-instantiated.
[[nodiscard]] constexpr bool IsPrefabInstanceComponentValidUVE(
    const PrefabInstanceComponentUVE& component) noexcept {
    return component.sourcePrefabGuid != Asset::kInvalidAssetGuidUVE;
}

} // namespace UVE::Scene
