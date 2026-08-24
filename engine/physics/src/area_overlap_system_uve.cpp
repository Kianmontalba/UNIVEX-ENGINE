// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/physics/area_overlap_system_uve.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <vector>

#include "uve/math/aabb_uve.h"
#include "uve/physics/detail/collider_world_aabb_cache_uve.h"
#include "uve/physics/detail/shape_narrow_phase_uve.h"
#include "uve/scene/components/area_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Physics {

namespace {

[[nodiscard]] bool IsFiniteVectorUVE(const Math::Vector3UVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool IsValidAabbUVE(const Math::AabbUVE& value) noexcept {
    return IsFiniteVectorUVE(value.min) && IsFiniteVectorUVE(value.max) && value.min.x <= value.max.x &&
           value.min.y <= value.max.y && value.min.z <= value.max.z;
}

struct AreaWorldAabbUVE final {
    Scene::EntityUVE entity;
    Math::AabbUVE worldAabb;
    std::uint32_t collisionLayer;
    std::uint32_t collisionMask;
};

struct AreaWorldAabbCacheUVE final {
    std::vector<AreaWorldAabbUVE> areas;
    bool truncated = false;
};

[[nodiscard]] AreaWorldAabbCacheUVE BuildAreaWorldAabbCacheUVE(
    Scene::IEntityManagerUVE& entityManager) {
    AreaWorldAabbCacheUVE cache;
    cache.areas.reserve(kMaximumAreaOverlapQueryAreasUVE);
    entityManager.ForEachUVE<Scene::WorldTransformComponentUVE, Scene::AreaComponentUVE>(
        [&cache](const Scene::EntityUVE entity, const Scene::WorldTransformComponentUVE& worldTransform,
                 const Scene::AreaComponentUVE& area) {
            if (!Scene::IsAreaComponentValidUVE(area)) {
                return;
            }
            if (cache.areas.size() >= kMaximumAreaOverlapQueryAreasUVE) {
                cache.truncated = true;
                return;
            }
            const Math::AabbUVE worldAabb =
                Math::AabbUVE::FromCenterExtentsUVE(worldTransform.worldPosition, area.halfExtents);
            if (!IsValidAabbUVE(worldAabb)) {
                return;
            }
            cache.areas.push_back(AreaWorldAabbUVE{entity, worldAabb, area.collisionLayer, area.collisionMask});
        });
    return cache;
}

[[nodiscard]] std::optional<Math::PenetrationUVE> ComputeAreaColliderPenetrationUVE(
    const AreaWorldAabbUVE& area, const Detail::ColliderWorldAabbUVE& collider) {
    switch (collider.shapeType) {
    case Scene::ColliderShapeTypeUVE::Sphere:
        return Detail::ComputeSphereAabbPenetrationUVE(
            area.worldAabb, collider.worldAabb.GetCenterUVE(), collider.shapeRadius);
    case Scene::ColliderShapeTypeUVE::Capsule:
        return Detail::ComputeCapsuleAabbPenetrationUVE(
            area.worldAabb, collider.shapeSegmentStart, collider.shapeSegmentEnd, collider.shapeRadius);
    case Scene::ColliderShapeTypeUVE::Box:
        return Detail::ComputeOrientedBoxOrientedBoxPenetrationUVE(
            area.worldAabb.GetCenterUVE(), area.worldAabb.GetExtentsUVE(), {},
            collider.worldAabb.GetCenterUVE(), collider.shapeHalfExtents, collider.shapeRotation);
    default:
        return Math::ComputePenetrationUVE(area.worldAabb, collider.worldAabb);
    }
}

} // namespace

AreaOverlapQueryResultUVE AreaOverlapSystemUVE::QueryUVE(
    Scene::IEntityManagerUVE& entityManager, const std::size_t maximumResults) {
    const AreaWorldAabbCacheUVE areaCache = BuildAreaWorldAabbCacheUVE(entityManager);
    const std::vector<Detail::ColliderWorldAabbUVE> colliders = Detail::BuildColliderWorldAabbCacheUVE(entityManager);

    const std::size_t resultCap = std::min(maximumResults, kMaximumAreaOverlapResultsUVE);
    AreaOverlapQueryResultUVE result;
    result.inspectedAreas = areaCache.areas.size();
    result.inspectedColliders = colliders.size();
    result.truncated = areaCache.truncated;
    result.overlaps.reserve(std::min(resultCap, colliders.size()));
    for (const AreaWorldAabbUVE& area : areaCache.areas) {
        for (const Detail::ColliderWorldAabbUVE& collider : colliders) {
            if ((area.collisionMask & collider.collisionLayer) == 0U ||
                (collider.collisionMask & area.collisionLayer) == 0U) {
                continue;
            }
            const std::optional<Math::PenetrationUVE> penetration =
                ComputeAreaColliderPenetrationUVE(area, collider);
            if (!penetration.has_value() || !std::isfinite(penetration->depth) || penetration->depth <= 0.0F) {
                continue;
            }
            if (result.overlaps.size() >= resultCap) {
                result.truncated = true;
                continue;
            }
            result.overlaps.push_back(AreaOverlapPairUVE{area.entity, collider.entity, penetration->depth});
        }
    }
    return result;
}

} // namespace UVE::Physics
