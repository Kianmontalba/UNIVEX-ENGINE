// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/physics/shape_cast_system_uve.h"

#include <cmath>
#include <limits>
#include <vector>

#include "uve/math/aabb_uve.h"
#include "uve/physics/detail/collider_world_aabb_cache_uve.h"
#include "uve/physics/detail/shape_narrow_phase_uve.h"
#include "uve/physics/physics_material_uve.h"
#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/i_entity_manager_uve.h"

namespace UVE::Physics {
namespace {

[[nodiscard]] bool IsFiniteVectorUVE(const Math::Vector3UVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool IsValidBoxCastRayUVE(const Math::RayUVE& ray) noexcept {
    const float lengthSquared = Math::LengthSquaredUVE(ray.direction);
    return IsFiniteVectorUVE(ray.origin) && IsFiniteVectorUVE(ray.direction) &&
           std::isfinite(lengthSquared) && lengthSquared > 0.0F;
}

[[nodiscard]] bool IsFiniteAabbUVE(const Math::AabbUVE& aabb) noexcept {
    return IsFiniteVectorUVE(aabb.min) && IsFiniteVectorUVE(aabb.max) &&
           aabb.min.x <= aabb.max.x && aabb.min.y <= aabb.max.y && aabb.min.z <= aabb.max.z;
}

} // namespace

std::optional<SphereCastHitUVE> ShapeCastSystemUVE::SphereCastUVE(
    Scene::IEntityManagerUVE& entityManager, const SphereCastQueryUVE& query) {
    if (!std::isfinite(query.radius) || query.radius < 0.0F || !std::isfinite(query.maxDistance) ||
        query.maxDistance < 0.0F || query.layerMask == 0U) {
        return std::nullopt;
    }

    const std::vector<Detail::ColliderWorldAabbUVE> colliders =
        Detail::BuildColliderWorldAabbCacheUVE(entityManager);
    std::optional<SphereCastHitUVE> closestHit;
    for (const Detail::ColliderWorldAabbUVE& collider : colliders) {
        if (collider.entity == query.ignoreEntity || (collider.collisionLayer & query.layerMask) == 0U) {
            continue;
        }

        std::optional<Math::RayHitUVE> hit;
        if (collider.shapeType == Scene::ColliderShapeTypeUVE::Sphere) {
            hit = Detail::IntersectMovingSphereSphereUVE(
                query.ray, collider.worldAabb.GetCenterUVE(), query.radius, collider.shapeRadius, query.maxDistance);
        } else if (collider.shapeType == Scene::ColliderShapeTypeUVE::Capsule) {
            const float expandedRadius = query.radius + collider.shapeRadius;
            if (std::isfinite(expandedRadius) && expandedRadius >= 0.0F) {
                hit = Detail::IntersectRayCapsuleUVE(query.ray, collider.shapeSegmentStart, collider.shapeSegmentEnd,
                                                     expandedRadius, query.maxDistance);
            }
        } else {
            const Math::Vector3UVE radiusExtents{query.radius, query.radius, query.radius};
            const Math::AabbUVE expandedAabb{
                collider.worldAabb.min - radiusExtents,
                collider.worldAabb.max + radiusExtents,
            };
            if (!IsFiniteAabbUVE(expandedAabb)) {
                continue;
            }
            hit = Math::IntersectRayUVE(query.ray, expandedAabb, query.maxDistance);
        }
        if (!hit.has_value() || (closestHit.has_value() && hit->distance >= closestHit->distance)) {
            continue;
        }

        const Scene::ColliderComponentUVE& colliderComponent =
            entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(collider.entity);
        closestHit = SphereCastHitUVE{collider.entity,
                                      query.ray.origin + query.ray.direction * hit->distance,
                                      hit->normal,
                                      hit->distance,
                                      MaterialOfUVE(colliderComponent)};
    }
    return closestHit;
}

std::optional<CapsuleCastHitUVE> ShapeCastSystemUVE::CapsuleCastUVE(
    Scene::IEntityManagerUVE& entityManager, const CapsuleCastQueryUVE& query) {
    if (!IsValidBoxCastRayUVE(query.ray) || !std::isfinite(query.radius) || query.radius <= 0.0F ||
        !std::isfinite(query.height) || query.height < 2.0F * query.radius ||
        !std::isfinite(query.maxDistance) || query.maxDistance < 0.0F || query.layerMask == 0U) {
        return std::nullopt;
    }

    const Math::Vector3UVE halfExtents{query.radius, query.height * 0.5F, query.radius};
    if (!IsFiniteVectorUVE(halfExtents)) {
        return std::nullopt;
    }
    const std::vector<Detail::ColliderWorldAabbUVE> colliders =
        Detail::BuildColliderWorldAabbCacheUVE(entityManager);
    std::optional<CapsuleCastHitUVE> closestHit;
    for (const Detail::ColliderWorldAabbUVE& collider : colliders) {
        if (collider.entity == query.ignoreEntity || (collider.collisionLayer & query.layerMask) == 0U) {
            continue;
        }
        const Math::AabbUVE expandedAabb{
            collider.worldAabb.min - halfExtents,
            collider.worldAabb.max + halfExtents,
        };
        if (!IsFiniteAabbUVE(expandedAabb)) {
            continue;
        }
        const std::optional<Math::RayHitUVE> hit =
            Math::IntersectRayUVE(query.ray, expandedAabb, query.maxDistance);
        if (!hit.has_value()) {
            continue;
        }
        const bool closer = !closestHit.has_value() || hit->distance < closestHit->distance;
        const bool tied = closestHit.has_value() && hit->distance == closestHit->distance &&
                          collider.entity.index < closestHit->entity.index;
        if (!closer && !tied) {
            continue;
        }
        const Scene::ColliderComponentUVE& colliderComponent =
            entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(collider.entity);
        closestHit = CapsuleCastHitUVE{collider.entity,
                                       query.ray.origin + query.ray.direction * hit->distance,
                                       hit->normal,
                                       hit->distance,
                                       MaterialOfUVE(colliderComponent)};
    }
    return closestHit;
}

std::optional<BoxCastHitUVE> ShapeCastSystemUVE::BoxCastUVE(
    Scene::IEntityManagerUVE& entityManager, const BoxCastQueryUVE& query) {
    if (!IsValidBoxCastRayUVE(query.ray) || !IsFiniteVectorUVE(query.halfExtents) ||
        query.halfExtents.x < 0.0F || query.halfExtents.y < 0.0F || query.halfExtents.z < 0.0F ||
        !std::isfinite(query.maxDistance) || query.maxDistance < 0.0F || query.layerMask == 0U) {
        return std::nullopt;
    }

    const std::vector<Detail::ColliderWorldAabbUVE> colliders =
        Detail::BuildColliderWorldAabbCacheUVE(entityManager);
    std::optional<BoxCastHitUVE> closestHit;
    for (const Detail::ColliderWorldAabbUVE& collider : colliders) {
        if (collider.entity == query.ignoreEntity || (collider.collisionLayer & query.layerMask) == 0U) {
            continue;
        }

        const Math::AabbUVE expandedAabb{
            collider.worldAabb.min - query.halfExtents,
            collider.worldAabb.max + query.halfExtents,
        };
        if (!IsFiniteAabbUVE(expandedAabb)) {
            continue;
        }
        const std::optional<Math::RayHitUVE> hit =
            Math::IntersectRayUVE(query.ray, expandedAabb, query.maxDistance);
        if (!hit.has_value()) {
            continue;
        }
        const bool closer = !closestHit.has_value() || hit->distance < closestHit->distance;
        const bool tied = closestHit.has_value() && hit->distance == closestHit->distance &&
                          collider.entity.index < closestHit->entity.index;
        if (!closer && !tied) {
            continue;
        }

        const Scene::ColliderComponentUVE& colliderComponent =
            entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(collider.entity);
        closestHit = BoxCastHitUVE{collider.entity,
                                   query.ray.origin + query.ray.direction * hit->distance,
                                   hit->normal,
                                   hit->distance,
                                   MaterialOfUVE(colliderComponent)};
    }
    return closestHit;
}

} // namespace UVE::Physics
