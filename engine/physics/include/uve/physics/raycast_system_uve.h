// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


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
