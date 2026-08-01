//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <vector>

#include "uve/math/aabb_uve.h"
#include "uve/scene/i_entity_manager_uve.h"

namespace UVE::Physics::Detail {

/// One entity's world-space AABB and collision layer, computed once and reused across every
/// query against it. NOT a stable public contract — this is an internal implementation detail
/// shared by CollisionSystemUVE and RaycastSystemUVE (Increment 16) so their iteration logic
/// doesn't independently drift apart, and is expected to be replaced or subsumed by a real BVH
/// broad-phase builder once entity counts justify one; nothing outside engine/physics should
/// depend on this type or function.
struct ColliderWorldAabbUVE {
    Scene::EntityUVE entity;
    Math::AabbUVE worldAabb;
    std::uint32_t collisionLayer;
};

/// Builds a flat cache of every entity with both WorldTransformComponentUVE and
/// ColliderComponentUVE via one ForEachUVE pass — the exact loop CollisionSystemUVE::
/// DetectCollisionsUVE() and RaycastSystemUVE::RaycastUVE() both need before they can do their
/// own pairwise/linear-scan work.
[[nodiscard]] std::vector<ColliderWorldAabbUVE> BuildColliderWorldAabbCacheUVE(
    Scene::IEntityManagerUVE& entityManager);

} // namespace UVE::Physics::Detail
