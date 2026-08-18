// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <vector>

#include "uve/physics/collision_pair_uve.h"
#include "uve/scene/i_entity_manager_uve.h"

namespace UVE::Physics {

/// ICollisionSystemUVE is the spec's `CollisionSystemUVE` (Part 7.5): pure collision *detection*
/// over every entity with both `Scene::WorldTransformComponentUVE` and
/// `Scene::ColliderComponentUVE`. The implementation uses a copied per-query median-split AABB BVH to
/// reduce broad-phase candidate work while preserving the legacy world-AABB narrow phase; the
/// detail-level DynamicAabbBvhUVE policy also supports refit-only updates for moved copied proxies; each
/// returned pair remains oriented and ordered by the original cache encounter indices so
/// sequential resolution is unchanged. The legacy box overlap is exact, while sphere/capsule
/// descriptors use conservative AABB bounds in v1 and need a future exact narrow phase. Deliberately returns plain
/// `CollisionPairUVE` values rather than mutating anything: resolving overlaps (deciding which
/// body moves, by how much) is `IPhysicsSystemUVE`'s job, not this one's.
/// Thread-safety: the public collision service remains stateless (holding no members), matching
/// `CameraSystemUVE`'s/`MeshRendererUVE`'s contract — every method only reads the
/// `IEntityManagerUVE` passed in.
class ICollisionSystemUVE {
public:
    virtual ~ICollisionSystemUVE() = default;

    /// Returns every overlapping pair of entities with both `WorldTransformComponentUVE` and
    /// `ColliderComponentUVE`, in a deterministic order matching the cache's encounter-index
    /// nested-pair order, independent of BVH traversal order — callers that resolve pairs
    /// sequentially can rely on this order being stable run-to-run given the same entity
    /// creation sequence.
    [[nodiscard]] virtual std::vector<CollisionPairUVE> DetectCollisionsUVE(
        Scene::IEntityManagerUVE& entityManager) const = 0;
};

} // namespace UVE::Physics
