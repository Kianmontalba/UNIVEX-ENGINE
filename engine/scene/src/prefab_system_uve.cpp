// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/scene/prefab_system_uve.h"

#include <vector>

#include "uve/debug/logging_macros_uve.h"
#include "uve/scene/components/prefab_instance_component_uve.h"

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

    const Asset::AssetGuidUVE guid = assetDatabase.RegisterUVE(path);
    assetDatabase.SaveUVE();
    return guid;
}

EntityUVE PrefabSystemUVE::InstantiateUVE(IEntityManagerUVE& entityManager, ISceneGraphUVE& sceneGraph,
                                           Asset::IAssetDatabaseUVE& assetDatabase,
                                           Asset::AssetGuidUVE prefabGuid, EntityUVE parent) {
    const std::filesystem::path path = assetDatabase.ResolveUVE(prefabGuid);
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
    entityManager.AddComponentUVE<PrefabInstanceComponentUVE>(root, PrefabInstanceComponentUVE{prefabGuid, {}});

    if (parent != kInvalidEntityUVE) {
        sceneGraph.SetParentUVE(entityManager, root, parent);
    }

    return root;
}

} // namespace UVE::Scene
