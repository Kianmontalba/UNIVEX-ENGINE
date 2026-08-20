// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>
#include <filesystem>

#include "uve/asset/i_asset_database_uve.h"
#include "uve/scene/entity_uve.h"
#include "uve/scene/i_entity_manager_uve.h"
#include "uve/scene/i_scene_graph_uve.h"

namespace UVE::Scene {

/// IPrefabSystemUVE is a thin layer over ISceneSerializerUVE providing the "reusable entity
/// template" workflow the master spec names (Part 7.3): save a subtree as a `.uveprefab`, then
/// instantiate independent copies of it later. Overrides are whole-component: an instantiated
/// entity is an ordinary live ECS entity, so any later AddComponentUVE/RemoveComponentUVE/
/// GetComponentUVE-mutation on it *is* the override — there is no separate override-tracking
/// data structure (see docs/CODING_STANDARDS.md/the Increment 6 plan for the rationale). Nested
/// prefabs "just work": an instance's PrefabInstanceComponentUVE is serialized/deserialized like
/// any other component, and is never re-resolved/re-instantiated during an ordinary load — only
/// an explicit InstantiateUVE() call actually instantiates a prefab asset.
/// Thread-safety: not thread-safe, matching IEntityManagerUVE's own main-thread-only contract.
class IPrefabSystemUVE {
public:
    virtual ~IPrefabSystemUVE() = default;

    /// Serializes `rootEntity` (and its descendants) to `path` as a reusable prefab template,
    /// registers `path` in `assetDatabase` (idempotent — re-saving the same path keeps its GUID
    /// stable) and immediately persists the registry, and returns that GUID. Returns
    /// `Asset::kInvalidAssetGuidUVE` if the underlying save fails (already logged).
    [[nodiscard]] virtual Asset::AssetGuidUVE SavePrefabUVE(IEntityManagerUVE& entityManager,
                                                             Asset::IAssetDatabaseUVE& assetDatabase,
                                                             EntityUVE rootEntity,
                                                             const std::filesystem::path& path) = 0;

    /// Instantiates the prefab identified by `prefabGuid` (resolved to a file path via
    /// `assetDatabase`) into fresh live entities, tags the new root with
    /// PrefabInstanceComponentUVE{prefabGuid}, and — if `parent != kInvalidEntityUVE` —
    /// reparents it under `parent` via `sceneGraph.SetParentUVE()`. Returns the new root entity,
    /// or `kInvalidEntityUVE` on failure (unknown GUID, or the underlying load fails — both
    /// logged).
    [[nodiscard]] virtual EntityUVE InstantiateUVE(IEntityManagerUVE& entityManager,
                                                    ISceneGraphUVE& sceneGraph,
                                                    Asset::IAssetDatabaseUVE& assetDatabase,
                                                    Asset::AssetGuidUVE prefabGuid,
                                                    EntityUVE parent) = 0;

    /// Instantiates and stamps a nonzero source revision into the new root's
    /// PrefabInstanceComponentUVE. The default implementation preserves legacy callers and
    /// delegates to InstantiateUVE without revision metadata; PrefabSystemUVE overrides it.
    [[nodiscard]] virtual EntityUVE InstantiateWithRevisionUVE(
        IEntityManagerUVE& entityManager, ISceneGraphUVE& sceneGraph,
        Asset::IAssetDatabaseUVE& assetDatabase, Asset::AssetGuidUVE prefabGuid, EntityUVE parent,
        std::uint64_t sourceRevision) {
        static_cast<void>(sourceRevision);
        return InstantiateUVE(entityManager, sceneGraph, assetDatabase, prefabGuid, parent);
    }

};

} // namespace UVE::Scene
