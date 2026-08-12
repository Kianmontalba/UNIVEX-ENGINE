// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

#include "uve/asset/uve_file_envelope_uve.h"
#include "uve/scene/entity_uve.h"
#include "uve/scene/i_entity_manager_uve.h"

namespace UVE::Scene {

/// The `assetType` field of the universal .uve* binary envelope (see ISceneSerializerUVE),
/// distinguishing a whole-scene save from a single-prefab-subtree save. The saved JSON payload
/// shape is identical either way — this is purely a file-type marker for tooling. An alias for
/// `Asset::AssetKindUVE` rather than its own enum: every `.uve*` file shares one global asset-
/// kind numbering (see `engine/asset/include/uve/asset/uve_file_envelope_uve.h`), so `Scene`/
/// `Prefab` can never collide with a value some other asset kind (e.g. `AssetBundleUVE`'s
/// `Bundle`) also uses.
using SceneAssetTypeUVE = Asset::AssetKindUVE;

/// An in-memory universal `.uve*` envelope containing one or more scene roots and every descendant.
/// It deliberately uses the same envelope, component registration, and file-local hierarchy-id rules
/// as SaveUVE()/LoadUVE(), allowing editor history to restore fresh entity handles without temporary
/// files. `assetType` is retained beside the bytes as a caller-visible intent check; RestoreUVE()
/// validates that it agrees with the embedded envelope header before mutating the entity manager.
struct SceneSnapshotUVE final {
    std::vector<std::byte> bytes;
    SceneAssetTypeUVE assetType = SceneAssetTypeUVE::Scene;
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

    /// Captures every entity reachable from `rootEntities` (each root plus all descendants) into
    /// a universal in-memory envelope. Returns std::nullopt without mutation if a root is invalid,
    /// a component is unregistered, or the requested asset type is not Scene/Prefab.
    [[nodiscard]] virtual std::optional<SceneSnapshotUVE> CaptureUVE(
        IEntityManagerUVE& entityManager, const std::vector<EntityUVE>& rootEntities,
        SceneAssetTypeUVE assetType) const = 0;

    /// Restores a valid snapshot by creating fresh live entities and remapping its file-local
    /// hierarchy ids. The returned entities are the snapshot roots in file order. On malformed or
    /// unsupported data it returns an empty vector and rolls back any entities created during the
    /// attempted restore.
    [[nodiscard]] virtual std::vector<EntityUVE> RestoreUVE(IEntityManagerUVE& entityManager,
                                                             const SceneSnapshotUVE& snapshot) const = 0;

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
