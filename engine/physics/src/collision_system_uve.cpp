//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/physics/collision_system_uve.h"

#include <cstddef>
#include <optional>

#include "uve/math/aabb_uve.h"
#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Physics {

namespace {

/// One entity's world-space AABB, computed once per DetectCollisionsUVE() call and reused across
/// every pairwise comparison — avoids recomputing the same AABB O(n) times per entity.
struct EntityAabbUVE {
    Scene::EntityUVE entity;
    Math::AabbUVE worldAabb;
};

} // namespace

std::vector<CollisionPairUVE> CollisionSystemUVE::DetectCollisionsUVE(Scene::IEntityManagerUVE& entityManager) const {
    std::vector<EntityAabbUVE> entityAabbs;
    entityManager.ForEachUVE<Scene::WorldTransformComponentUVE, Scene::ColliderComponentUVE>(
        [&entityAabbs](Scene::EntityUVE entity, const Scene::WorldTransformComponentUVE& worldTransform,
                       const Scene::ColliderComponentUVE& collider) {
            entityAabbs.push_back(EntityAabbUVE{
                entity, Math::AabbUVE::FromCenterExtentsUVE(worldTransform.worldPosition, collider.halfExtents)});
        });

    std::vector<CollisionPairUVE> pairs;
    for (std::size_t i = 0; i < entityAabbs.size(); ++i) {
        for (std::size_t j = i + 1; j < entityAabbs.size(); ++j) {
            const std::optional<Math::PenetrationUVE> penetration =
                Math::ComputePenetrationUVE(entityAabbs[i].worldAabb, entityAabbs[j].worldAabb);
            if (penetration.has_value()) {
                pairs.push_back(CollisionPairUVE{entityAabbs[i].entity, entityAabbs[j].entity, penetration->axis,
                                                  penetration->depth});
            }
        }
    }
    return pairs;
}

} // namespace UVE::Physics
