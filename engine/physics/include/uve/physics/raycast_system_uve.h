//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include "uve/physics/i_raycast_system_uve.h"

namespace UVE::Physics {

/// RaycastSystemUVE is the concrete, engine-standard implementation of IRaycastSystemUVE.
/// Deliberately stateless (no members) — matches CollisionSystemUVE's precedent exactly.
class RaycastSystemUVE final : public IRaycastSystemUVE {
public:
    [[nodiscard]] std::optional<RaycastHitUVE> RaycastUVE(Scene::IEntityManagerUVE& entityManager,
                                                             const RaycastQueryUVE& query) const override;
};

} // namespace UVE::Physics
