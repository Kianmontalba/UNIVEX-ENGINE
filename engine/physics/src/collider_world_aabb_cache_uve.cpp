// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/physics/detail/collider_world_aabb_cache_uve.h"

#include <cmath>

#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Physics::Detail {

std::vector<ColliderWorldAabbUVE> BuildColliderWorldAabbCacheUVE(Scene::IEntityManagerUVE& entityManager) {
    std::vector<ColliderWorldAabbUVE> cache;
    entityManager.ForEachUVE<Scene::WorldTransformComponentUVE, Scene::ColliderComponentUVE>(
        [&](Scene::EntityUVE entity, const Scene::WorldTransformComponentUVE& worldTransform,
                 const Scene::ColliderComponentUVE& collider) {
            const bool isSphere = collider.shapeType == Scene::ColliderShapeTypeUVE::Sphere;
            const bool isCapsule = collider.shapeType == Scene::ColliderShapeTypeUVE::Capsule;
            const float capsuleHalfSegment = isCapsule ? collider.height * 0.5F - collider.radius : 0.0F;
            const Math::Vector3UVE localCapsuleOffset{0.0F, capsuleHalfSegment, 0.0F};
            const Math::Vector3UVE worldCapsuleOffset =
                isCapsule ? Math::RotateVectorUVE(worldTransform.worldRotation, localCapsuleOffset)
                          : Math::Vector3UVE{};
            const Math::Vector3UVE capsuleSegmentStart = worldTransform.worldPosition - worldCapsuleOffset;
            const Math::Vector3UVE capsuleSegmentEnd = worldTransform.worldPosition + worldCapsuleOffset;
            const Math::Vector3UVE worldHalfExtents = isCapsule
                                                          ? Math::Vector3UVE{
                                                                collider.radius + std::fabs(worldCapsuleOffset.x),
                                                                collider.radius + std::fabs(worldCapsuleOffset.y),
                                                                collider.radius + std::fabs(worldCapsuleOffset.z),
                                                            }
                                                          : Scene::GetColliderLocalHalfExtentsUVE(collider);
            cache.push_back(ColliderWorldAabbUVE{
                entity,
                Math::AabbUVE::FromCenterExtentsUVE(worldTransform.worldPosition, worldHalfExtents),
                collider.collisionLayer,
                collider.collisionMask,
                collider.shapeType,
                (isSphere || isCapsule) ? collider.radius : 0.0F,
                capsuleSegmentStart,
                capsuleSegmentEnd,
            });
        });
    return cache;
}

} // namespace UVE::Physics::Detail
