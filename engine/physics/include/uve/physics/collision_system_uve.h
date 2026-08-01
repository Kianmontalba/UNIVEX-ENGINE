//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include "uve/physics/i_collision_system_uve.h"

namespace UVE::Physics {

/// CollisionSystemUVE is the concrete, engine-standard implementation of ICollisionSystemUVE.
/// Deliberately stateless (no members, no PIMPL) — matches CameraSystemUVE's/MeshRendererUVE's
/// precedent for utility services that always take the IEntityManagerUVE& they operate on as an
/// explicit parameter rather than owning a reference to one.
class CollisionSystemUVE final : public ICollisionSystemUVE {
public:
    [[nodiscard]] std::vector<CollisionPairUVE> DetectCollisionsUVE(
        Scene::IEntityManagerUVE& entityManager) const override;
};

} // namespace UVE::Physics
