//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/physics/detail/collider_world_aabb_cache_uve.h"

#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Physics::Detail {

std::vector<ColliderWorldAabbUVE> BuildColliderWorldAabbCacheUVE(Scene::IEntityManagerUVE& entityManager) {
    std::vector<ColliderWorldAabbUVE> cache;
    entityManager.ForEachUVE<Scene::WorldTransformComponentUVE, Scene::ColliderComponentUVE>(
        [&cache](Scene::EntityUVE entity, const Scene::WorldTransformComponentUVE& worldTransform,
                 const Scene::ColliderComponentUVE& collider) {
            cache.push_back(ColliderWorldAabbUVE{
                entity, Math::AabbUVE::FromCenterExtentsUVE(worldTransform.worldPosition, collider.halfExtents),
                collider.collisionLayer});
        });
    return cache;
}

} // namespace UVE::Physics::Detail
