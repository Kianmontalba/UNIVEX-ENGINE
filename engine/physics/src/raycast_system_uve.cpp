// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/physics/raycast_system_uve.h"

#include "uve/math/aabb_uve.h"
#include "uve/physics/detail/collider_world_aabb_cache_uve.h"
#include "uve/physics/detail/shape_narrow_phase_uve.h"
#include "uve/physics/physics_material_uve.h"
#include "uve/scene/components/collider_component_uve.h"

namespace UVE::Physics {

std::optional<RaycastHitUVE> RaycastSystemUVE::RaycastUVE(Scene::IEntityManagerUVE& entityManager,
                                                            const RaycastQueryUVE& query) const {
    const std::vector<Detail::ColliderWorldAabbUVE> colliders = Detail::BuildColliderWorldAabbCacheUVE(entityManager);

    std::optional<RaycastHitUVE> closestHit;
    for (const Detail::ColliderWorldAabbUVE& collider : colliders) {
        if (collider.entity == query.ignoreEntity) {
            continue;
        }
        if ((collider.collisionLayer & query.layerMask) == 0) {
            continue;
        }

        std::optional<Math::RayHitUVE> hit;
        switch (collider.shapeType) {
        case Scene::ColliderShapeTypeUVE::Sphere:
            hit = Detail::IntersectRaySphereUVE(
                query.ray, collider.worldAabb.GetCenterUVE(), collider.shapeRadius, query.maxDistance);
            break;
        case Scene::ColliderShapeTypeUVE::Capsule:
            hit = Detail::IntersectRayCapsuleUVE(
                query.ray, collider.shapeSegmentStart, collider.shapeSegmentEnd, collider.shapeRadius,
                query.maxDistance);
            break;
        case Scene::ColliderShapeTypeUVE::Box:
            hit = Detail::IntersectRayOrientedBoxUVE(
                query.ray, collider.worldAabb.GetCenterUVE(), collider.shapeHalfExtents,
                collider.shapeRotation, query.maxDistance);
            break;
        default:
            hit = Math::IntersectRayUVE(query.ray, collider.worldAabb, query.maxDistance);
            break;
        }
        if (!hit.has_value()) {
            continue;
        }
        // Strict `<` (never `<=`): ties are broken deterministically by first-encountered-in-
        // cache-order — i.e. ForEachUVE's own deterministic chunk-order iteration — matching the
        // contract documented on IRaycastSystemUVE::RaycastUVE().
        if (closestHit.has_value() && hit->distance >= closestHit->distance) {
            continue;
        }

        const Scene::ColliderComponentUVE& colliderComponent =
            entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(collider.entity);
        closestHit = RaycastHitUVE{collider.entity, query.ray.origin + query.ray.direction * hit->distance,
                                    hit->normal, hit->distance, MaterialOfUVE(colliderComponent)};
    }
    return closestHit;
}

} // namespace UVE::Physics
