//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include "uve/scene/i_scene_graph_uve.h"

namespace UVE::Scene {

/// SceneGraphUVE is the concrete, engine-standard implementation of ISceneGraphUVE. Holds no
/// persistent members of its own — every method takes the IEntityManagerUVE it operates on
/// explicitly, so it is trivially constructible/testable and has no ordering dependency on any
/// specific manager instance's construction.
/// Thread-safety: not thread-safe — every method must be called only from the main/scene
/// thread, matching IEntityManagerUVE's own contract.
class SceneGraphUVE final : public ISceneGraphUVE {
public:
    SceneGraphUVE() = default;

    void AttachTransformUVE(IEntityManagerUVE& entityManager, EntityUVE entity,
                             const TransformComponentUVE& localTransform) override;
    void SetLocalTransformUVE(IEntityManagerUVE& entityManager, EntityUVE entity,
                               const TransformComponentUVE& localTransform) override;
    void SetParentUVE(IEntityManagerUVE& entityManager, EntityUVE child, EntityUVE newParent) override;
    void UpdateUVE(IEntityManagerUVE& entityManager) override;
    [[nodiscard]] std::vector<EntityUVE> GetChildrenUVE(IEntityManagerUVE& entityManager,
                                                         EntityUVE parent) override;
};

} // namespace UVE::Scene
