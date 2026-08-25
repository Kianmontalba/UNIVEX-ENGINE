// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/scene/prefab_system_uve.h"

#include <exception>
#include <optional>
#include <vector>

#include "uve/asset/asset_content_fingerprint_uve.h"

#include "uve/debug/logging_macros_uve.h"
#include "uve/scene/components/hierarchy_component_uve.h"
#include "uve/scene/components/prefab_instance_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"
#include "uve/scene/prefab_revision_policy_uve.h"

namespace UVE::Scene {
namespace {

void DestroySubtreeUVE(IEntityManagerUVE& entityManager, ISceneGraphUVE& sceneGraph, const EntityUVE root) {
    if (!entityManager.IsAliveUVE(root)) {
        return;
    }
    const std::vector<EntityUVE> children = sceneGraph.GetChildrenUVE(entityManager, root);
    for (const EntityUVE child : children) {
        DestroySubtreeUVE(entityManager, sceneGraph, child);
    }
    entityManager.DestroyEntityUVE(root);
}

} // namespace

std::optional<std::uint64_t> ComputePrefabSourceRevisionUVE(const std::filesystem::path& path) {
    const std::optional<Asset::AssetContentFingerprintUVE> fingerprint =
        Asset::ComputeAssetContentFingerprintUVE(path);
    if (!fingerprint.has_value() || fingerprint->byteCount == 0U) {
        return std::nullopt;
    }
    std::uint64_t revision = fingerprint->hash;
    if (revision == 0U) {
        revision = fingerprint->byteCount;
    }
    return revision == 0U ? std::optional<std::uint64_t>{1U} : std::optional<std::uint64_t>{revision};
}

Asset::AssetGuidUVE PrefabSystemUVE::SavePrefabUVE(IEntityManagerUVE& entityManager,
                                                    Asset::IAssetDatabaseUVE& assetDatabase,
                                                    EntityUVE rootEntity, const std::filesystem::path& path) {
    if (path.empty() || path.extension() != ".uveprefab") {
        UVE_ERROR("PrefabSystemUVE: prefab path must use the .uveprefab extension");
        return Asset::kInvalidAssetGuidUVE;
    }
    const bool saved =
        m_sceneSerializer.SaveUVE(entityManager, {rootEntity}, path, SceneAssetTypeUVE::Prefab);
    if (!saved) {
        UVE_ERROR("PrefabSystemUVE: failed to save prefab to \"{}\"", path.string());
        return Asset::kInvalidAssetGuidUVE;
    }

    Asset::AssetGuidUVE guid = Asset::kInvalidAssetGuidUVE;
    try {
        guid = assetDatabase.RegisterUVE(path);
        if (guid == Asset::kInvalidAssetGuidUVE) {
            UVE_ERROR("PrefabSystemUVE: asset database rejected prefab registration for \"{}\"", path.string());
            return Asset::kInvalidAssetGuidUVE;
        }
        static_cast<void>(assetDatabase.SaveUVE());
    } catch (const std::exception& exception) {
        UVE_ERROR("PrefabSystemUVE: asset database threw while registering prefab \"{}\": {}", path.string(),
                  exception.what());
        return Asset::kInvalidAssetGuidUVE;
    } catch (...) {
        UVE_ERROR("PrefabSystemUVE: asset database threw an unknown exception while registering prefab \"{}\"",
                  path.string());
        return Asset::kInvalidAssetGuidUVE;
    }
    return guid;
}

EntityUVE PrefabSystemUVE::InstantiateUVE(IEntityManagerUVE& entityManager, ISceneGraphUVE& sceneGraph,
                                           Asset::IAssetDatabaseUVE& assetDatabase,
                                           Asset::AssetGuidUVE prefabGuid, EntityUVE parent) {
    std::filesystem::path path;
    try {
        path = assetDatabase.ResolveUVE(prefabGuid);
    } catch (const std::exception& exception) {
        UVE_ERROR("PrefabSystemUVE: asset database threw while resolving prefab GUID {}: {}", prefabGuid.value,
                  exception.what());
        return kInvalidEntityUVE;
    } catch (...) {
        UVE_ERROR("PrefabSystemUVE: asset database threw an unknown exception while resolving prefab GUID {}",
                  prefabGuid.value);
        return kInvalidEntityUVE;
    }
    if (path.empty()) {
        UVE_ERROR("PrefabSystemUVE: unknown prefab GUID {}", prefabGuid.value);
        return kInvalidEntityUVE;
    }
    const std::optional<std::uint64_t> sourceRevision = ComputePrefabSourceRevisionUVE(path);
    if (!sourceRevision.has_value()) {
        UVE_ERROR("PrefabSystemUVE: could not compute source revision for prefab GUID {}", prefabGuid.value);
        return kInvalidEntityUVE;
    }
    return InstantiateWithRevisionUVE(entityManager, sceneGraph, assetDatabase, prefabGuid, parent, *sourceRevision);
}

EntityUVE PrefabSystemUVE::InstantiateWithRevisionUVE(
    IEntityManagerUVE& entityManager, ISceneGraphUVE& sceneGraph, Asset::IAssetDatabaseUVE& assetDatabase,
    Asset::AssetGuidUVE prefabGuid, EntityUVE parent, const std::uint64_t sourceRevision) {
    if (sourceRevision == 0U) {
        UVE_ERROR("PrefabSystemUVE: source revision must be nonzero for prefab GUID {}", prefabGuid.value);
        return kInvalidEntityUVE;
    }
    if (parent != kInvalidEntityUVE && !entityManager.IsAliveUVE(parent)) {
        UVE_ERROR("PrefabSystemUVE: cannot instantiate prefab GUID {} under an invalid parent ({}, {})",
                  prefabGuid.value, parent.index, parent.generation);
        return kInvalidEntityUVE;
    }
    std::filesystem::path path;
    try {
        path = assetDatabase.ResolveUVE(prefabGuid);
    } catch (const std::exception& exception) {
        UVE_ERROR("PrefabSystemUVE: asset database threw while resolving prefab GUID {}: {}", prefabGuid.value,
                  exception.what());
        return kInvalidEntityUVE;
    } catch (...) {
        UVE_ERROR("PrefabSystemUVE: asset database threw an unknown exception while resolving prefab GUID {}",
                  prefabGuid.value);
        return kInvalidEntityUVE;
    }
    if (path.empty()) {
        UVE_ERROR("PrefabSystemUVE: unknown prefab GUID {}", prefabGuid.value);
        return kInvalidEntityUVE;
    }

    const std::vector<EntityUVE> roots = m_sceneSerializer.LoadUVE(entityManager, path);
    if (roots.empty()) {
        UVE_ERROR("PrefabSystemUVE: failed to instantiate prefab \"{}\" (GUID {})", path.string(),
                  prefabGuid.value);
        return kInvalidEntityUVE;
    }

    // A prefab file always has exactly one root (SavePrefabUVE only ever serializes one entity).
    const EntityUVE root = roots.front();

    // The loaded root may already carry a PrefabInstanceComponentUVE (e.g. it was itself an
    // instance of a different prefab when this one was saved) — overwrite it so the tag always
    // names the prefab this entity was *directly* instantiated from.
    if (entityManager.HasComponentUVE<PrefabInstanceComponentUVE>(root)) {
        entityManager.RemoveComponentUVE<PrefabInstanceComponentUVE>(root);
    }
    entityManager.AddComponentUVE<PrefabInstanceComponentUVE>(
        root, PrefabInstanceComponentUVE{prefabGuid, {}, sourceRevision, sourceRevision});

    if (parent != kInvalidEntityUVE) {
        const bool hasTransform = entityManager.HasComponentUVE<TransformComponentUVE>(root);
        const bool hasWorldTransform = entityManager.HasComponentUVE<WorldTransformComponentUVE>(root);
        const bool hasHierarchy = entityManager.HasComponentUVE<HierarchyComponentUVE>(root);
        if (!hasTransform && !hasWorldTransform && !hasHierarchy) {
            // A parented prefab must participate in the scene graph even when its authored root
            // had no transform components; preserve transform-less roots only for unparented use.
            sceneGraph.AttachTransformUVE(entityManager, root, TransformComponentUVE{});
        }
        sceneGraph.SetParentUVE(entityManager, root, parent);
    }

    return root;
}

PrefabRefreshResultUVE PrefabSystemUVE::RefreshInstanceUVE(
    IEntityManagerUVE& entityManager, ISceneGraphUVE& sceneGraph,
    Asset::IAssetDatabaseUVE& assetDatabase, const EntityUVE instanceRoot, const bool forceRefresh) {
    if (!entityManager.IsAliveUVE(instanceRoot) ||
        !entityManager.HasComponentUVE<PrefabInstanceComponentUVE>(instanceRoot)) {
        return {PrefabRefreshCodeUVE::InvalidInstance, instanceRoot, 0U,
                "Prefab refresh requires a live entity with a prefab instance component."};
    }

    const PrefabInstanceComponentUVE instance =
        entityManager.GetComponentUVE<PrefabInstanceComponentUVE>(instanceRoot);
    std::filesystem::path path;
    try {
        path = assetDatabase.ResolveUVE(instance.sourcePrefabGuid);
    } catch (const std::exception&) {
        return {PrefabRefreshCodeUVE::SourceUnavailable, instanceRoot, 0U,
                "Prefab refresh could not resolve the source asset."};
    } catch (...) {
        return {PrefabRefreshCodeUVE::SourceUnavailable, instanceRoot, 0U,
                "Prefab refresh could not resolve the source asset."};
    }
    const std::optional<std::uint64_t> observedRevision = ComputePrefabSourceRevisionUVE(path);
    if (path.empty() || !observedRevision.has_value()) {
        return {PrefabRefreshCodeUVE::SourceUnavailable, instanceRoot, 0U,
                "Prefab refresh could not resolve or fingerprint the source asset."};
    }

    const PrefabRevisionRefreshDecisionUVE decision =
        *observedRevision == instance.instanceRevision
            ? PrefabRevisionRefreshDecisionUVE::NoOp
            : (instance.overrides.empty() ? PrefabRevisionRefreshDecisionUVE::Refresh
                                           : PrefabRevisionRefreshDecisionUVE::MergeRequired);
    if (decision == PrefabRevisionRefreshDecisionUVE::NoOp) {
        return {PrefabRefreshCodeUVE::NoOp, instanceRoot, *observedRevision,
                "Prefab instance already matches its source revision."};
    }
    if (decision == PrefabRevisionRefreshDecisionUVE::MergeRequired && !forceRefresh) {
        return {PrefabRefreshCodeUVE::MergeRequired, instanceRoot, *observedRevision,
                "Prefab source changed while local overrides are present; merge is required."};
    }
    if (decision != PrefabRevisionRefreshDecisionUVE::Refresh &&
        !(forceRefresh && decision == PrefabRevisionRefreshDecisionUVE::MergeRequired)) {
        return {PrefabRefreshCodeUVE::SourceRevisionInvalid, instanceRoot, *observedRevision,
                "Prefab source revision cannot safely refresh this instance."};
    }

    EntityUVE parent = kInvalidEntityUVE;
    if (entityManager.HasComponentUVE<HierarchyComponentUVE>(instanceRoot)) {
        parent = entityManager.GetComponentUVE<HierarchyComponentUVE>(instanceRoot).parent;
    }
    const EntityUVE replacement = InstantiateWithRevisionUVE(
        entityManager, sceneGraph, assetDatabase, instance.sourcePrefabGuid, parent, *observedRevision);
    if (replacement == kInvalidEntityUVE) {
        return {PrefabRefreshCodeUVE::SourceUnavailable, instanceRoot, *observedRevision,
                "Prefab refresh could not load a replacement source subtree."};
    }

    DestroySubtreeUVE(entityManager, sceneGraph, instanceRoot);
    return {PrefabRefreshCodeUVE::Refreshed, replacement, *observedRevision,
            "Prefab instance refreshed from its current source revision."};
}

} // namespace UVE::Scene
