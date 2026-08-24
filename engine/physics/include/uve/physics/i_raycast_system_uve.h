// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <optional>

#include "uve/physics/raycast_hit_uve.h"
#include "uve/physics/raycast_query_uve.h"
#include "uve/scene/i_entity_manager_uve.h"

namespace UVE::Physics {

/// IRaycastSystemUVE is the spec's `RaycastSystemUVE` (Part 7.5). The point query routes known
/// Box/Sphere/Capsule colliders through exact bounded target geometry: oriented-box slab tests,
/// sphere quadratics, and finite capsule cylinder/cap tests. Unknown shape values retain the
/// conservative cached-AABB fallback. Shape casts remain a separate continuous-sweep API. The
/// interface is stateless and can later be replaced by a backend wrapper without call-site change.
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
