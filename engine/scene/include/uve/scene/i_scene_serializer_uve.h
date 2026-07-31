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
#include <filesystem>
#include <vector>

#include "uve/scene/entity_uve.h"
#include "uve/scene/i_entity_manager_uve.h"

namespace UVE::Scene {

/// The `assetType` field of the universal .uve* binary envelope (see ISceneSerializerUVE),
/// distinguishing a whole-scene save from a single-prefab-subtree save. The saved JSON payload
/// shape is identical either way — this is purely a file-type marker for tooling.
enum class SceneAssetTypeUVE : std::uint32_t {
    Scene = 1,
    Prefab = 2,
};

/// ISceneSerializerUVE saves/loads entity subtrees (an entity plus every descendant reachable
/// via HierarchyComponentUVE) to/from the universal `.uve*` binary envelope: magic `"UVE\0"`,
/// `version uint32`, `assetType uint32`, `compressionMethod uint32` (`0 = None` — the only value
/// implemented this increment), `payloadLength uint64`, then that many bytes of UTF-8 JSON. Used
/// directly for `.uvescene` (potentially many root entities) and, via PrefabSystemUVE, for
/// `.uveprefab` (always one root) — the payload format is identical either way.
/// `WorldTransformComponentUVE` is never serialized (it's derived/cached; SceneGraphUVE
/// recomputes it after load) and `HierarchyComponentUVE.parent` is written as a file-local id,
/// never a raw runtime `EntityUVE` (which is only meaningful within one process's session).
/// Thread-safety: not thread-safe, matching IEntityManagerUVE's own main-thread-only contract —
/// Save/Load both read/mutate the entity manager directly.
class ISceneSerializerUVE {
public:
    virtual ~ISceneSerializerUVE() = default;

    /// Serializes every entity reachable from `rootEntities` (each root plus all its
    /// descendants) to `path`. Returns false (logging the reason) on any failure — an
    /// unregistered component type, or a file-system write error — never partially writes a
    /// corrupt file (the whole payload is built in memory first).
    [[nodiscard]] virtual bool SaveUVE(IEntityManagerUVE& entityManager,
                                        const std::vector<EntityUVE>& rootEntities,
                                        const std::filesystem::path& path,
                                        SceneAssetTypeUVE assetType) = 0;

    /// Deserializes every entity from `path`, creating fresh live entities via `entityManager`.
    /// Returns the newly-created root entities, in file order, or an empty vector on any
    /// failure (bad magic, truncated file, unsupported version/asset-type/compression, JSON
    /// parse error, or an unknown component type — all logged).
    [[nodiscard]] virtual std::vector<EntityUVE> LoadUVE(IEntityManagerUVE& entityManager,
                                                          const std::filesystem::path& path) = 0;
};

} // namespace UVE::Scene
