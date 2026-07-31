//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

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

    [[nodiscard]] bool SaveUVE(IEntityManagerUVE& entityManager,
                                const std::vector<EntityUVE>& rootEntities,
                                const std::filesystem::path& path,
                                SceneAssetTypeUVE assetType) override;
    [[nodiscard]] std::vector<EntityUVE> LoadUVE(IEntityManagerUVE& entityManager,
                                                  const std::filesystem::path& path) override;
};

} // namespace UVE::Scene
