// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/physics/area_overlap_system_uve.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

#include "uve/math/aabb_uve.h"
#include "uve/physics/detail/collider_world_aabb_cache_uve.h"
#include "uve/scene/components/area_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Physics {

namespace {

struct AreaWorldAabbUVE final {
    Scene::EntityUVE entity;
    Math::AabbUVE worldAabb;
    std::uint32_t collisionLayer;
    std::uint32_t collisionMask;
};

[[nodiscard]] std::vector<AreaWorldAabbUVE> BuildAreaWorldAabbCacheUVE(
    Scene::IEntityManagerUVE& entityManager) {
    std::vector<AreaWorldAabbUVE> areas;
    entityManager.ForEachUVE<Scene::WorldTransformComponentUVE, Scene::AreaComponentUVE>(
        [&areas](const Scene::EntityUVE entity, const Scene::WorldTransformComponentUVE& worldTransform,
                 const Scene::AreaComponentUVE& area) {
            if (!Scene::IsAreaComponentValidUVE(area)) {
                return;
            }
            areas.push_back(AreaWorldAabbUVE{
                entity, Math::AabbUVE::FromCenterExtentsUVE(worldTransform.worldPosition, area.halfExtents),
                area.collisionLayer, area.collisionMask});
        });
    return areas;
}

} // namespace

AreaOverlapQueryResultUVE AreaOverlapSystemUVE::QueryUVE(
    Scene::IEntityManagerUVE& entityManager, const std::size_t maximumResults) {
    const std::vector<AreaWorldAabbUVE> areas = BuildAreaWorldAabbCacheUVE(entityManager);
    const std::vector<Detail::ColliderWorldAabbUVE> colliders = Detail::BuildColliderWorldAabbCacheUVE(entityManager);

    const std::size_t resultCap = std::min(maximumResults, kMaximumAreaOverlapResultsUVE);
    AreaOverlapQueryResultUVE result;
    result.inspectedAreas = areas.size();
    result.inspectedColliders = colliders.size();
    result.overlaps.reserve(std::min(resultCap, colliders.size()));
    for (const AreaWorldAabbUVE& area : areas) {
        for (const Detail::ColliderWorldAabbUVE& collider : colliders) {
            if ((area.collisionMask & collider.collisionLayer) == 0U ||
                (collider.collisionMask & area.collisionLayer) == 0U) {
                continue;
            }
            const std::optional<Math::PenetrationUVE> penetration =
                Math::ComputePenetrationUVE(area.worldAabb, collider.worldAabb);
            if (!penetration.has_value()) {
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
