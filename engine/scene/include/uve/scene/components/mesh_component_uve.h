// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/asset/asset_guid_uve.h"

namespace UVE::Scene {

/// One of the master spec's named built-in components (Part 7.3). References the mesh and
/// material assets to draw, by GUID — resolved and loaded on demand by MeshRendererUVE
/// (Part 7.2) via AssetManagerUVE/AssetDatabaseUVE, matching PrefabInstanceComponentUVE's
/// existing AssetGuidUVE-holding precedent. kInvalidAssetGuidUVE means "no mesh/material
/// assigned" — MeshRendererUVE skips such entities.
struct MeshComponentUVE final {
    Asset::AssetGuidUVE meshGuid;
    Asset::AssetGuidUVE materialGuid;
};

/// Validates the authored asset-reference pair without resolving assets. Both invalid GUIDs are
/// the established unassigned state; a single invalid GUID would otherwise create a permanently
/// incomplete render reference and is rejected at persistence/runtime boundaries.
[[nodiscard]] constexpr bool IsMeshComponentValidUVE(const MeshComponentUVE& component) noexcept {
    const bool meshUnassigned = component.meshGuid == Asset::kInvalidAssetGuidUVE;
    const bool materialUnassigned = component.materialGuid == Asset::kInvalidAssetGuidUVE;
    return meshUnassigned == materialUnassigned;
}

} // namespace UVE::Scene
