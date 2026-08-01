//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <vector>

#include "uve/physics/collision_pair_uve.h"
#include "uve/scene/i_entity_manager_uve.h"

namespace UVE::Physics {

/// ICollisionSystemUVE is the spec's `CollisionSystemUVE` (Part 7.5): pure collision *detection*
/// over every entity with both `Scene::WorldTransformComponentUVE` and
/// `Scene::ColliderComponentUVE`. This increment's broad-phase is a naive O(n^2) pairwise AABB
/// overlap test (a BVH is real future work once entity counts justify it — matches
/// `ISceneGraphUVE::GetChildrenUVE()`'s own "profile before optimizing" precedent); since the
/// only collider shape is an axis-aligned box, the overlap test IS the narrow-phase result too —
/// there is no false positive left to refine away. Deliberately returns plain
/// `CollisionPairUVE` values rather than mutating anything: resolving overlaps (deciding which
/// body moves, by how much) is `IPhysicsSystemUVE`'s job, not this one's.
/// Thread-safety: implementations should be stateless (holding no members), matching
/// `CameraSystemUVE`'s/`MeshRendererUVE`'s contract — every method only reads the
/// `IEntityManagerUVE` passed in.
class ICollisionSystemUVE {
public:
    virtual ~ICollisionSystemUVE() = default;

    /// Returns every overlapping pair of entities with both `WorldTransformComponentUVE` and
    /// `ColliderComponentUVE`, in a deterministic order (fixed by `ForEachUVE`'s own chunk-order
    /// iteration, then nested pairwise comparison order) — callers that resolve pairs
    /// sequentially can rely on this order being stable run-to-run given the same entity
    /// creation sequence.
    [[nodiscard]] virtual std::vector<CollisionPairUVE> DetectCollisionsUVE(
        Scene::IEntityManagerUVE& entityManager) const = 0;
};

} // namespace UVE::Physics
