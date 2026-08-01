//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <optional>

#include "uve/physics/raycast_hit_uve.h"
#include "uve/physics/raycast_query_uve.h"
#include "uve/scene/i_entity_manager_uve.h"

namespace UVE::Physics {

/// IRaycastSystemUVE is the spec's `RaycastSystemUVE` (Part 7.5). This increment implements only
/// the `Raycast` query (single closest-hit point query) — `SphereCast`/`BoxCast`/`CapsuleCast` are
/// deferred as a group: colliders are box-only (nothing to sphere/capsule-cast against), and even
/// a `BoxCast` is a genuinely harder continuous-sweep problem, not a small extension of a point
/// ray test. Deliberately interfaced the same way ICollisionSystemUVE/IPhysicsSystemUVE are, so a
/// future backend-wrapping implementation (Jolt/Bullet) could replace RaycastSystemUVE without any
/// call-site change.
/// Thread-safety: implementations should be stateless (no members), matching CollisionSystemUVE's
/// contract — every method only reads the IEntityManagerUVE passed in; RaycastUVE() performs no
/// mutation, so concurrent read-only calls are safe as long as the passed-in IEntityManagerUVE's
/// own concurrent-read guarantees hold (a pre-existing property, not something this interface
/// changes).
class IRaycastSystemUVE {
public:
    virtual ~IRaycastSystemUVE() = default;

    /// Returns the closest hit within `[0, query.maxDistance]`, or std::nullopt if nothing was
    /// hit. Only entities with both WorldTransformComponentUVE and ColliderComponentUVE are
    /// considered; `query.ignoreEntity` is excluded outright, `query.layerMask` filters via
    /// `(collider.collisionLayer & layerMask) != 0`. Ties (multiple entities at exactly the same
    /// distance) are broken deterministically: whichever entity ForEachUVE's own deterministic
    /// chunk-order iteration encounters first wins — the same same-platform/same-build/
    /// same-input-sequence determinism scope PhysicsSystemUVE (Increment 15) already documents,
    /// made an explicit contract here rather than an accidental byproduct.
    [[nodiscard]] virtual std::optional<RaycastHitUVE> RaycastUVE(Scene::IEntityManagerUVE& entityManager,
                                                                    const RaycastQueryUVE& query) const = 0;
};

} // namespace UVE::Physics
