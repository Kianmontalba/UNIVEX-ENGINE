// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/scene/prefab_system_uve.h"

#include <exception>
#include <vector>

#include "uve/debug/logging_macros_uve.h"
#include "uve/scene/components/hierarchy_component_uve.h"
#include "uve/scene/components/prefab_instance_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Scene {

Asset::AssetGuidUVE PrefabSystemUVE::SavePrefabUVE(IEntityManagerUVE& entityManager,
                                                    Asset::IAssetDatabaseUVE& assetDatabase,
                                                    EntityUVE rootEntity, const std::filesystem::path& path) {
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
    return InstantiateWithRevisionUVE(entityManager, sceneGraph, assetDatabase, prefabGuid, parent, 1U);
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

} // namespace UVE::Scene
