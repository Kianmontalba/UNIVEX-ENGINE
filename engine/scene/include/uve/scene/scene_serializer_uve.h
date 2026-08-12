// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/scene/i_scene_serializer_uve.h"

namespace UVE::Scene {

/// SceneSerializerUVE is the concrete, engine-standard implementation of ISceneSerializerUVE.
/// Holds no persistent members of its own (same "stateless, dependencies passed per-call" shape
/// as SceneGraphUVE) — the per-component-type JSON (de)serialization table and the binary
/// envelope helpers are private implementation details entirely inside scene_serializer_uve.cpp,
/// including the only `#include <nlohmann/json.hpp>` in this module.
class SceneSerializerUVE final : public ISceneSerializerUVE {
public:
    SceneSerializerUVE() = default;

    [[nodiscard]] std::optional<SceneSnapshotUVE> CaptureUVE(
        IEntityManagerUVE& entityManager, const std::vector<EntityUVE>& rootEntities,
        SceneAssetTypeUVE assetType) const override;
    [[nodiscard]] std::vector<EntityUVE> RestoreUVE(IEntityManagerUVE& entityManager,
                                                     const SceneSnapshotUVE& snapshot) const override;

    [[nodiscard]] bool SaveUVE(IEntityManagerUVE& entityManager,
                                const std::vector<EntityUVE>& rootEntities,
                                const std::filesystem::path& path,
                                SceneAssetTypeUVE assetType) override;
    [[nodiscard]] std::vector<EntityUVE> LoadUVE(IEntityManagerUVE& entityManager,
                                                  const std::filesystem::path& path) override;
};

} // namespace UVE::Scene
