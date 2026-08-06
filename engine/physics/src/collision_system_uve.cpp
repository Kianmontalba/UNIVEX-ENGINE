// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/physics/collision_system_uve.h"

#include <cstddef>
#include <optional>

#include "uve/math/aabb_uve.h"
#include "uve/physics/detail/collider_world_aabb_cache_uve.h"

namespace UVE::Physics {

std::vector<CollisionPairUVE> CollisionSystemUVE::DetectCollisionsUVE(Scene::IEntityManagerUVE& entityManager) const {
    const std::vector<Detail::ColliderWorldAabbUVE> colliders = Detail::BuildColliderWorldAabbCacheUVE(entityManager);

    std::vector<CollisionPairUVE> pairs;
    for (std::size_t i = 0; i < colliders.size(); ++i) {
        for (std::size_t j = i + 1; j < colliders.size(); ++j) {
            const std::optional<Math::PenetrationUVE> penetration =
                Math::ComputePenetrationUVE(colliders[i].worldAabb, colliders[j].worldAabb);
            if (penetration.has_value()) {
                pairs.push_back(
                    CollisionPairUVE{colliders[i].entity, colliders[j].entity, penetration->axis, penetration->depth});
            }
        }
    }
    return pairs;
}

} // namespace UVE::Physics
