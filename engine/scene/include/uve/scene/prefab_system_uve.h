// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/scene/i_prefab_system_uve.h"
#include "uve/scene/scene_serializer_uve.h"

namespace UVE::Scene {

/// PrefabSystemUVE is the concrete, engine-standard implementation of IPrefabSystemUVE. Owns a
/// private SceneSerializerUVE by value (it needs no constructor arguments of its own), so
/// callers never have to separately fetch a serializer just to save/instantiate prefabs.
class PrefabSystemUVE final : public IPrefabSystemUVE {
public:
    PrefabSystemUVE() = default;

    [[nodiscard]] Asset::AssetGuidUVE SavePrefabUVE(IEntityManagerUVE& entityManager,
                                                     Asset::IAssetDatabaseUVE& assetDatabase,
                                                     EntityUVE rootEntity,
                                                     const std::filesystem::path& path) override;
    [[nodiscard]] EntityUVE InstantiateUVE(IEntityManagerUVE& entityManager, ISceneGraphUVE& sceneGraph,
                                           Asset::IAssetDatabaseUVE& assetDatabase,
                                           Asset::AssetGuidUVE prefabGuid, EntityUVE parent) override;
    [[nodiscard]] EntityUVE InstantiateWithRevisionUVE(
        IEntityManagerUVE& entityManager, ISceneGraphUVE& sceneGraph,
        Asset::IAssetDatabaseUVE& assetDatabase, Asset::AssetGuidUVE prefabGuid, EntityUVE parent,
        std::uint64_t sourceRevision) override;

private:
    SceneSerializerUVE m_sceneSerializer;
};

} // namespace UVE::Scene
