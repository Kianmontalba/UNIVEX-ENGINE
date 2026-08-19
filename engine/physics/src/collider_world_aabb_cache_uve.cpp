// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/physics/detail/collider_world_aabb_cache_uve.h"

#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Physics::Detail {

std::vector<ColliderWorldAabbUVE> BuildColliderWorldAabbCacheUVE(Scene::IEntityManagerUVE& entityManager) {
    std::vector<ColliderWorldAabbUVE> cache;
    entityManager.ForEachUVE<Scene::WorldTransformComponentUVE, Scene::ColliderComponentUVE>(
        [&](Scene::EntityUVE entity, const Scene::WorldTransformComponentUVE& worldTransform,
                 const Scene::ColliderComponentUVE& collider) {
            cache.push_back(ColliderWorldAabbUVE{
                entity,
                Math::AabbUVE::FromCenterExtentsUVE(
                    worldTransform.worldPosition, Scene::GetColliderLocalHalfExtentsUVE(collider)),
                collider.collisionLayer,
                collider.collisionMask,
                collider.shapeType,
                collider.shapeType == Scene::ColliderShapeTypeUVE::Sphere ? collider.radius : 0.0F,
            });
        });
    return cache;
}

} // namespace UVE::Physics::Detail
