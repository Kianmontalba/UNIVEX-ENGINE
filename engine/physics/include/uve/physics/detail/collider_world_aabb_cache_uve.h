// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>
#include <vector>

#include "uve/math/aabb_uve.h"
#include "uve/scene/i_entity_manager_uve.h"

namespace UVE::Physics::Detail {

/// One entity's world-space AABB and collision layer/mask, computed once and reused across every
/// query against it. NOT a stable public contract — this is an internal implementation detail
/// shared by CollisionSystemUVE, its transient BVH builder, and RaycastSystemUVE (Increment 16)
/// so their shape extraction and iteration inputs cannot independently drift; nothing outside
/// engine/physics should depend on this type or function.
struct ColliderWorldAabbUVE {
    Scene::EntityUVE entity;
    Math::AabbUVE worldAabb;
    std::uint32_t collisionLayer;
    std::uint32_t collisionMask;
};

/// Builds a flat cache of every entity with both WorldTransformComponentUVE and
/// ColliderComponentUVE via one ForEachUVE pass. CollisionSystemUVE builds its transient static
/// BVH from this snapshot, while RaycastSystemUVE retains its deterministic linear query path.
[[nodiscard]] std::vector<ColliderWorldAabbUVE> BuildColliderWorldAabbCacheUVE(
    Scene::IEntityManagerUVE& entityManager);

} // namespace UVE::Physics::Detail
